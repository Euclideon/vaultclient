#include "udPointCloud.h"
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
    struct
    {
      bool repeating;
      double min;
      double max;
      double increment;
      const uint8_t *pColourArray; //length == 256*3
    } namedAttribute;
  } data;
};

uint32_t vcVoxelShader_Black(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_Colour(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_Intensity(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_Classification(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementDistance(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_DisplacementDirection(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_GPSTime(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_ScanAngle(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_PointSourceID(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_ReturnNumber(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);
uint32_t vcVoxelShader_NumberOfReturns(udPointCloud *pPointCloud,const udVoxelID *pVoxelID, const void *pUserData);

uint32_t vcVoxelShader_NamedAttributeF64(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeF32(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeUI64(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeUI32(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeUI16(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeUI8(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeI64(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeI32(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeI16(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
uint32_t vcVoxelShader_NamedAttributeI8(udPointCloud* pPointCloud, const udVoxelID* pVoxelID, const void* pUserData);
