#include "vcAtmosphereRenderer.h"
#include "vcState.h"
#include "vcInternalModels.h"

#include "gl/vcShader.h"
#include "gl/vcRenderShaders.h"

#include "udGeoZone.h"
#include "udMath.h"
#include "udPlatform.h"
#include "udStringUtil.h"

#include "atmosphere/model.h"

struct vcAtmosphereRenderer
{
  atmosphere::Model *pModel;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_sceneColour;
    vcShaderSampler *uniform_sceneDepth;
    vcShaderConstantBuffer *uniform_vertParams;
    vcShaderConstantBuffer *uniform_fragParams;

    struct
    {
      udFloat4 camera; // (camera position).xyz, exposure.w
      udFloat4 white_point; // w unused
      udFloat4 earth_center; // w unused
      udFloat4 sun_direction;// w unused
      udFloat4 sun_size; //zw unused
      udFloat4x4 inverseProjection;
    } fragParams;

    struct
    {
      udFloat4x4 modelFromView;
      udFloat4x4 viewFromClip;
    } vertParams;

  } renderShader;
};

int previous_mouse_x_ = 0;
int previous_mouse_y_ = 0;

constexpr double kPi = 3.1415926;
constexpr double kSunAngularRadius = 0.00935 / 2.0;
constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
constexpr double kLengthUnitInMeters = 1.0;

//#include "atmosphere/demo/demo.glsl.inc"

enum Luminance {
  // Render the spectral radiance at kLambdaR, kLambdaG, kLambdaB.
  NONE,
  // Render the sRGB luminance, using an approximate (on the fly) conversion
  // from 3 spectral radiance values only (see section 14.3 in <a href=
  // "https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
  //  Evaluation of 8 Clear Sky Models</a>).
  APPROXIMATE,
  // Render the sRGB luminance, precomputed from 15 spectral radiance values
  // (see section 4.4 in <a href=
  // "http://www.oskee.wz.cz/stranka/uploads/SCCG10ElekKmoch.pdf">Real-time
  //  Spectral Scattering in Large-scale Natural Participating Media</a>).
  PRECOMPUTED
};

bool use_constant_solar_spectrum_ = false;
bool use_ozone_ = true;
Luminance use_luminance_ = NONE;
bool use_half_precision_ = true;
bool use_combined_textures_ = true;
bool do_white_balance_ = false;

double view_zenith_angle_radians_ = 1.47;
double view_azimuth_angle_radians_ = -0.1;
double sun_zenith_angle_radians_ = 1.3;
double sun_azimuth_angle_radians_ = 2.9;
double exposure_ = 10.0;

void vcAtmosphereRenderer_HandleFakeInput(vcAtmosphereRenderer *pAtmosphereRenderer);

udResult vcAtmosphereRenderer_Create(vcAtmosphereRenderer **ppAtmosphereRenderer)
{
  udResult result;
  vcAtmosphereRenderer *pAtmosphereRenderer = nullptr;
  const char *pCompleteAtmosphereShaderSource = nullptr;

  double white_point_r = 1.0;
  double white_point_g = 1.0;
  double white_point_b = 1.0;

  // Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
  // (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
  // summed and averaged in each bin (e.g. the value for 360nm is the average
  // of the ASTM G-173 values for all wavelengths between 360 and 370nm).
  // Values in W.m^-2.
  constexpr int kLambdaMin = 360;
  constexpr int kLambdaMax = 830;
  constexpr double kSolarIrradiance[48] = {
    1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
    1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
    1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
    1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
    1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
    1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
  };
  // Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
  // referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
  // each bin (e.g. the value for 360nm is the average of the original values
  // for all wavelengths between 360 and 370nm). Values in m^2.
  constexpr double kOzoneCrossSection[48] = {
    1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
    8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
    1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
    4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
    2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
    6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
    2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
  };
  // From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
  constexpr double kDobsonUnit = 2.687e20;
  // Maximum number density of ozone molecules, in m^-3 (computed so at to get
  // 300 Dobson units of ozone - for this we divide 300 DU by the integral of
  // the ozone density profile defined below, which is equal to 15km).
  constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
  // Wavelength independent solar irradiance "spectrum" (not physically
  // realistic, but was used in the original implementation).
  constexpr double kConstantSolarIrradiance = 1.5;
  constexpr double kBottomRadius = (6378137.0);//6360000.0;
  constexpr double kTopRadius = (6420000.0 + 18137.0);
  constexpr double kRayleigh = 1.24062e-6;
  constexpr double kRayleighScaleHeight = 8000.0;
  constexpr double kMieScaleHeight = 1200.0;
  constexpr double kMieAngstromAlpha = 0.0;
  constexpr double kMieAngstromBeta = 5.328e-3;
  constexpr double kMieSingleScatteringAlbedo = 0.9;
  constexpr double kMiePhaseFunctionG = 0.8;
  constexpr double kGroundAlbedo = 0.1;
  const double max_sun_zenith_angle =
    (use_half_precision_ ? 102.0 : 120.0) / 180.0 * kPi;

  atmosphere::DensityProfileLayer
    rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
  atmosphere::DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
  // Density profile increasing linearly from 0 to 1 between 10 and 25km, and
  // decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
  // profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
  // Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
  std::vector<atmosphere::DensityProfileLayer> ozone_density;
  ozone_density.push_back(
    atmosphere::DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
  ozone_density.push_back(
    atmosphere::DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

  std::vector<double> wavelengths;
  std::vector<double> solar_irradiance;
  std::vector<double> rayleigh_scattering;
  std::vector<double> mie_scattering;
  std::vector<double> mie_extinction;
  std::vector<double> absorption_extinction;
  std::vector<double> ground_albedo;
  for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
    double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
    double mie =
      kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
    wavelengths.push_back(l);
    if (use_constant_solar_spectrum_) {
      solar_irradiance.push_back(kConstantSolarIrradiance);
    }
    else {
      solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
    }
    rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
    mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
    mie_extinction.push_back(mie);
    absorption_extinction.push_back(use_ozone_ ?
      kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] :
      0.0);
    ground_albedo.push_back(kGroundAlbedo);
  }

  UD_ERROR_NULL(ppAtmosphereRenderer, udR_InvalidParameter_);

  pAtmosphereRenderer = udAllocType(vcAtmosphereRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pAtmosphereRenderer, udR_MemoryAllocationFailure);

  pAtmosphereRenderer->pModel = new atmosphere::Model(wavelengths, solar_irradiance, kSunAngularRadius,
    kBottomRadius, kTopRadius, { rayleigh_layer }, rayleigh_scattering,
    { mie_layer }, mie_scattering, mie_extinction, kMiePhaseFunctionG,
    ozone_density, absorption_extinction, ground_albedo, max_sun_zenith_angle,
    kLengthUnitInMeters, use_luminance_ == PRECOMPUTED ? 15 : 3,
    use_combined_textures_, use_half_precision_);
  pAtmosphereRenderer->pModel->Init();

  udSprintf(&pCompleteAtmosphereShaderSource, "%s\n%s", pAtmosphereRenderer->pModel->shaderHeader().c_str(), g_AtmosphereFragmentShader);
  UD_ERROR_IF(!vcShader_CreateFromText(&pAtmosphereRenderer->renderShader.pProgram, g_AtmosphereVertexShader, pCompleteAtmosphereShaderSource, vcP3UV2VertexLayout), udR_InternalError);

  vcShader_Bind(pAtmosphereRenderer->renderShader.pProgram);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_sceneColour, pAtmosphereRenderer->renderShader.pProgram, "u_sceneColour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_sceneDepth, pAtmosphereRenderer->renderShader.pProgram, "u_sceneDepth"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pAtmosphereRenderer->renderShader.uniform_vertParams, pAtmosphereRenderer->renderShader.pProgram, "u_vertParams", sizeof(pAtmosphereRenderer->renderShader.vertParams)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pAtmosphereRenderer->renderShader.uniform_fragParams, pAtmosphereRenderer->renderShader.pProgram, "u_fragParams", sizeof(pAtmosphereRenderer->renderShader.fragParams)), udR_InternalError);

  pAtmosphereRenderer->pModel->SetProgramUniforms(pAtmosphereRenderer->renderShader.pProgram->programID, 0, 1, 2, 3);
  if (do_white_balance_) {
    atmosphere::Model::ConvertSpectrumToLinearSrgb(wavelengths, solar_irradiance,
      &white_point_r, &white_point_g, &white_point_b);
    double white_point = (white_point_r + white_point_g + white_point_b) / 3.0;
    white_point_r /= white_point;
    white_point_g /= white_point;
    white_point_b /= white_point;
  }

  pAtmosphereRenderer->renderShader.fragParams.white_point.x = (float)white_point_r;
  pAtmosphereRenderer->renderShader.fragParams.white_point.y = (float)white_point_g;
  pAtmosphereRenderer->renderShader.fragParams.white_point.z = (float)white_point_b;
  pAtmosphereRenderer->renderShader.fragParams.sun_size.x = (float)udTan(kSunAngularRadius);
  pAtmosphereRenderer->renderShader.fragParams.sun_size.y = (float)udCos(kSunAngularRadius);

 //pAtmosphereRenderer->pModel->SetProgramUniforms(pAtmosphereRenderer->renderShader.pProgram->programID, 0, 1, 2, 3);

  *ppAtmosphereRenderer = pAtmosphereRenderer;
  result = udR_Success;
epilogue:
  udFree(pCompleteAtmosphereShaderSource);
  return result;
}

udResult vcAtmosphereRenderer_Destroy(vcAtmosphereRenderer **ppAtmosphereRenderer)
{
  if (ppAtmosphereRenderer == nullptr || *ppAtmosphereRenderer == nullptr)
    return udR_InvalidParameter_;

  delete (*ppAtmosphereRenderer)->pModel;
  vcShader_DestroyShader(&(*ppAtmosphereRenderer)->renderShader.pProgram);
  udFree((*ppAtmosphereRenderer));

  return udR_Success;
}

bool vcAtmosphereRenderer_Render(vcAtmosphereRenderer *pAtmosphereRenderer, vcState *pProgramState, vcTexture *pSceneColour, vcTexture *pSceneDepth)
{
  bool result = true;

  vcAtmosphereRenderer_HandleFakeInput(pAtmosphereRenderer);

  vcShader_Bind(pAtmosphereRenderer->renderShader.pProgram);
  pAtmosphereRenderer->pModel->SetProgramUniforms(pAtmosphereRenderer->renderShader.pProgram->programID, 0, 1, 2, 3);

  udDouble3 earthCenter = pProgramState->camera.position;

  if (!pProgramState->gis.isProjected || pProgramState->gis.zone.projection >= udGZPT_TransverseMercator) //TODO: Fix this list
  {
    if (pProgramState->gis.isProjected)
      earthCenter.z = -pProgramState->gis.zone.semiMajorAxis;
    else
      earthCenter.z = -6378137.000;
  }
  else
  {
    udGeoZone destZone = {};
    udGeoZone_SetFromSRID(&destZone, 4978);
    earthCenter = udGeoZone_TransformPoint(pProgramState->camera.position, pProgramState->gis.zone, destZone);
  }

  earthCenter.z /= kLengthUnitInMeters;

  udDouble4 earthCenterEyePos = udDouble4::create(earthCenter, 1.0);//pProgramState->camera.matrices.view *udDouble4::create(earthCenter, 1.0);

  udFloat4x4 inverseProjection = udFloat4x4::create(udInverse(pProgramState->camera.matrices.projection));
  udFloat4x4 inverseView = udFloat4x4::create(udInverse(pProgramState->camera.matrices.view));
  //udFloat4x4 inverseViewProjection = udFloat4x4::create(pProgramState->camera.matrices.inverseViewProjection);

  pAtmosphereRenderer->renderShader.vertParams.viewFromClip = inverseProjection;
  pAtmosphereRenderer->renderShader.vertParams.modelFromView = inverseView;

  pAtmosphereRenderer->renderShader.fragParams.earth_center.x = (float)earthCenterEyePos.x;
  pAtmosphereRenderer->renderShader.fragParams.earth_center.y = (float)earthCenterEyePos.y;
  pAtmosphereRenderer->renderShader.fragParams.earth_center.z = (float)earthCenterEyePos.z;
  pAtmosphereRenderer->renderShader.fragParams.inverseProjection = inverseProjection;
  pAtmosphereRenderer->renderShader.fragParams.camera.x = (float)pProgramState->camera.position.x;
  pAtmosphereRenderer->renderShader.fragParams.camera.y = (float)pProgramState->camera.position.y;
  pAtmosphereRenderer->renderShader.fragParams.camera.z = (float)pProgramState->camera.position.z;
  pAtmosphereRenderer->renderShader.fragParams.camera.w = (float)(use_luminance_ != NONE ? exposure_ * 1e-5 : exposure_);

  pAtmosphereRenderer->renderShader.fragParams.sun_direction.x = (float)(udCos(sun_azimuth_angle_radians_) * udSin(sun_zenith_angle_radians_));
  pAtmosphereRenderer->renderShader.fragParams.sun_direction.y = (float)(udSin(sun_azimuth_angle_radians_) * udSin(sun_zenith_angle_radians_));
  pAtmosphereRenderer->renderShader.fragParams.sun_direction.z = (float)udCos(sun_zenith_angle_radians_);

  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pSceneColour, 4, pAtmosphereRenderer->renderShader.uniform_sceneColour);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pSceneDepth, 5, pAtmosphereRenderer->renderShader.uniform_sceneDepth);

  vcShader_BindConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_vertParams, &pAtmosphereRenderer->renderShader.vertParams, sizeof(pAtmosphereRenderer->renderShader.vertParams));
  vcShader_BindConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_fragParams, &pAtmosphereRenderer->renderShader.fragParams, sizeof(pAtmosphereRenderer->renderShader.fragParams));

  //glBindVertexArray(full_screen_quad_vao_);
  //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
  glBindVertexArray(0);

  return result;
}

void vcAtmosphereRenderer_HandleFakeInput(vcAtmosphereRenderer *pAtmosphereRenderer)
{
  udUnused(pAtmosphereRenderer);
  int mouse_x = (int)ImGui::GetIO().MousePos.x;
  int mouse_y = (int)ImGui::GetIO().MousePos.y;

  if (ImGui::GetIO().MouseDown[0])
  {
    constexpr double kScale = 500.0;
    if (ImGui::GetIO().KeyCtrl) {
      sun_zenith_angle_radians_ -= (previous_mouse_y_ - mouse_y) / kScale;
      sun_zenith_angle_radians_ =
        udMax(0.0, udMin(kPi, sun_zenith_angle_radians_));
      sun_azimuth_angle_radians_ += (previous_mouse_x_ - mouse_x) / kScale;
    }
    else {
      view_zenith_angle_radians_ += (previous_mouse_y_ - mouse_y) / kScale;
      view_zenith_angle_radians_ =
        udMax(0.0, udMin(kPi / 2.0, view_zenith_angle_radians_));
      view_azimuth_angle_radians_ += (previous_mouse_x_ - mouse_x) / kScale;
    }
  }
  previous_mouse_x_ = mouse_x;
  previous_mouse_y_ = mouse_y;
}
