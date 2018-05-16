#ifndef UDCHUNKEDARRAY_H
#define UDCHUNKEDARRAY_H
#include "udPlatform.h"
#include "udResult.h"

// --------------------------------------------------------------------------
template <typename T>
struct udChunkedArray
{
  udResult Init(size_t chunkElementCount);
  udResult Deinit();
  udResult Clear();

  T &operator[](size_t index);
  const T &operator[](size_t index) const;
  T *GetElement(size_t index);
  const T *GetElement(size_t index) const;
  size_t FindIndex(const T &element, size_t compareLen = sizeof(T)) const; // Linear search for matching element (first compareLen bytes compared)
  void SetElement(size_t index, const T &data);

  udResult PushBack(const T &v);
  udResult PushBack(T **ppElement);              // NOTE: Does not zero memory, can fail if memory allocation fails
  T *PushBack();                                 // DEPRECATED: Please use PushBack(const T&) or PushBack(T **)

  udResult PushFront(const T &v);
  udResult PushFront(T **ppElement);             // NOTE: Does not zero memory, can fail if memory allocation fails
  T *PushFront();                                // DEPRECATED: Please use PushFront(const T&) or PushFront(T **)

  udResult Insert(size_t index, const T *pData = nullptr);  // Insert the element at index, pushing and moving all elements after to make space.

  bool PopBack(T *pData = nullptr);              // Returns false if no element to pop
  bool PopFront(T *pData = nullptr);             // Returns false if no element to pop
  void RemoveAt(size_t index);                   // Remove the element at index, moving all elements after to fill the gap.
  void RemoveSwapLast(size_t index);             // Remove the element at index, swapping with the last element to ensure array is contiguous

  udResult ToArray(T *pArray, size_t arrayLength, size_t startIndex = 0, size_t count = 0); // Copy elements to an array supplied by caller
  udResult ToArray(T **ppArray, size_t startIndex = 0, size_t count = 0);                   // Copy elements to an array allocated and returned to caller

  udResult GrowBack(size_t numberOfNewElements); // Push back a number of new elements, zeroing the memory
  udResult ReserveBack(size_t newCapacity);      // Reserve memory for a given number of elements without changing 'length'  NOTE: Does not reduce in size
  udResult AddChunks(size_t numberOfNewChunks);  // Add a given number of chunks capacity without changing 'length'

  size_t GetElementRunLength(size_t index) const;// At element index, return the number of elements including index that follow in the same chunk (ie can be indexed directly)
  size_t ChunkElementCount() const               { return chunkElementCount; }
  size_t ElementSize() const                     { return sizeof(T); }

  enum { ptrArrayInc = 32};

  T **ppChunks;
  size_t ptrArraySize;
  size_t chunkElementCount;
  size_t chunkCount;

  size_t length;
  size_t inset;
};

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::Init(size_t a_chunkElementCount)
{
  udResult result = udR_Success;
  size_t c = 0;

  ppChunks = nullptr;
  chunkElementCount = 0;
  chunkCount = 0;
  length = 0;
  inset = 0;

  // Must be power of 2.
  UD_ERROR_IF(!a_chunkElementCount || (a_chunkElementCount & (a_chunkElementCount - 1)), udR_InvalidParameter_);

  chunkElementCount = a_chunkElementCount;
  chunkCount = 1;

  if (chunkCount > ptrArrayInc)
    ptrArraySize = ((chunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
  else
    ptrArraySize = ptrArrayInc;

  ppChunks = udAllocType(T*, ptrArraySize, udAF_Zero);
  if (!ppChunks)
  {
    result = udR_MemoryAllocationFailure;
    goto epilogue;
  }

  for (; c < chunkCount; ++c)
  {
    ppChunks[c] = udAllocType(T, chunkElementCount, udAF_None);
    if (!ppChunks[c])
    {
      result = udR_MemoryAllocationFailure;
      goto epilogue;
    }
  }

  return result;
epilogue:

  for (size_t i = 0; i < c; ++i)
    udFree(ppChunks[i]);

  udFree(ppChunks);

  return result;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::Deinit()
{
  for (size_t c = 0; c < chunkCount; ++c)
    udFree(ppChunks[c]);

  udFree(ppChunks);

  chunkCount = 0;
  length = 0;
  inset = 0;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, June 2015
template <typename T>
inline udResult udChunkedArray<T>::Clear()
{
  length = 0;
  inset = 0;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::AddChunks(size_t numberOfNewChunks)
{
  size_t newChunkCount = chunkCount + numberOfNewChunks;

  if (newChunkCount > ptrArraySize)
  {
    size_t newPtrArraySize = ((newChunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
    T **newppChunks = udAllocType(T*, newPtrArraySize, udAF_Zero);
    if (!newppChunks)
      return udR_MemoryAllocationFailure;

    memcpy(newppChunks, ppChunks, ptrArraySize * sizeof(T*));
    udFree(ppChunks);

    ppChunks = newppChunks;
    ptrArraySize = newPtrArraySize;
  }

  for (size_t c = chunkCount; c < newChunkCount; ++c)
  {
    ppChunks[c] = udAllocType(T, chunkElementCount, udAF_None);
    if (!ppChunks[c])
    {
      chunkCount = c;
      return udR_MemoryAllocationFailure;
    }
  }

  chunkCount = newChunkCount;
  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::GrowBack(size_t numberOfNewElements)
{
  if (numberOfNewElements == 0)
    return udR_InvalidParameter_;

  size_t oldLength = inset + length;
  size_t newLength = oldLength + numberOfNewElements;
  size_t prevUsedChunkCount = (oldLength + chunkElementCount - 1) / chunkElementCount;

  udResult res = ReserveBack(length + numberOfNewElements);
  if (res != udR_Success)
    return res;

  // Zero new elements
  size_t newUsedChunkCount = (newLength + chunkElementCount - 1) / chunkElementCount;
  size_t usedChunkDelta = newUsedChunkCount - prevUsedChunkCount;
  size_t head = oldLength % chunkElementCount;

  if (usedChunkDelta)
  {
    if (head)
      memset(&ppChunks[prevUsedChunkCount - 1][head], 0, (chunkElementCount - head) * sizeof(T));

    size_t tail = newLength % chunkElementCount;

    for (size_t chunkIndex = prevUsedChunkCount; chunkIndex < (newUsedChunkCount - 1 + !tail); ++chunkIndex)
      memset(ppChunks[chunkIndex], 0, sizeof(T) * chunkElementCount);

    if (tail)
      memset(&ppChunks[newUsedChunkCount - 1][0], 0, tail * sizeof(T));
  }
  else
  {
    memset(&ppChunks[prevUsedChunkCount - 1][head], 0, numberOfNewElements * sizeof(T));
  }

  length += numberOfNewElements;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::ReserveBack(size_t newCapacity)
{
  udResult res = udR_Success;
  size_t oldCapacity = chunkElementCount * chunkCount - inset;
  if (newCapacity > oldCapacity)
  {
    size_t newChunksCount = (newCapacity - oldCapacity + chunkElementCount - 1) / chunkElementCount;
    res = AddChunks(newChunksCount);
  }
  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline T &udChunkedArray<T>::operator[](size_t index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return ppChunks[chunkIndex][index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, September 2015
template <typename T>
inline const T &udChunkedArray<T>::operator[](size_t index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return ppChunks[chunkIndex][index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline T* udChunkedArray<T>::GetElement(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return &ppChunks[chunkIndex][index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, January 2016
template <typename T>
inline const T* udChunkedArray<T>::GetElement(size_t index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return &ppChunks[chunkIndex][index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, March 2018
template <typename T>
inline size_t udChunkedArray<T>::FindIndex(const T &element, size_t compareLen) const
{
  size_t index = 0;
  while (index < length)
  {
    size_t runLen = GetElementRunLength(index);
    const T *pRun = GetElement(index);
    for (size_t runIndex = 0; runIndex < runLen; ++runIndex)
    {
      if (memcmp(&element, &pRun[runIndex], compareLen) == 0)
        return index + runIndex;
    }
    index += runLen;
  }
  return length; // Sentinal to indicate not found
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline void udChunkedArray<T>::SetElement(size_t index, const T &data)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  ppChunks[chunkIndex][index % chunkElementCount] = data;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushBack(T **ppElement)
{
  UDASSERT(ppElement, "parameter is null");

  udResult res = ReserveBack(length + 1);
  if (res != udR_Success)
    return res;

  size_t newIndex = inset + length;
  size_t chunkIndex = size_t(newIndex / chunkElementCount);

  *ppElement = ppChunks[chunkIndex] + (newIndex % chunkElementCount);

  ++length;
  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline T *udChunkedArray<T>::PushBack()
{
  T *pElement = nullptr;

  if (PushBack(&pElement) == udR_Success)
    memset(pElement, 0, sizeof(T));

  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushBack(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushBack(&pElement);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, March 2016
template <typename T>
inline udResult udChunkedArray<T>::PushFront(T **ppElement)
{
  UDASSERT(ppElement, "parameter is null");

  if (inset)
  {
    --inset;
  }
  else
  {
    if (length)
    {
      // Are we out of pointers
      if ((chunkCount + 1) > ptrArraySize)
      {
        T **ppNewChunks = udAllocType(T*, (ptrArraySize + ptrArrayInc), udAF_Zero);
        if (!ppNewChunks)
          return udR_MemoryAllocationFailure;

        ptrArraySize += ptrArrayInc;
        memcpy(ppNewChunks + 1, ppChunks, chunkCount * sizeof(T*));

        udFree(ppChunks);
        ppChunks = ppNewChunks;
      }
      else
      {
        memmove(ppChunks + 1, ppChunks, chunkCount * sizeof(T*));
      }

      // See if we have an unused chunk at the end of the array
      if (((chunkCount * chunkElementCount) - length) > chunkElementCount)
      {
        //Note that these operations are not chunkCount-1 as the array has been moved above and chunked-1 is actually @chunkcount now
        ppChunks[0] = ppChunks[chunkCount];
        ppChunks[chunkCount] = nullptr;
      }
      else
      {
        T *pNewBlock = udAllocType(T, chunkElementCount, udAF_None);
        if (!pNewBlock)
        {
          memmove(ppChunks, ppChunks + 1, chunkCount * sizeof(T*));
          return udR_MemoryAllocationFailure;
        }
        ppChunks[0] = pNewBlock;
        ++chunkCount;
      }
    }
    inset = chunkElementCount - 1;
  }

  ++length;

  *ppElement = ppChunks[0] + inset;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline T *udChunkedArray<T>::PushFront()
{
  T *pElement = nullptr;

  if (PushFront(&pElement) == udR_Success)
    memset(pElement, 0, sizeof(T));

  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushFront(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushFront(&pElement);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline bool udChunkedArray<T>::PopBack(T *pDest)
{
  if (length)
  {
    if (pDest)
      *pDest = *GetElement(length - 1);
    --length;

    if (length == 0)
      inset = 0;
    return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline bool udChunkedArray<T>::PopFront(T *pDest)
{
  if (length)
  {
    if (pDest)
      *pDest = *GetElement(0);
    ++inset;
    if (inset == chunkElementCount)
    {
      inset = 0;
      if (chunkCount > 1)
      {
        T *pHead = ppChunks[0];
        memmove(ppChunks, ppChunks + 1, (chunkCount - 1) * sizeof(T*));
        ppChunks[chunkCount - 1] = pHead;
      }
    }

    --length;

    if (length == 0)
      inset = 0;
    return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, October 2015
template <typename T>
inline void udChunkedArray<T>::RemoveAt(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");

  if (index == 0)
  {
    PopFront();
  }
  else if (index == (length - 1))
  {
    PopBack();
  }
  else
  {
    index += inset;

    size_t chunkIndex = index / chunkElementCount;

    // Move within the chunk of the remove item
    if ((index % chunkElementCount) != (chunkElementCount - 1)) // If there are items after the remove item
      memmove(&ppChunks[chunkIndex][index % chunkElementCount], &ppChunks[chunkIndex][(index + 1) % chunkElementCount], sizeof(T) * (chunkElementCount - 1 - (index % chunkElementCount)));

    // Handle middle chunks
    for (size_t i = (chunkIndex + 1); i < (chunkCount - 1); ++i)
    {
      // Move first item down
      memcpy(&ppChunks[i - 1][chunkElementCount - 1], &ppChunks[i][0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[i][0], &ppChunks[i][1], sizeof(T) * (chunkElementCount - 1));
    }

    // Handle last chunk
    if (chunkIndex != (chunkCount - 1))
    {
      // Move first item down
      memcpy(&ppChunks[chunkCount - 2][chunkElementCount - 1], &ppChunks[chunkCount - 1][0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[chunkCount - 1][0], &ppChunks[chunkCount - 1][1], sizeof(T) * ((length + (inset - 1)) % chunkElementCount));
    }

    PopBack();
  }
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2015
template <typename T>
inline void udChunkedArray<T>::RemoveSwapLast(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");

  // Only copy the last element over if the element being removed isn't the last element
  if (index != (length - 1))
    SetElement(index, *GetElement(length - 1));
  PopBack();
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
template <typename T>
inline udResult udChunkedArray<T>::ToArray(T *pArray, size_t arrayLength, size_t startIndex, size_t count)
{
  udResult result;

  if (count == 0)
    count = length - startIndex;
  UD_ERROR_IF(startIndex >= length, udR_OutOfRange);
  UD_ERROR_NULL(pArray, udR_InvalidParameter_);
  UD_ERROR_IF(arrayLength < count, udR_BufferTooSmall);
  while (count)
  {
    size_t runLen = udMin(count, GetElementRunLength(startIndex));
    memcpy(pArray, GetElement(startIndex), runLen * sizeof(T));
    pArray += runLen;
    startIndex += runLen;
    count -= runLen;
  }
  result = udR_Success;

epilogue:
  return result;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
template <typename T>
udResult udChunkedArray<T>::ToArray(T **ppArray, size_t startIndex, size_t count)
{
  udResult result;
  T *pArray = nullptr;

  if (count == 0)
    count = length - startIndex;
  UD_ERROR_IF(startIndex >= length, udR_OutOfRange);
  UD_ERROR_NULL(ppArray, udR_InvalidParameter_);

  if (count)
  {
    pArray = udAllocType(T, count, udAF_None);
    UD_ERROR_NULL(pArray, udR_MemoryAllocationFailure);
    UD_ERROR_CHECK(ToArray(pArray, count, startIndex, count));
  }
  // Transfer ownership of array and assign success
  *ppArray = pArray;
  pArray = nullptr;
  result = udR_Success;

epilogue:
  udFree(pArray);
  return result;
}

// --------------------------------------------------------------------------
// Author: Bryce Kiefer, November 2015
template <typename T>
inline udResult udChunkedArray<T>::Insert(size_t index, const T *pData)
{
  UDASSERT(index <= length, "Index out of bounds");

  // Make room for new element
  udResult result = GrowBack(1);
  if (result != udR_Success)
    return result;

  // TODO: This should be changed to a per-chunk loop,
  // using memmove to move all but the last element, and a
  // memcpy from the previous chunk for the (now first)
  // element of each chunk

  // Move each element at and after the insertion point to the right by one
  for (size_t i = length - 1; i > index; --i)
  {
    memcpy(&ppChunks[i / chunkElementCount][i % chunkElementCount], &ppChunks[(i - 1) / chunkElementCount][(i - 1) % chunkElementCount], sizeof(T));
  }

  // Copy the new element into the insertion point if it exists
  if (pData != nullptr)
    memcpy(&ppChunks[index / chunkElementCount][index % chunkElementCount], pData, sizeof(T));

  return result;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, November 2017
template <typename T>
inline size_t udChunkedArray<T>::GetElementRunLength(size_t index) const
{
  if (index < length)
  {
    index += inset;
    size_t runLength = chunkElementCount - (index % chunkElementCount);
    return (runLength > length - index) ? length - index : runLength;
  }
  return 0;
}

#endif // UDCHUNKEDARRAY_H
