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
      displacementColour = pData->data.displacementAmount.errorColour;
    else if (*pDisplacement <= pData->data.displacementAmount.minThreshold)
      displacementColour = pData->data.displacementAmount.minColour;
    else if (*pDisplacement >= pData->data.displacementAmount.maxThreshold)
      displacementColour = pData->data.displacementAmount.maxColour;
    else
      displacementColour = pData->data.displacementAmount.midColour;
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
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacementDirection.attributeOffsets[0], (const void **)&pDisplacementDirections[0]);
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacementDirection.attributeOffsets[1], (const void **)&pDisplacementDirections[1]);
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->data.displacementDirection.attributeOffsets[2], (const void **)&pDisplacementDirections[2]);

  if (pDisplacement != nullptr && pDisplacementDirections[0] != nullptr && pDisplacementDirections[1] != nullptr && pDisplacementDirections[2] != nullptr)
  {
    udDouble3 direction = udDouble3::create(*pDisplacementDirections[0], *pDisplacementDirections[1], *pDisplacementDirections[2]);
    if (direction != udDouble3::zero())
    {
      double amount = udDot(direction, pData->data.displacementDirection.cameraDirection);

      if (amount >= 0)
        displacementColour = pData->data.displacementDirection.posColour;
      else
        displacementColour = pData->data.displacementDirection.negColour;

      for (int i = 0; i < 3; ++i)
      {
        uint32_t shift = i * 8;
        uint32_t mask = ~(0xFF << shift);
        uint32_t val = (uint32_t(((displacementColour >> shift) & 0xFF) * udMin(*pDisplacement, 1.f) * 255) << shift);
        displacementColour = (displacementColour & mask) | val;
      }

      displacementColour = vcVoxelShader_FadeAlpha(displacementColour, baseColour);
    }
    else
    {
      displacementColour = baseColour;
    }
  }

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

uint32_t vcVoxelShader_GPSTime(vdkPointCloud * pPointCloud, uint64_t voxelID, const void * pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  const double *pGPSTime = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void**)&pGPSTime);
  if (pGPSTime != nullptr)
  {
    if (*pGPSTime < pData->data.GPSTime.minTime)
      result = 0xFF000000;
    else if (*pGPSTime > pData->data.GPSTime.maxTime)
      result = 0xFFFFFFFF;
    else
    {
      double frac = (*pGPSTime - pData->data.GPSTime.minTime) * pData->data.GPSTime.mult;
      uint32_t channel = uint32_t(255.0 * frac);
      result = 0xFF000000 | (channel << 0) | (channel << 8) | (channel << 16);
    }
  }

  return vcPCShaders_BuildAlpha(pData->pModel) | result;
}

uint32_t vcVoxelShader_ScanAngle(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  int16_t *pScanAngle = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pScanAngle);

  if (pScanAngle == nullptr)
    goto epilogue;

  if (*pScanAngle < pData->data.scanAngle.minAngle)
    result = 0xFF000000;
  else if (*pScanAngle > pData->data.scanAngle.maxAngle)
    result = 0xFFFFFFFF;
  else
  {
    uint32_t scanAngle = uint32_t(int32_t(*pScanAngle) - pData->data.scanAngle.minAngle);
    uint32_t frac = (scanAngle << 16) / pData->data.scanAngle.range; //fixed point value where pivot at bit 16
    uint32_t channel = (255 * frac) >> 16;
    result = 0xFF000000 | (channel << 0) | (channel << 8) | (channel << 16);
  }

epilogue:
  return vcPCShaders_BuildAlpha(pData->pModel) | (0xffffff & result);
}

uint32_t vcVoxelShader_PointSourceID(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = pData->data.pointSourceID.defaultColour;
  uint16_t *pID = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pID);

  if (pID != nullptr)
  {
    for (size_t i = 0; i < pData->data.pointSourceID.pColourMap->length; ++i)
    {
      if (*pID == pData->data.pointSourceID.pColourMap->operator[](i).id)
      {
        result = pData->data.pointSourceID.pColourMap->operator[](i).colour;
        break;
      }
    }
  }

  return vcPCShaders_BuildAlpha(pData->pModel) | (0xffffff & result);
}

uint32_t vcVoxelShader_ReturnNumber(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  uint8_t *pNumber = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pNumber);

  if (pNumber != nullptr && *pNumber <= vcVisualizationSettings::s_maxReturnNumbers)
    result = pData->data.returnNumber.pColours[*pNumber];

  return vcPCShaders_BuildAlpha(pData->pModel) | (0xffffff & result);
}

uint32_t vcVoxelShader_NumberOfReturns(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData)
{
  vcUDRSData *pData = (vcUDRSData *)pUserData;

  uint32_t result = 0;
  uint8_t *pNumber = nullptr;
  vdkPointCloud_GetAttributeAddress(pPointCloud, voxelID, pData->attributeOffset, (const void **)&pNumber);

  if (pNumber != nullptr)
    result = pData->data.returnNumber.pColours[*pNumber];

  return vcPCShaders_BuildAlpha(pData->pModel) | (0xffffff & result);
}
