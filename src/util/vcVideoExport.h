#ifndef vcVideoExport_h__
#define vcVideoExport_h__

#include "udMath.h"
#include "vcFeatures.h"

struct vcVideoExport;

enum vcVideoExportFormat
{
  vcVideoExportFormat_PNGSequence,
  vcVideoExportFormat_JPGSequence,

#if VC_HAS_WINDOWSMEDIAFOUNDATION
  vcVideoExportFormat_MP4_H264,
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION

  vcVideoExportFormat_Count
};

struct vcVideoExportSettings
{
  vcVideoExportFormat format;
  char filename[1024];

  uint32_t width;
  uint32_t height;
  uint32_t fps; // 30, 60 etc.

  uint32_t targetBitrate;
};

udResult vcVideoExport_BeginExport(vcVideoExport **ppExport, const vcVideoExportSettings &settings);
udResult vcVideoExport_Complete(vcVideoExport **ppExport);

udResult vcVideoExport_AddFrame(vcVideoExport *pExport, struct vcTexture *pTexture, struct vcFramebuffer *pFramebuffer);

#endif
