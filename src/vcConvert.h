#ifndef vcConvert_h__
#define vcConvert_h__

#include "vdkConvert.h"

#include "vcState.h"
#include "gl/vcTexture.h"

struct vcState;

enum vcConvertQueueStatus
{
  vcCQS_Preparing,
  vcCQS_Queued,
  vcCQS_QueuedPendingLicense,
  vcCQS_Running,
  vcCQS_Completed,
  vcCQS_Cancelled,
  vcCQS_WriteFailed,
  vcCQS_ParseFailed,
  vcCQS_ImageParseFailed,
  vcCQS_Failed,

  vcCQS_Count
};

struct vcConvertItem
{
  vdkConvertContext *pConvertContext;
  const vdkConvertInfo *pConvertInfo;
  volatile vcConvertQueueStatus status;
  bool previewRequested;

  char author[vcMetadataMaxLength];
  char comment[vcMetadataMaxLength];
  char copyright[vcMetadataMaxLength];
  char license[vcMetadataMaxLength];

  struct
  {
    bool isDirty;
    const char *pFilename;
    vcTexture *pTexture;
    int width;
    int height;
  } watermark;
};

struct vcConvertContext
{
  udChunkedArray<vcConvertItem*> jobs;
  bool threadRunning;
  udSemaphore *pSemaphore;
  udMutex *pMutex;
  udThread *pThread;

  size_t selectedItem;
};

void vcConvert_Init(vcState *pProgramState);
void vcConvert_Deinit(vcState *pProgramState);

void vcConvert_ShowUI(vcState *pProgramState);

bool vcConvert_AddFile(vcState *pProgramState, const char *pFilename, bool watermark = false);
void vcConvert_RemoveJob(vcState *pProgramState, size_t index);

#endif // !vcConvert_h__
