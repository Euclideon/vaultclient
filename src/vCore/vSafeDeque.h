#ifndef vSafeDeque_h__
#define vSafeDeque_h__
#include "udPlatform/udResult.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"

/// Thread safe double ended queue
template <typename T>
struct vSafeDeque
{
  udChunkedArray<T> chunkedArray;
  udMutex *pMutex = nullptr;
};

/// Creates a vSafeDeque and initialises it
/// Will return nullptr if fails to create or initialise
template <typename T>
udResult vSafeDeque_Create(vSafeDeque<T> **ppDeque, size_t elementCount)
{
  if (ppDeque == nullptr || elementCount == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  vSafeDeque<T> *pDeque = udAllocType(vSafeDeque<T>, 1, udAF_Zero);

  UD_ERROR_NULL(pDeque, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(pDeque->chunkedArray.Init(elementCount));

  pDeque->pMutex = udCreateMutex();
  UD_ERROR_NULL(pDeque, udR_MemoryAllocationFailure);

epilogue:
  if (result != udR_Success)
    vSafeDeque_Destroy(&pDeque);

  *ppDeque = pDeque;
  return result;
}

/// Deinitialises and deletes the vSafeDeque
/// Will return false is fails to deinitialises or delete.
template <typename T>
void vSafeDeque_Destroy(vSafeDeque<T> **ppDeque)
{
  if (ppDeque == nullptr || *ppDeque == nullptr)
    return;

  udDestroyMutex(&(*ppDeque)->pMutex);
  (*ppDeque)->chunkedArray.Deinit();
  udFree(*ppDeque);
}

/// Adds v to the back of vSafeDeque
/// Will return false if failed to add v
template <typename T>
inline udResult vSafeDeque_PushBack(vSafeDeque<T> *pDeque, const T &v)
{
  if (pDeque == nullptr)
    return udR_InvalidParameter_;

  udLockMutex(pDeque->pMutex);
  udResult result = pDeque->chunkedArray.PushBack(v);
  udReleaseMutex(pDeque->pMutex);

  return result;
}

/// Adds v to the front of vSafeDeque
/// Will return false if failed to add v
template <typename T>
inline udResult vSafeDeque_PushFront(vSafeDeque<T> *pDeque, const T &v)
{
  if (pDeque == nullptr)
    return udR_InvalidParameter_;

  udLockMutex(pDeque->pMutex);
  udResult result = pDeque->chunkedArray.PushFront(v);
  udReleaseMutex(pDeque->pMutex);

  return result;
}

/// Removes the last element in vSafeDeque and copies it to pData
/// Will return false if failed to remove and copy the last element
template <typename T>
inline udResult vSafeDeque_PopBack(vSafeDeque<T> *pDeque, T *pData)
{
  if (pDeque == nullptr)
    return udR_InvalidParameter_;

  udLockMutex(pDeque->pMutex);
  udResult result = pDeque->chunkedArray.PopBack(pData) ? udR_Success : udR_Failure_;
  udReleaseMutex(pDeque->pMutex);

  return result;
}

/// Removes the first element in vSafeDeque and copies it to pData
/// Will return false if failed to remove and copy the first element
template <typename T>
inline udResult vSafeDeque_PopFront(vSafeDeque<T> *pDeque, T *pData)
{
  if (pDeque == nullptr)
    return udR_InvalidParameter_;

  if (pDeque->chunkedArray.length == 0)
    return udR_NothingToDo;

  udLockMutex(pDeque->pMutex);
  udResult result = pDeque->chunkedArray.PopFront(pData) ? udR_Success : udR_Failure_;
  udReleaseMutex(pDeque->pMutex);

  return result;
}

#endif // vSafeDeque_h__
