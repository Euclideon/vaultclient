#include "vcVoxelShaders.h"

inline uint32_t vcPCShaders_BuildAlpha(vcModel *pModel)
{
  return (pModel->m_selected ? 0x01000000 : 0x00000000);
}

uint32_t vcVoxelShader_Black(vdkPointCloud * /*pPointCloud*/, uint64_t /*voxelID*/, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  return vcPCShaders_BuildAlpha(pData->pModel) | 0x00000000;
}

uint32_t vcVoxelShader_Intensity(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  const uint16_t *pIntensity = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void**)&pIntensity);
  if (pIntensity != nullptr)
  {
    float value = udMax(float(*pIntensity - pData->data.intensity.minIntensity) / pData->data.intensity.intensityRange, 0.f);
    uint32_t channel = udMin((uint32_t)(udPow(value, 1) * 255.0), (uint32_t)255);
    result = (channel * 0x00010101);
  }

  return vcPCShaders_BuildAlpha(pData->pModel) | result;
}

uint32_t vcVoxelShader_DisplacementAbs(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &result);

  float *pDisplacement = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pDisplacement);

  if (pDisplacement != nullptr)
  {
    if (*pDisplacement == FLT_MAX)
      result = pData->data.displacement.errorColour;
    else
    {
      uint32_t red = 0;
      uint32_t green = 0;
      uint32_t blue = 0;

      const uint32_t nBits = 2;
      const uint32_t clrMask = 0xC0;

      float absDisplacement = udAbs(*pDisplacement);

      if ((absDisplacement < pData->data.displacement.minBound) || (absDisplacement > pData->data.displacement.maxBound ))
      {
        result = pData->data.displacement.outOfBoundsColour;
      }
      else
      {
        //TODO Color ranges can be user defined
        uint32_t nBounds = 3;
        uint32_t redMix[] = {0, 255, 255};
        uint32_t greenMix[] = {0, 0, 0};
        uint32_t blueMix[] = {255, 255, 0};

        float range = pData->data.displacement.maxBound - pData->data.displacement.minBound;
        for (uint32_t i = 0; i < nBounds - 1; ++i)
        {
          float frac = (absDisplacement - pData->data.displacement.minBound) / range;
          float upperBound = float(i + 1) / (nBounds - 1);
          if (frac <= upperBound)
          {
            float mix = (frac * (nBounds - 1) - i);
            red = uint32_t((float)redMix[i] + (redMix[i + 1] - redMix[i]) * mix);
            green = uint32_t((float)greenMix[i] + (greenMix[i + 1] - greenMix[i]) * mix);
            blue = uint32_t((float)blueMix[i] + (blueMix[i + 1] - blueMix[i]) * mix);
            break;
          }
        }

        result = (result >> nBits) & 0x3F3F3F;
        result = result | ((red & clrMask) << 16);
        result = result | ((green & clrMask) << 8);
        result = result | (blue & clrMask);
      }
    }
  }

  return vcPCShaders_BuildAlpha(pData->pModel) | (result & 0xFFFFFF);
}

uint32_t vcVoxelShader_DisplacementSigned(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &result);

  float *pDisplacement = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pDisplacement);

  if (pDisplacement != nullptr)
  {
    if (*pDisplacement == FLT_MAX)
      result = pData->data.displacement.errorColour;
    else
    {
      uint32_t red = 0;
      uint32_t green = 0;
      uint32_t blue = 0;

      const uint32_t nBits = 2;
      const uint32_t clrMask = 0xC0;

      if ((*pDisplacement < pData->data.displacement.minBound) || (*pDisplacement > pData->data.displacement.maxBound ))
      {
        result = pData->data.displacement.outOfBoundsColour;
      }
      else
      {
        uint32_t nBounds = 5;
        uint32_t redMix[] = {0, 0, 0, 255, 255};
        uint32_t greenMix[] = {255, 255, 0, 0, 0};
        uint32_t blueMix[] = {0, 255, 255, 255, 0};

        float range = pData->data.displacement.maxBound - pData->data.displacement.minBound;
        for (uint32_t i = 0; i < nBounds - 1; ++i)
        {
          float frac = (*pDisplacement - pData->data.displacement.minBound) / range;
          float upperBound = float(i + 1) / (nBounds - 1);
          if (frac <= upperBound)
          {
            float mix = (frac * (nBounds - 1) - i);
            red = uint32_t((float)redMix[i] + (redMix[i + 1] - redMix[i]) * mix);
            green = uint32_t((float)greenMix[i] + (greenMix[i + 1] - greenMix[i]) * mix);
            blue = uint32_t((float)blueMix[i] + (blueMix[i + 1] - blueMix[i]) * mix);
            break;
          }
        }

        result = (result >> nBits) & 0x3F3F3F;
        result = result | ((red & clrMask) << 16);
        result = result | ((green & clrMask) << 8);
        result = result | (blue & clrMask);
      }
    }
  }

  return vcPCShaders_BuildAlpha(pData->pModel) | (result & 0xFFFFFF);
}

uint32_t vcVoxelShader_Classification(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  uint8_t *pClassification = nullptr;

  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pClassification);
  if (pClassification)
    result = pData->data.classification.pCustomClassificationColors[*pClassification];

  //Apply the alpha to each component
  uint32_t a = (result >> 24) & 0xFF;
  uint32_t r = (result >> 16) & 0xFF;
  uint32_t g = (result >> 8) & 0xFF;
  uint32_t b = (result) & 0xFF;

  r = (r * a) >> 8;
  g = (g * a) >> 8;
  b = (b * a) >> 8;

  result = (a << 24) | (r << 16) | (g << 8) | b;

  return vcPCShaders_BuildAlpha(pData->pModel) | result;
}

uint32_t vcVoxelShader_Colour(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &result);

  return vcPCShaders_BuildAlpha(pData->pModel) | (0xffffff & result);
}
