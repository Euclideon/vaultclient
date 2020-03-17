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

inline uint32_t fadeAlpha(uint32_t firstColour, uint32_t secondColour)
{
  
  uint32_t alphaMask = 0xFF << 24;

  float  alpha = (float)((alphaMask & firstColour) >> 24);
  alpha/=255.f;
  uint32_t result = 0;
  //extract the each colour and scale it by the alpha in the first channel
  for (int shift = 0; shift < 24; shift += 8)
  {
    float scaledChannel = alpha * (float)((firstColour >> shift) & 0xFF);
    scaledChannel += (1 - alpha) * (float)((secondColour >> shift) & 0xFF);
    result |= ((uint32_t) scaledChannel) << shift;
  }

  return result;
}

uint32_t vcVoxelShader_Displacement(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  uint32_t baseColour = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &baseColour);

  float *pDisplacement = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pDisplacement);
  pData->data.displacement.errorColour = 0xFF | 128<<24;
  pData->data.displacement.minColour = 0xFF<<8 ;
  pData->data.displacement.maxColour = 0xFF<<16 | 128 <<24;

  if (pDisplacement != nullptr)
  {
    
    if (*pDisplacement == FLT_MAX)
      result = pData->data.displacement.errorColour;
    else if (*pDisplacement <= pData->data.displacement.minThreshold)
      result = pData->data.displacement.minColour;
    else if (*pDisplacement >= pData->data.displacement.maxThreshold)
      result = pData->data.displacement.maxColour;
    else
      result = pData->data.displacement.midColour;
  }

  result = fadeAlpha(result, baseColour);
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
