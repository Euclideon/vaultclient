#ifndef vdkConvert_h__
#define vdkConvert_h__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct vdkContext;
  struct vdkConvertContext;
  struct vdkPointCloud;

  enum vdkConvertSourceProjection
  {
    vdkCSP_SourceCartesian,
    vdkCSP_SourceLatLong,
    vdkCSP_SourceLongLat,

    vdkCSP_Count
  };

  struct vdkConvertInfo
  {
    const char *pOutputName;
    const char *pTempFilesPrefix;

    const char *pMetadata; // The metadata that will be added to this model (in JSON format)
    const char *pWatermark; // Base64 Encoded PNG watermark (in the format it gets stored in the metadata)

    double globalOffset[3]; // This amount is added to every point during conversion. Useful for moving the origin of the entire scene to geolocate

    double minPointResolution;
    double maxPointResolution;
    bool skipErrorsWherePossible; // If true it will continue processing other files if a file is detected as corrupt or incorrect

    uint32_t everyNth; // If this value is >1, only every Nth point is included in the model. e.g. 4 means only every 4th point will be included, skipping 3/4 of the points

    bool overrideResolution; // Set to true to stop the resolution from being recalculated
    double pointResolution; // The scale to be used in the conversion (either calculated or overriden)

    bool overrideSRID; // Set to true to prevent the SRID being recalculated
    int srid; // The geospatial reference ID (either calculated or overriden)

    uint64_t totalPointsRead; // How many points have been read in this model
    uint64_t totalItems; // How many items are in the list

    // These are quick stats while converting
    uint64_t currentInputItem; // Which item is currently being read
    uint64_t outputFileSize; // Size of the result UDS file
    uint64_t sourcePointCount; // Number of points added (may include duplicates or out of range points)
    uint64_t uniquePointCount; // Number of unique points in the final model
    uint64_t discardedPointCount; // Number of duplicate or ignored out of range points
    uint64_t outputPointCount; // Number of points written to UDS (can be used for progress)
    uint64_t peakDiskUsage; // Peak amount of disk space used including both temp files and the actual output file
    uint64_t peakTempFileUsage; // Peak amount of disk space that contained temp files
    uint32_t peakTempFileCount; // Peak number of temporary files written
  };

  struct vdkConvertItemInfo
  {
    const char *pFilename; // Name of the file
    int64_t pointsCount; // This might be an estimate, -1 is no estimate is available
    uint64_t pointsRead; // Once conversation begins, this will give an indication of progress

    vdkConvertSourceProjection sourceProjection; // What sort of projection this input has
  };

  VDKDLL_API enum vdkError vdkConvert_CreateContext(struct vdkContext *pContext, struct vdkConvertContext **ppConvertContext);
  VDKDLL_API enum vdkError vdkConvert_DestroyContext(struct vdkContext *pContext, struct vdkConvertContext **ppConvertContext);

  VDKDLL_API enum vdkError vdkConvert_SetOutputFilename(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const char *pFilename);
  VDKDLL_API enum vdkError vdkConvert_SetTempDirectory(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const char *pFolder);

  VDKDLL_API enum vdkError vdkConvert_SetPointResolution(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, bool override, double pointResolutionMeters);
  VDKDLL_API enum vdkError vdkConvert_SetSRID(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, bool override, int srid);
  VDKDLL_API enum vdkError vdkConvert_SetGlobalOffset(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const double globalOffset[3]);

  VDKDLL_API enum vdkError vdkConvert_SetSkipErrorsWherePossible(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, bool ignoreParseErrorsWherePossible);
  VDKDLL_API enum vdkError vdkConvert_SetEveryNth(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, uint32_t everyNth);

  VDKDLL_API enum vdkError vdkConvert_SetMetadata(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const char *pMetadataKey, const char *pMetadataValue);

  VDKDLL_API enum vdkError vdkConvert_AddItem(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const char *pFilename);
  VDKDLL_API enum vdkError vdkConvert_RemoveItem(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, uint64_t index);

  VDKDLL_API enum vdkError vdkConvert_SetInputSourceProjection(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, uint64_t index, vdkConvertSourceProjection actualProjection);

  VDKDLL_API enum vdkError vdkConvert_AddWatermark(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const char *pFilename);
  VDKDLL_API enum vdkError vdkConvert_RemoveWatermark(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext);

  VDKDLL_API enum vdkError vdkConvert_GetInfo(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, const struct vdkConvertInfo **ppInfo); // VDK continues to own and manage the info and it will be cleaned up when the context is
  VDKDLL_API enum vdkError vdkConvert_GetItemInfo(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, uint64_t index, struct vdkConvertItemInfo *pInfo); // Fills out the pInfo struct with this inputs info

  VDKDLL_API enum vdkError vdkConvert_DoConvert(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext);
  VDKDLL_API enum vdkError vdkConvert_Cancel(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext);

  VDKDLL_API enum vdkError vdkConvert_GeneratePreview(struct vdkContext *pContext, struct vdkConvertContext *pConvertContext, struct vdkPointCloud **ppCloud); // Caller needs to destroy this

#ifdef __cplusplus
}
#endif

#endif // vdkConvert_h__
