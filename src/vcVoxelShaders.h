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
    } displacementAmount;
    struct
    {
      uint32_t attributeOffsets[3];
      udDouble3 cameraDirection;
      uint32_t posColour;
      uint32_t negColour;
    } displacementDirection;
    struct
    {
      double minTime;
      double maxTime;
      double mult;
    } GPSTime;
    struct
    {
      int16_t minAngle;
      int16_t maxAngle;
      uint32_t range;
    } scanAngle;
    struct
    {
      const udChunkedArray<vcVisualizationSettings::KV> *pColourMap;
      uint32_t defaultColour;
    } pointSourceID;
    struct
    {
      const uint32_t *pColours; //length == 256
    } returnNumber;
    struct
    {
      const uint32_t *pColours; //length == 256
    } numberOfReturns;
  } data;
};

uint32_t vcVoxelShader_Black(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Colour(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Intensity(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_Classification(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementDistance(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementDirection(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_GPSTime(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_ScanAngle(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_PointSourceID(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_ReturnNumber(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
uint32_t vcVoxelShader_NumberOfReturns(vdkPointCloud *pPointCloud, uint64_t voxelID, const void *pUserData);
