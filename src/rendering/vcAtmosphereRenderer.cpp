#include "vcAtmosphereRenderer.h"
#include "vcState.h"
#include "vcInternalModels.h"

#include "vcShader.h"

#include "udGeoZone.h"
#include "udMath.h"
#include "udPlatform.h"
#include "udStringUtil.h"

#include "vcMesh.h"

#include "atmosphere/model.h"

enum Luminance
{
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

struct vcAtmosphereRenderer
{
  atmosphere::Model *pModel;

  // some model params
  Luminance use_luminance;
  udDouble3 sunDirection;
  double exposure;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_transmittance;
    vcShaderSampler *uniform_scattering;
    vcShaderSampler *uniform_irradiance;
    vcShaderSampler *uniform_sceneColour;
    vcShaderSampler *uniform_sceneNormal;
    vcShaderSampler *uniform_sceneDepth;
    vcShaderConstantBuffer *uniform_vertParams;
    vcShaderConstantBuffer *uniform_fragParams;

    struct
    {
      udFloat4 camera; // (camera position).xyz, exposure.w
      udFloat4 whitePoint; // w unused
      udFloat4 earthCenter; // w unused
      udFloat4 sunDirection;// w unused
      udFloat4 sunSize; //zw unused
    } fragParams;

    struct
    {
      udFloat4x4 modelFromView;
      udFloat4x4 viewFromClip;
    } vertParams;

  } renderShader;
};

constexpr double kSunAngularRadius = 0.00935 / 2.0;
constexpr double kLengthUnitInMeters = 1.0;

void vcAtmosphere_AsyncLoadWorkerThreadWork(void *pLoadInfo)
{
  vcAtmosphereRenderer *pAtmosphereRenderer = (vcAtmosphereRenderer*)pLoadInfo;
  pAtmosphereRenderer->pModel->LoadPrecomputedTextures();
}

void vcAtmosphere_AsyncLoadMainThreadWork(void *pLoadInfo)
{
  vcAtmosphereRenderer *pAtmosphereRenderer = (vcAtmosphereRenderer*)pLoadInfo;
  pAtmosphereRenderer->pModel->UploadPrecomputedTextures();
}

udResult vcAtmosphereRenderer_Create(vcAtmosphereRenderer **ppAtmosphereRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;
  vcAtmosphereRenderer *pAtmosphereRenderer = nullptr;

  bool use_constant_solar_spectrum_ = false;
  //bool use_ozone_ = true;
  //bool use_half_precision_ = true;
  ////bool use_combined_textures_ = true;
  bool do_white_balance_ = false;
  //
  double white_point_r = 1.0;
  double white_point_g = 1.0;
  double white_point_b = 1.0;
  //
  //// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
  //// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
  //// summed and averaged in each bin (e.g. the value for 360nm is the average
  //// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
  //// Values in W.m^-2.
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
  //// Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
  //// referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
  //// each bin (e.g. the value for 360nm is the average of the original values
  //// for all wavelengths between 360 and 370nm). Values in m^2.
  //constexpr double kOzoneCrossSection[48] = {
  //  1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
  //  8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
  //  1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
  //  4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
  //  2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
  //  6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
  //  2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
  //};
  //// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
  //constexpr double kDobsonUnit = 2.687e20;
  //// Maximum number density of ozone molecules, in m^-3 (computed so at to get
  //// 300 Dobson units of ozone - for this we divide 300 DU by the integral of
  //// the ozone density profile defined below, which is equal to 15km).
  //constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
  //// Wavelength independent solar irradiance "spectrum" (not physically
  //// realistic, but was used in the original implementation).
  constexpr double kConstantSolarIrradiance = 1.5;
  //constexpr double kBottomRadius = 0.0;//6378137.0;
  //constexpr double kTopRadius = 0.0;//kBottomRadius + 60000;
  //constexpr double kRayleigh = 1.24062e-6;
  //constexpr double kRayleighScaleHeight = 8000.0;
  //constexpr double kMieScaleHeight = 1200.0;
  //constexpr double kMieAngstromAlpha = 0.0;
  //constexpr double kMieAngstromBeta = 5.328e-3;
  //constexpr double kMieSingleScatteringAlbedo = 0.9;
  //constexpr double kMiePhaseFunctionG = 0.8;
  //constexpr double kGroundAlbedo = 0.1;
  //const double max_sun_zenith_angle =
  //  (use_half_precision_ ? 102.0 : 120.0) / 180.0 * UD_PI;
  //
  //atmosphere::DensityProfileLayer
  //  rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
  //atmosphere::DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
  //// Density profile increasing linearly from 0 to 1 between 10 and 25km, and
  //// decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
  //// profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
  //// Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
  //std::vector<atmosphere::DensityProfileLayer> ozone_density;
  //ozone_density.push_back(
  //  atmosphere::DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
  //ozone_density.push_back(
  //  atmosphere::DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));
  //
  std::vector<double> wavelengths;
  std::vector<double> solar_irradiance;
  //std::vector<double> rayleigh_scattering;
  //std::vector<double> mie_scattering;
  //std::vector<double> mie_extinction;
  //std::vector<double> absorption_extinction;
  //std::vector<double> ground_albedo;
  for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
    //double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
    //double mie =
    //  kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
    wavelengths.push_back(l);
    if (use_constant_solar_spectrum_) {
      solar_irradiance.push_back(kConstantSolarIrradiance);
    }
    else {
      solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
    }
    //rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
    //mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
    //mie_extinction.push_back(mie);
    //absorption_extinction.push_back(use_ozone_ ?
    //  kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] :
    //  0.0);
    //ground_albedo.push_back(kGroundAlbedo);
  }

  UD_ERROR_NULL(ppAtmosphereRenderer, udR_InvalidParameter_);

  pAtmosphereRenderer = udAllocType(vcAtmosphereRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pAtmosphereRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcAtmosphereRenderer_ReloadShaders(pAtmosphereRenderer, pWorkerPool));

  // some defaults
  pAtmosphereRenderer->use_luminance = NONE;
  pAtmosphereRenderer->sunDirection = udDouble3::create(0, 0, 1);
  pAtmosphereRenderer->exposure = 10.0;

  pAtmosphereRenderer->pModel = new atmosphere::Model();
  //pAtmosphereRenderer->pModel = new atmosphere::Model(wavelengths, solar_irradiance, kSunAngularRadius,
  //  kBottomRadius, kTopRadius, { rayleigh_layer }, rayleigh_scattering,
  //  { mie_layer }, mie_scattering, mie_extinction, kMiePhaseFunctionG,
  //  ozone_density, absorption_extinction, ground_albedo, max_sun_zenith_angle,
  //  kLengthUnitInMeters, pAtmosphereRenderer->use_luminance == PRECOMPUTED ? 15 : 3,
  //  true, use_half_precision_);


  udWorkerPool_AddTask(pWorkerPool, vcAtmosphere_AsyncLoadWorkerThreadWork, pAtmosphereRenderer, false, vcAtmosphere_AsyncLoadMainThreadWork);

  if (do_white_balance_) {
    atmosphere::Model::ConvertSpectrumToLinearSrgb(wavelengths, solar_irradiance,
      &white_point_r, &white_point_g, &white_point_b);
    double white_point = (white_point_r + white_point_g + white_point_b) / 3.0;
    white_point_r /= white_point;
    white_point_g /= white_point;
    white_point_b /= white_point;
  }

  pAtmosphereRenderer->renderShader.fragParams.whitePoint.x = (float)white_point_r;
  pAtmosphereRenderer->renderShader.fragParams.whitePoint.y = (float)white_point_g;
  pAtmosphereRenderer->renderShader.fragParams.whitePoint.z = (float)white_point_b;
  pAtmosphereRenderer->renderShader.fragParams.sunSize.x = (float)udTan(kSunAngularRadius);
  pAtmosphereRenderer->renderShader.fragParams.sunSize.y = (float)udCos(kSunAngularRadius);

  *ppAtmosphereRenderer = pAtmosphereRenderer;
  result = udR_Success;
epilogue:

  return result;
}

void vcAtmosphereRenderer_DestroyShaders(vcAtmosphereRenderer *pAtmosphereRenderer)
{
  vcShader_ReleaseConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_vertParams);
  vcShader_ReleaseConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_fragParams);
  vcShader_DestroyShader(&pAtmosphereRenderer->renderShader.pProgram);
}

udResult vcAtmosphereRenderer_Destroy(vcAtmosphereRenderer **ppAtmosphereRenderer)
{
  if (ppAtmosphereRenderer == nullptr || *ppAtmosphereRenderer == nullptr)
    return udR_InvalidParameter_;

  vcAtmosphereRenderer_DestroyShaders(*ppAtmosphereRenderer);
  delete (*ppAtmosphereRenderer)->pModel;
  udFree((*ppAtmosphereRenderer));

  return udR_Success;
}

udResult vcAtmosphereRenderer_ReloadShaders(vcAtmosphereRenderer *pAtmosphereRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcAtmosphereRenderer_DestroyShaders(pAtmosphereRenderer);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pAtmosphereRenderer->renderShader.pProgram, pWorkerPool, "asset://assets/shaders/atmosphereVertexShader", "asset://assets/shaders/atmosphereFragmentShader", vcP3UV2VertexLayout,
    [pAtmosphereRenderer](void *)
    {
      vcShader_Bind(pAtmosphereRenderer->renderShader.pProgram);
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_transmittance, pAtmosphereRenderer->renderShader.pProgram, "transmittance");
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_scattering, pAtmosphereRenderer->renderShader.pProgram, "scattering");
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_irradiance, pAtmosphereRenderer->renderShader.pProgram, "irradiance");
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_sceneColour, pAtmosphereRenderer->renderShader.pProgram, "sceneColour");
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_sceneNormal, pAtmosphereRenderer->renderShader.pProgram, "sceneNormal");
      vcShader_GetSamplerIndex(&pAtmosphereRenderer->renderShader.uniform_sceneDepth, pAtmosphereRenderer->renderShader.pProgram, "sceneDepth");
      vcShader_GetConstantBuffer(&pAtmosphereRenderer->renderShader.uniform_vertParams, pAtmosphereRenderer->renderShader.pProgram, "u_vertParams", sizeof(pAtmosphereRenderer->renderShader.vertParams));
      vcShader_GetConstantBuffer(&pAtmosphereRenderer->renderShader.uniform_fragParams, pAtmosphereRenderer->renderShader.pProgram, "u_fragParams", sizeof(pAtmosphereRenderer->renderShader.fragParams));
    }
  ), udR_InternalError);

  result = udR_Success;
epilogue:

  return result;
}

void vcAtmosphereRenderer_SetVisualParams(vcState *pProgramState, vcAtmosphereRenderer *pAtmosphereRenderer)
{
  pAtmosphereRenderer->exposure = pProgramState->settings.presentation.skybox.exposure;

  //At solar noon the hour angle is 0.000 degree, with the time before solar noon expressed as negative degrees, and the local time after solar noon expressed as positive degrees.
  //For example, at 10:30 AM local apparent time the hour angle is -22.5° (15° per hour times 1.5 hours before noon).

  const double DaysPerYear = 365.25;
  const double BaseTime = 7305.0; // 2020-01-01:00:00:00UTC in MJD2000

  double terrestrialDateJ2000 = 0.0;

  double hourAngleRadians = UD_DEG2RAD((pProgramState->settings.presentation.skybox.timeOfDay) * 15.0);
  double yearNormalized = (pProgramState->settings.presentation.skybox.month / 12.0);

  if (pProgramState->settings.presentation.skybox.useLiveTime)
  {
    terrestrialDateJ2000 = (udGetEpochSecsUTCf() / 86400.0) - 10957.0;
  }
  else if (pProgramState->settings.presentation.skybox.keepSameTime && pProgramState->geozone.projection != udGZPT_Unknown)
  {
    udDouble3 latLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->camera.position);
    hourAngleRadians -= UD_DEG2RAD(latLong.y);

    terrestrialDateJ2000 = BaseTime + hourAngleRadians / UD_2PI + yearNormalized * DaysPerYear;
  }
  else // Only use setting
  {
    terrestrialDateJ2000 = BaseTime + hourAngleRadians / UD_2PI + yearNormalized * DaysPerYear;
  }

  double terrestrialYearJ2000 = terrestrialDateJ2000 / DaysPerYear;

  yearNormalized = terrestrialYearJ2000 - int(terrestrialYearJ2000);
  hourAngleRadians = (terrestrialDateJ2000 - int(terrestrialDateJ2000) + yearNormalized) * UD_2PI;

  double meanSunLongitudeDegrees = 280.460 + 0.9856474 * terrestrialDateJ2000; // Already corrected for aberration
  double meanSunAnomalyRadians = UD_DEG2RAD(357.528 + 0.9856003 * terrestrialDateJ2000);

  while (meanSunLongitudeDegrees < 0.0)
    meanSunLongitudeDegrees += 360.0;

  while (meanSunLongitudeDegrees > 360.0)
    meanSunLongitudeDegrees -= 360.0;

  while (meanSunAnomalyRadians < 0.0)
    meanSunAnomalyRadians += UD_2PI;

  while (meanSunAnomalyRadians > UD_2PI)
    meanSunAnomalyRadians -= UD_2PI;

  // The Ecliptic Latitude is always ~0.0;
  double eclipticLongitudeRadians = UD_DEG2RAD(meanSunLongitudeDegrees + 1.915 * udSin(meanSunAnomalyRadians) + 0.02 * udSin(2 * meanSunAnomalyRadians));
  double sunDistAU = 1.00014 - 0.01671 * udCos(meanSunAnomalyRadians) - 0.00014 * udCos(2 * meanSunAnomalyRadians);

  double eclipticObliquityRadians = UD_DEG2RAD(23.45229 - 0.0130125 * terrestrialYearJ2000 - 0.000001638889 * terrestrialYearJ2000 * terrestrialYearJ2000 + 5.027778e-7 * terrestrialYearJ2000 * terrestrialYearJ2000 * terrestrialYearJ2000);

  pAtmosphereRenderer->sunDirection.x = sunDistAU * udCos(eclipticLongitudeRadians);
  pAtmosphereRenderer->sunDirection.y = sunDistAU * udCos(eclipticObliquityRadians) * udSin(eclipticLongitudeRadians);
  pAtmosphereRenderer->sunDirection.z = sunDistAU * udSin(eclipticObliquityRadians) * udSin(eclipticLongitudeRadians);

  pAtmosphereRenderer->sunDirection = udDoubleQuat::create({ 0, 0, 1 }, -yearNormalized * UD_2PI - hourAngleRadians).apply(pAtmosphereRenderer->sunDirection);

  if (pProgramState->geozone.projection == udGZPT_Unknown)
  {
    pAtmosphereRenderer->sunDirection = { pAtmosphereRenderer->sunDirection.z, pAtmosphereRenderer->sunDirection.x, pAtmosphereRenderer->sunDirection.y };
  }
  else if (pProgramState->geozone.projection >= udGZPT_TransverseMercator)
  {
    udGeoZone zone = {};
    udGeoZone_SetFromSRID(&zone, 4978);

    udDouble3 camPos = udGeoZone_TransformPoint(pProgramState->camera.position, pProgramState->geozone, zone);

    pAtmosphereRenderer->sunDirection = udGeoZone_TransformPoint(camPos + pAtmosphereRenderer->sunDirection, zone, pProgramState->geozone) - pProgramState->camera.position;
  }

  pAtmosphereRenderer->sunDirection = udNormalize(pAtmosphereRenderer->sunDirection);

  // TEMP HACK, until atmosphere shader paras are accessible to voxel shaders
  extern udFloat3 g_globalSunDirection;
  g_globalSunDirection = udFloat3::create(pAtmosphereRenderer->sunDirection);
}

bool vcAtmosphereRenderer_Render(vcAtmosphereRenderer *pAtmosphereRenderer, vcState *pProgramState, vcTexture *pSceneColour, vcTexture *pSceneNormal, vcTexture *pSceneDepth)
{
  bool result = true;

  udDouble3 earthCenter = pProgramState->camera.position;
  if (pProgramState->geozone.projection == udGZPT_Unknown || pProgramState->geozone.projection >= udGZPT_TransverseMercator)
  {
    if (pProgramState->geozone.projection == udGZPT_Unknown)
      earthCenter.z = -6378137;
    else
      earthCenter.z = -pProgramState->geozone.semiMajorAxis;
  }
  else if (pProgramState->geozone.projection == udGZPT_ECEF)
  {
    earthCenter = udDouble3::zero();
  }
  else
  {
    udGeoZone destZone = {};
    udGeoZone_SetFromSRID(&destZone, 4978);
    earthCenter = udGeoZone_TransformPoint(pProgramState->camera.position, pProgramState->geozone, destZone);
  }

  earthCenter += pProgramState->camera.cameraUp * pProgramState->settings.maptiles.mapHeight;

  // calculate the earth radius at in this zone
  udDouble3 cameraPositionInLongLat = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->camera.position);
  cameraPositionInLongLat.z = 0.0;
  udDouble3 pointOnAltitudeZero = udGeoZone_LatLongToCartesian(pProgramState->geozone, cameraPositionInLongLat);
  double earthRadius = udClamp(udMag3(pointOnAltitudeZero), pProgramState->geozone.semiMinorAxis, pProgramState->geozone.semiMajorAxis);

  earthCenter.z /= kLengthUnitInMeters;

  // This commented code is a WIP - working on doing some calculations in eye space
  udDouble4 earthCenterEyePos = udDouble4::create(earthCenter, 1.0);//pProgramState->camera.matrices.view *udDouble4::create(earthCenter, 1.0);

  udFloat4x4 inverseProjection = udFloat4x4::create(udInverse(pProgramState->camera.matrices.projection));
  udFloat4x4 inverseView = udFloat4x4::create(udInverse(pProgramState->camera.matrices.view));

  vcShader_Bind(pAtmosphereRenderer->renderShader.pProgram);
  pAtmosphereRenderer->renderShader.vertParams.viewFromClip = inverseProjection;
  pAtmosphereRenderer->renderShader.vertParams.modelFromView = inverseView;

  pAtmosphereRenderer->renderShader.fragParams.earthCenter.x = (float)earthCenterEyePos.x;
  pAtmosphereRenderer->renderShader.fragParams.earthCenter.y = (float)earthCenterEyePos.y;
  pAtmosphereRenderer->renderShader.fragParams.earthCenter.z = (float)earthCenterEyePos.z;
  pAtmosphereRenderer->renderShader.fragParams.earthCenter.w = (float)earthRadius;
  pAtmosphereRenderer->renderShader.fragParams.camera.x = (float)pProgramState->camera.position.x;
  pAtmosphereRenderer->renderShader.fragParams.camera.y = (float)pProgramState->camera.position.y;
  pAtmosphereRenderer->renderShader.fragParams.camera.z = (float)pProgramState->camera.position.z;
  pAtmosphereRenderer->renderShader.fragParams.camera.w = (float)(pAtmosphereRenderer->use_luminance != NONE ? pAtmosphereRenderer->exposure * 1e-5 : pAtmosphereRenderer->exposure);

  pAtmosphereRenderer->renderShader.fragParams.sunDirection = udFloat4::create(udFloat3::create(pAtmosphereRenderer->sunDirection), 0);

  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->pModel->pTransmittance_texture_, 0, pAtmosphereRenderer->renderShader.uniform_transmittance);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->pModel->pScattering_texture_, 1, pAtmosphereRenderer->renderShader.uniform_scattering);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->pModel->pIrradiance_texture_, 2, pAtmosphereRenderer->renderShader.uniform_irradiance);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pSceneColour, 3, pAtmosphereRenderer->renderShader.uniform_sceneColour);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pSceneNormal, 4, pAtmosphereRenderer->renderShader.uniform_sceneNormal);
  vcShader_BindTexture(pAtmosphereRenderer->renderShader.pProgram, pSceneDepth, 5, pAtmosphereRenderer->renderShader.uniform_sceneDepth);

  vcShader_BindConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_vertParams, &pAtmosphereRenderer->renderShader.vertParams, sizeof(pAtmosphereRenderer->renderShader.vertParams));
  vcShader_BindConstantBuffer(pAtmosphereRenderer->renderShader.pProgram, pAtmosphereRenderer->renderShader.uniform_fragParams, &pAtmosphereRenderer->renderShader.fragParams, sizeof(pAtmosphereRenderer->renderShader.fragParams));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
  return result;
}
