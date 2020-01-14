#ifndef vcConvert_h__
#define vcConvert_h__

#include "vdkConvert.h"

#include "vcState.h"
#include "gl/vcTexture.h"

#include "udSafeDeque.h"

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
  udMutex *pMutex;

  vdkConvertContext *pConvertContext;
  const vdkConvertInfo *pConvertInfo;
  volatile vcConvertQueueStatus status;
  bool previewRequested;

  char author[vcMetadataMaxLength];
  char comment[vcMetadataMaxLength];
  char copyright[vcMetadataMaxLength];
  char license[vcMetadataMaxLength];

  const char *pItemProcessing;
  udChunkedArray<const char*> itemsToProcess;

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

  bool threadsRunning;
  udRWLock *pRWLock; // Protects the `jobs` chunked array
  size_t selectedItem;

  udSemaphore *pConversionSemaphore;
  udThread *pConversionThread;

  udSemaphore *pProcessingSemaphore;
  udThread *pProcessingThread;

};

void vcConvert_Init(vcState *pProgramState);
void vcConvert_Deinit(vcState *pProgramState);

void vcConvert_ShowUI(vcState *pProgramState);

void vcConvert_QueueFile(vcState *pProgramState, const char *pFilename);
void vcConvert_RemoveJob(vcState *pProgramState, size_t index);

#endif // !vcConvert_h__
