/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*<h2>atmosphere/model.cc</h2>

<p>This file implements the <a href="model.h.html">API of our atmosphere
model</a>. Its main role is to precompute the transmittance, scattering and
irradiance textures. The GLSL functions to precompute them are provided in
<a href="functions.glsl.html">functions.glsl</a>, but they are not sufficient.
They must be used in fully functional shaders and programs, and these programs
must be called in the correct order, with the correct input and output textures
(via framebuffer objects), to precompute each scattering order in sequence, as
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. This is the role
of the following C++ code.
*/

#include "atmosphere/model.h"
#include "atmosphere/constants.h"

#include "udFile.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>


using namespace atmosphere;
/*
<p>The rest of this file is organized in 3 parts:
<ul>
<li>the <a href="#shaders">first part</a> defines the shaders used to precompute
the atmospheric textures,</li>
<li>the <a href="#utilities">second part</a> provides utility classes and
functions used to compile shaders, create textures, draw quads, etc,</li>
<li>the <a href="#implementation">third part</a> provides the actual
implementation of the <code>Model</code> class, using the above tools.</li>
</ul>

<h3 id="shaders">Shader definitions</h3>

<p>In order to precompute a texture we attach it to a framebuffer object (FBO)
and we render a full quad in this FBO. For this we need a basic vertex shader:
*/

namespace atmosphere {

namespace {
/*
<p>Finally, we need a utility function to compute the value of the conversion
constants *<code>_RADIANCE_TO_LUMINANCE</code>, used above to convert the
spectral results into luminance values. These are the constants k_r, k_g, k_b
described in Section 14.3 of <a href="https://arxiv.org/pdf/1612.04336.pdf">A
Qualitative and Quantitative Evaluation of 8 Clear Sky Models</a>.

<p>Computing their value requires an integral of a function times a CIE color
matching function. Thus, we first need functions to interpolate an arbitrary
function (specified by some samples), and a CIE color matching function
(specified by tabulated values), at an arbitrary wavelength. This is the purpose
of the following two functions:
*/

constexpr int kLambdaMin = 360;
constexpr int kLambdaMax = 830;

double CieColorMatchingFunctionTableValue(double wavelength, int column) {
  if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) {
    return 0.0;
  }
  double u = (wavelength - kLambdaMin) / 5.0;
  int row = static_cast<int>(std::floor(u));
  assert(row >= 0 && row + 1 < 95);
  assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
         CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
  u -= row;
  return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
      CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(
    const std::vector<double>& wavelengths,
    const std::vector<double>& wavelength_function,
    double wavelength) {
  assert(wavelength_function.size() == wavelengths.size());
  if (wavelength < wavelengths[0]) {
    return wavelength_function[0];
  }
  for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
    if (wavelength < wavelengths[i + 1]) {
      double u =
          (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
      return
          wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
    }
  }
  return wavelength_function[wavelength_function.size() - 1];
}


/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

// The returned constants are in lumen.nm / watt.
/*void ComputeSpectralRadianceToLuminanceFactors(
    const std::vector<double>& wavelengths,
    const std::vector<double>& solar_irradiance,
    double lambda_power, double* k_r, double* k_g, double* k_b) {
  *k_r = 0.0;
  *k_g = 0.0;
  *k_b = 0.0;
  double solar_r = Interpolate(wavelengths, solar_irradiance, Model::kLambdaR);
  double solar_g = Interpolate(wavelengths, solar_irradiance, Model::kLambdaG);
  double solar_b = Interpolate(wavelengths, solar_irradiance, Model::kLambdaB);
  int dlambda = 1;
  for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
    double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
    double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
    double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
    const double* xyz2srgb = XYZ_TO_SRGB;
    double r_bar =
        xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
    double g_bar =
        xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
    double b_bar =
        xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
    double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
    *k_r += r_bar * irradiance / solar_r *
        pow(lambda / Model::kLambdaR, lambda_power);
    *k_g += g_bar * irradiance / solar_g *
        pow(lambda / Model::kLambdaG, lambda_power);
    *k_b += b_bar * irradiance / solar_b *
        pow(lambda / Model::kLambdaB, lambda_power);
  }
  *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}
*/

}  // anonymous namespace

/*<h3 id="implementation">Model implementation</h3>

<p>Using the above utility functions and classes, we can now implement the
constructor of the <code>Model</code> class. This constructor generates a piece
of GLSL code that defines an <code>ATMOSPHERE</code> constant containing the
atmosphere parameters (we use constants instead of uniforms to enable constant
folding and propagation optimizations in the GLSL compiler), concatenated with
<a href="functions.glsl.html">functions.glsl</a>, and with
<code>kAtmosphereShader</code>, to get the shader exposed by our API in
<code>GetShader</code>. It also allocates the precomputed textures (but does not
initialize them), as well as a vertex buffer object to render a full screen quad
(used to render into the precomputed textures).
*/

Model::Model(
    //const std::vector<double>& wavelengths,
    //const std::vector<double>& solar_irradiance,
    //const double sun_angular_radius,
    //double bottom_radius,
    //double top_radius,
    //const std::vector<DensityProfileLayer>& rayleigh_density,
    //const std::vector<double>& rayleigh_scattering,
    //const std::vector<DensityProfileLayer>& mie_density,
    //const std::vector<double>& mie_scattering,
    //const std::vector<double>& mie_extinction,
    //double mie_phase_function_g,
    //const std::vector<DensityProfileLayer>& absorption_density,
    //const std::vector<double>& absorption_extinction,
    //const std::vector<double>& ground_albedo,
    //double max_sun_zenith_angle,
    //double length_unit_in_meters,
    //unsigned int num_precomputed_wavelengths,
    //bool combine_scattering_textures,
    //bool half_precision
)
{

  //udUnused(wavelengths);
  //udUnused(solar_irradiance);
  //udUnused(sun_angular_radius);
  //udUnused(bottom_radius);
  //udUnused(top_radius);
  //udUnused(rayleigh_density);
  //udUnused(rayleigh_scattering);
  //udUnused(mie_density);
  //udUnused(mie_scattering);
  //udUnused(mie_extinction);
  //udUnused(mie_phase_function_g);
  //udUnused(absorption_density);
  //udUnused(absorption_extinction);
  //udUnused(ground_albedo);
  //udUnused(max_sun_zenith_angle);
  //udUnused(length_unit_in_meters);
  //udUnused(combine_scattering_textures);
  //udUnused(half_precision);
  //udUnused(num_precomputed_wavelengths);

  // Allocate the precomputed textures, but don't precompute them yet.
  vcTexture_CreateAdv(&pTransmittance_texture_, vcTextureType_Texture2D, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1, nullptr, vcTextureFormat_RGBA32F, vcTFM_Linear, false, vcTWM_Clamp, vcTCF_Dynamic);
  vcTexture_CreateAdv(&pIrradiance_texture_, vcTextureType_Texture2D, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1, nullptr, vcTextureFormat_RGBA32F, vcTFM_Linear, false, vcTWM_Clamp, vcTCF_Dynamic);
  vcTexture_CreateAdv(&pScattering_texture_, vcTextureType_Texture3D, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, nullptr, vcTextureFormat_RGBA16F, vcTFM_Linear, false, vcTWM_Clamp, vcTCF_Dynamic);
}

/*
<p>The destructor is trivial:
*/

Model::~Model() {
  vcTexture_Destroy(&pTransmittance_texture_);
  vcTexture_Destroy(&pIrradiance_texture_);
  vcTexture_Destroy(&pScattering_texture_);

  udFree(pTransmittancePixels);
  udFree(pIrradiancePixels);
  udFree(pScatteringPixels);
}

void Model::UploadPrecomputedTextures()
{
  vcTexture_UploadPixels(pTransmittance_texture_, pTransmittancePixels, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1);
  udFree(pTransmittancePixels);

  vcTexture_UploadPixels(pIrradiance_texture_, pIrradiancePixels, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1);
  udFree(pIrradiancePixels);

  vcTexture_UploadPixels(pScattering_texture_, pScatteringPixels, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
  udFree(pScatteringPixels);
}

bool Model::LoadPrecomputedTextures()
{
  // Load raw textures, and convert from RGB32F to RGBA32F where necessary
  float *pRawPixels = nullptr;

  // transmittance
  if (udFile_Load("asset://assets/data/transmittance.dat", &pRawPixels) != udR_Success)
    return false;
  pTransmittancePixels = udAllocType(float, TRANSMITTANCE_TEXTURE_WIDTH * TRANSMITTANCE_TEXTURE_HEIGHT * 4, udAF_Zero);
  // manually convert from RGB32F to RGBA32F
  for (int y = 0; y < TRANSMITTANCE_TEXTURE_HEIGHT; ++y)
  {
    for (int x = 0; x < TRANSMITTANCE_TEXTURE_WIDTH; ++x)
    {
      int index = (y * TRANSMITTANCE_TEXTURE_WIDTH) + x;
      float *pPixel = pRawPixels + index * 3;
      float r = pPixel[0];
      float g = pPixel[1];
      float b = pPixel[2];

      pTransmittancePixels[index * 4 + 0] = r;
      pTransmittancePixels[index * 4 + 1] = g;
      pTransmittancePixels[index * 4 + 2] = b;
    }
  }
  udFree(pRawPixels);

  // irradiance
  if (udFile_Load("asset://assets/data/irradiance.dat", &pRawPixels) != udR_Success)
    return false;
  pIrradiancePixels = udAllocType(float, IRRADIANCE_TEXTURE_WIDTH * IRRADIANCE_TEXTURE_HEIGHT * 4, udAF_Zero);
  // manually convert from RGB32F to RGBA32F
  for (int y = 0; y < IRRADIANCE_TEXTURE_HEIGHT; ++y)
  {
    for (int x = 0; x < IRRADIANCE_TEXTURE_WIDTH; ++x)
    {
      int index = (y * IRRADIANCE_TEXTURE_WIDTH) + x;
      float *pPixel = pRawPixels + index * 3;
      float r = pPixel[0];
      float g = pPixel[1];
      float b = pPixel[2];

      pIrradiancePixels[index * 4 + 0] = r;
      pIrradiancePixels[index * 4 + 1] = g;
      pIrradiancePixels[index * 4 + 2] = b;
    }
  }
  udFree(pRawPixels);

  // scattering
  if (udFile_Load("asset://assets/data/scattering.dat", &pScatteringPixels) != udR_Success)
    return false;
  
  return true;
} 

/*
<p>The utility method <code>ConvertSpectrumToLinearSrgb</code> is implemented
with a simple numerical integration of the given function, times the CIE color
matching funtions (with an integration step of 1nm), followed by a matrix
multiplication:
*/


void Model::ConvertSpectrumToLinearSrgb(
    const std::vector<double>& wavelengths,
    const std::vector<double>& spectrum,
    double* r, double* g, double* b) {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  const int dlambda = 1;
  for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
    double value = Interpolate(wavelengths, spectrum, lambda);
    x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
    y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
    z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
  }
  *r = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
  *g = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
  *b = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}


}  // namespace atmosphere
