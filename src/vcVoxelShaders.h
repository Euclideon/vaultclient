#include "vdkPointCloud.h"
#include "vcModel.h"
#include "vcState.h"

struct vcUDRSData
{
  vcModel *pModel;
  vcState *pProgramData;
  uint32_t attributeOffset;

  union
  {
    struct
    {
      uint16_t minIntensity;
      uint16_t maxIntensity;
      float intensityRange;
    } intensity;
    struct
    {
      const uint32_t *pCustomClassificationColors;
    } classification;
    struct
    {
      float minBound;
      float maxBound;
      uint32_t errorColour;
      uint32_t outOfBoundsColour;
    } displacement;
  } data;
};

uint32_t vcVoxelShader_Black(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Colour(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Intensity(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Classification(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementAbs(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementSigned(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
