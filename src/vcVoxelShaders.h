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
      float minThreshold;
      float maxThreshold;
      uint32_t minColour;
      uint32_t maxColour;
      uint32_t midColour;
      uint32_t errorColour;
    } displacement;
  } data;
};

uint32_t vcVoxelShader_Black(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Colour(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Intensity(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Classification(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Displacement(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
