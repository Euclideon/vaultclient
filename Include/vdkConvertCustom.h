#ifndef vdkConvertCustom_h__
#define vdkConvertCustom_h__

#include "vdkConvert.h"

#ifdef __cplusplus
extern "C" {
#endif

  enum vdkConvertCustomItemFlags
  {
    vdkCCIF_None = 0,
    vdkCCIF_SkipErrorsWherePossible = 1,
    vdkCCIF_PolygonVerticesOnly = 2, // Do not rasterise the polygons, just use the vertices as points
  };

  enum vdkConvertAttribute
  {
    vdkCA_GPSTime,
    vdkCA_ARGB,
    vdkCA_Normal,
    vdkCA_Intensity,
    vdkCA_NIR,
    vdkCA_ScanAngle,
    vdkCA_PointSourceID,
    vdkCA_Classification,
    vdkCA_ReturnNumber,
    vdkCA_NumberOfReturns,
    vdkCA_ClassificationFlags,
    vdkCA_ScannerChannel,
    vdkCA_ScanDirection,
    vdkCA_EdgeOfFlightLine,
    vdkCA_ScanAngleRank,
    vdkCA_LASUserData,

    vdkCA_Count
  };

  enum vdkConvertAttributeContent
  {
    vdkCAC_None = (0),
    vdkCAC_GPSTime = (1 << vdkCA_GPSTime),
    vdkCAC_ARGB = (1 << vdkCA_ARGB),
    vdkCAC_Normal = (1 << vdkCA_Normal),
    vdkCAC_Intensity = (1 << vdkCA_Intensity),
    vdkCAC_NIR = (1 << vdkCA_NIR),
    vdkCAC_ScanAngle = (1 << vdkCA_ScanAngle),
    vdkCAC_PointSourceID = (1 << vdkCA_PointSourceID),
    vdkCAC_Classification = (1 << vdkCA_Classification),
    vdkCAC_ReturnNumber = (1 << vdkCA_ReturnNumber),
    vdkCAC_NumberOfReturns = (1 << vdkCA_NumberOfReturns),
    vdkCAC_ClassificationFlags = (1 << vdkCA_ClassificationFlags),
    vdkCAC_ScannerChannel = (1 << vdkCA_ScannerChannel),
    vdkCAC_ScanDirection = (1 << vdkCA_ScanDirection),
    vdkCAC_EdgeOfFlightLine = (1 << vdkCA_EdgeOfFlightLine),
    vdkCAC_ScanAngleRank = (1 << vdkCA_ScanAngleRank),
    vdkCAC_LasUserData = (1 << vdkCA_LASUserData),
  };
  inline vdkConvertAttributeContent operator|(vdkConvertAttributeContent a, vdkConvertAttributeContent b) { return (vdkConvertAttributeContent)(int(a) | int(b)); }
  inline vdkConvertAttributeContent operator&(vdkConvertAttributeContent a, vdkConvertAttributeContent b) { return (vdkConvertAttributeContent)(int(a) & int(b)); }

  struct vdkConvertPointBufferDouble
  {
    double (*pPositions)[3];
    uint8_t *pAttributes;
    size_t positionSize;
    size_t attributeSize;
    size_t pointCount;
    size_t pointsAllocated;
    vdkConvertAttributeContent content;
    uint32_t _reserved;
  };

  struct vdkConvertPointBufferInt64
  {
    int64_t (*pPositions)[3];
    uint8_t *pAttributes;
    size_t positionSize;
    size_t attributeSize;
    size_t pointCount;
    size_t pointsAllocated;
    vdkConvertAttributeContent content;
    uint32_t _reserved;
  };

  struct vdkConvertCustomItem
  {
    enum vdkError(*pOpen)(struct vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, enum vdkConvertCustomItemFlags flags);
    enum vdkError(*pReadPointsInt)(struct vdkConvertCustomItem *pConvertInput, vdkConvertPointBufferInt64 *pBuffer);
    enum vdkError(*pReadPointsFloat)(struct vdkConvertCustomItem *pConvertInput, vdkConvertPointBufferDouble *pBuffer);
    void(*pClose)(vdkConvertCustomItem *pConvertInput);
    void *pData;                        // Private data relevant to the specific type, must be freed by the pClose function

    const char *pName;                  // Filename or other identifier
    double boundMin[3], boundMax[3];    // Optional (see boundsKnown) source space min/max values
    double sourceResolution;            // Source resolution (eg 0.01 if points are 1cm apart). 0 indicates unknown
    int64_t pointCount;                 // Number of points coming, -1 if unknown
    int32_t srid;                       // If non-zero, this input is considered to be within the given srid code (useful mainly as a default value for other files in the conversion)
    vdkConvertAttributeContent content;        // Content of the input file
    vdkConvertSourceProjection sourceProjection;  // SourceLatLong defines x as latitude, y as longitude in WGS84.
    bool boundsKnown;                   // If true, knownMin and knownMax are valid
    bool pointCountIsEstimate;          // If true, the point count is an estimate and may be different
  };

  VDKDLL_API enum vdkError vdkConvert_AddCustomItem(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, struct vdkConvertCustomItem *pCustomItem);

#ifdef __cplusplus
}
#endif

#endif // vdkConvertCustom_h__
