#ifndef vcHistoryBuffer_h__
#define vcHistoryBuffer_h__

#include <stdint.h>
#include "udResult.h"

typedef void vcHistoryBuffer_DoFunc(void *pData);
typedef void vcHistoryBuffer_UndoFunc(void *pData);
typedef void vcHistoryBuffer_CleanupFunc(void *pData);

struct vcHistoryBufferNode
{
  vcHistoryBuffer_DoFunc *pDoFunc;
  vcHistoryBuffer_UndoFunc *pUndoFunc;
  vcHistoryBuffer_CleanupFunc *pCleanupFunc;

  void *pData;
};

struct vcHistoryBuffer;

udResult vcHistoryBuffer_Create(vcHistoryBuffer **ppBuffer);
udResult vcHistoryBuffer_Destroy(vcHistoryBuffer **ppBuffer);

udResult vcHistoryBuffer_Clear(vcHistoryBuffer *pBuffer);
udResult vcHistoryBuffer_DoAction(vcHistoryBuffer *pBuffer, vcHistoryBufferNode node, bool clearBuffer = false);
udResult vcHistoryBuffer_RedoAction(vcHistoryBuffer *pBuffer);
udResult vcHistoryBuffer_UndoAction(vcHistoryBuffer *pBuffer);

#endif // !vcHistoryBuffer_h__
