#include "vcHistoryBuffer.h"

#include "udChunkedArray.h"

struct vcHistoryBuffer
{
  udChunkedArray<vcHistoryBufferNode> buffer;
  size_t offset; // Offset from buffer.size()
};

udResult vcHistoryBuffer_Create(vcHistoryBuffer **ppBuffer)
{
  udResult result = udR_Success;
  vcHistoryBuffer *pBuffer = nullptr;
  UD_ERROR_NULL(ppBuffer, udR_InvalidParameter_);

  pBuffer = udAllocType(vcHistoryBuffer, 1, udAF_Zero);
  UD_ERROR_NULL(pBuffer, udR_MemoryAllocationFailure);
  pBuffer->buffer.Init(128);

  *ppBuffer = pBuffer;
  pBuffer = nullptr;

epilogue:
  udFree(pBuffer);

  return result;
}

udResult vcHistoryBuffer_Destroy(vcHistoryBuffer **ppBuffer)
{
  udResult result = udR_Success;
  UD_ERROR_NULL(ppBuffer, udR_InvalidParameter_);
  UD_ERROR_NULL(*ppBuffer, udR_InvalidParameter_);

  vcHistoryBuffer_Clear(*ppBuffer);
  (*ppBuffer)->buffer.Deinit();
  udFree(*ppBuffer);

epilogue:
  return result;
}

udResult vcHistoryBuffer_Clear(vcHistoryBuffer *pBuffer)
{
  udResult result = udR_Success;
  UD_ERROR_NULL(pBuffer, udR_InvalidParameter_);

  for (size_t i = 0; i < pBuffer->buffer.length; ++i)
    pBuffer->buffer[i].pCleanupFunc(pBuffer->buffer[i].pData);

epilogue:
  return result;
}

udResult vcHistoryBuffer_DoAction(vcHistoryBuffer *pBuffer, vcHistoryBufferNode node, bool clearBuffer /*= false*/)
{
  udResult result = udR_Success;
  UD_ERROR_NULL(pBuffer, udR_InvalidParameter_);

  if (clearBuffer)
    vcHistoryBuffer_Clear(pBuffer);

  while (pBuffer->offset > 0)
  {
    vcHistoryBufferNode temp = {};
    UD_ERROR_IF(pBuffer->buffer.PopBack(&temp), udR_InternalError);
    temp.pCleanupFunc(temp.pData);
    --pBuffer->offset;
  }

  UD_ERROR_CHECK(pBuffer->buffer.PushBack(node));
  node.pDoFunc(node.pData);

epilogue:
  return result;
}

udResult vcHistoryBuffer_RedoAction(vcHistoryBuffer *pBuffer)
{
  udResult result = udR_Success;
  vcHistoryBufferNode *pNode = nullptr;
  UD_ERROR_NULL(pBuffer, udR_InvalidParameter_);
  UD_ERROR_IF(pBuffer->offset == 0, udR_NothingToDo);

  pNode = pBuffer->buffer.GetElement(pBuffer->buffer.length - pBuffer->offset - 1);
  pNode->pDoFunc(pNode->pData);
  --pBuffer->offset;

epilogue:
  return result;
}

udResult vcHistoryBuffer_UndoAction(vcHistoryBuffer *pBuffer)
{
  udResult result = udR_Success;
  vcHistoryBufferNode *pNode = nullptr;
  UD_ERROR_NULL(pBuffer, udR_InvalidParameter_);
  UD_ERROR_IF(pBuffer->offset == pBuffer->buffer.length, udR_NothingToDo);

  pNode = pBuffer->buffer.GetElement(pBuffer->buffer.length - pBuffer->offset - 1);
  pNode->pUndoFunc(pNode->pData);
  ++pBuffer->offset;

epilogue:
  return result;
}
