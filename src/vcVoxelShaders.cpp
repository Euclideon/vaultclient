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

/*
Blends two colours based on the contents of the first colours alpha channel
*/
inline uint32_t vcVoxelShader_FadeAlpha(uint32_t firstColour, uint32_t secondColour)
{
  float  alpha = (float)(firstColour>>24 & 0xFF)/255.0f;
  uint32_t result = 0;
  //extract the each colour and scale it by the alpha in the first channel
  for (int shift = 0; shift < 24; shift += 8)
  {
    float scaledChannel = alpha * (float)((firstColour >> shift) & 0xFF);//scale the first colour by alpha
    scaledChannel += (1 - alpha) * (float)((secondColour >> shift) & 0xFF);//scale the second by 1-alpha
    result |= ((uint32_t) scaledChannel) << shift;
  }
  return result;
}

uint32_t vcVoxelShader_DisplacementDistance(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t displacementColour = 0;
  uint32_t baseColour = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &baseColour);

  float *pDisplacement = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pDisplacement);

  if (pDisplacement != nullptr)
  {
    if (*pDisplacement == FLT_MAX)
      displacementColour = pData->data.displacement.errorColour;
    else if (*pDisplacement <= pData->data.displacement.minThreshold)
      displacementColour = pData->data.displacement.minColour;
    else if (*pDisplacement >= pData->data.displacement.maxThreshold)
      displacementColour = pData->data.displacement.maxColour;
    else
      displacementColour = pData->data.displacement.midColour;
  }

  displacementColour = vcVoxelShader_FadeAlpha(displacementColour, baseColour);
  return vcPCShaders_BuildAlpha(pData->pModel) | (displacementColour & 0xFFFFFF);
}

uint32_t vcVoxelShader_DisplacementDirection(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t displacementColour = 0;
  uint32_t baseColour = 0;
  vdkPointCloud_GetNodeColour(pPointCloud, voxelID, &baseColour);

  float *pDisplacement = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pDisplacement);

  float *pDisplacementDirections[3];
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacement.attributeOffsets[0], (const void **)&pDisplacementDirections[0]);
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacement.attributeOffsets[1], (const void **)&pDisplacementDirections[1]);
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacement.attributeOffsets[2], (const void **)&pDisplacementDirections[2]);

  if (pDisplacement != nullptr && pDisplacementDirections[0] != nullptr && pDisplacementDirections[1] != nullptr && pDisplacementDirections[2] != nullptr)
  {
    udFloat3 direction = udFloat3::create(*pDisplacementDirections[0], *pDisplacementDirections[1], *pDisplacementDirections[2]);

    int32_t red = (int32_t)((*pDisplacementDirections[0] / 2.f + 0.5f) * 255);
    int32_t green = (int32_t)((*pDisplacementDirections[1] / 2.f + 0.5f) * 255);
    int32_t blue = (int32_t)((*pDisplacementDirections[2] / 2.f + 0.5f) * 255);

    displacementColour = 0xFF000000 | ((red << 16) | (green << 8) | blue);

    //if (*pDisplacement == FLT_MAX)
    //  displacementColour = pData->data.displacement.errorColour;
    //else if (*pDisplacement <= pData->data.displacement.minThreshold)
    //  displacementColour = pData->data.displacement.minColour;
    //else if (*pDisplacement >= pData->data.displacement.maxThreshold)
    //  displacementColour = pData->data.displacement.maxColour;
    //else
    //  displacementColour = pData->data.displacement.midColour;
  }

  displacementColour = vcVoxelShader_FadeAlpha(displacementColour, baseColour);
  return vcPCShaders_BuildAlpha(pData->pModel) | (displacementColour & 0xFFFFFF);
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
