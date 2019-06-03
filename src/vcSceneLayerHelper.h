#ifndef vcSceneLayerHelper_h__
#define vcSceneLayerHelper_h__

#include "udMath.h"
#include "udStringUtil.h"

template <typename T>
size_t vcSceneLayerHelper_ReadSceneLayerType(T *pValue, char *pData, const char *pType)
{
  size_t size = 0;
  if (udStrEqual(pType, "Int64"))
  {
    size = 8;
    *pValue = (T)*((int64_t*)pData);
  }
  else if (udStrEqual(pType, "Int32"))
  {
    size = 4;
    *pValue = (T)(*((int32_t*)pData));
  }
  else if (udStrEqual(pType, "Int16"))
  {
    size = 2;
    *pValue = (T)(*((int16_t*)pData));
  }
  else if (udStrEqual(pType, "Int8"))
  {
    size = 1;
    *pValue = (T)(*((int8_t*)pData));
  }
  else if (udStrEqual(pType, "UInt64"))
  {
    size = 8;
    *pValue = (T)(*((uint64_t*)pData));
  }
  else if (udStrEqual(pType, "UInt32"))
  {
    size = 4;
    *pValue = (T)(*((uint32_t*)pData));
  }
  else if (udStrEqual(pType, "UInt16"))
  {
    size = 2;
    *pValue = (T)(*((uint16_t*)pData));
  }
  else if (udStrEqual(pType, "UInt8"))
  {
    size = 1;
    *pValue = (T)(*((uint8_t*)pData));
  }
  else if (udStrEqual(pType, "Float64"))
  {
    size = 8;
    *pValue = (T)(*((double*)pData));
  }
  else if (udStrEqual(pType, "Float32"))
  {
    size = 4;
    *pValue = (T)(*((float*)pData));
  }
  else if (udStrEqual(pType, "String"))
  {
    // utf-8
    // TODO: handle
  }
  else
  {
    // TODO: Unexpected type!
  }

  return size;
}

size_t vcSceneLayerHelper_GetSceneLayerTypeSize(const char *pType)
{
  if (udStrEqual(pType, "Int64"))
    return 8;
  else if (udStrEqual(pType, "Int32"))
    return 4;
  else if (udStrEqual(pType, "Int16"))
    return 2;
  else if (udStrEqual(pType, "Int8"))
    return 1;
  else if (udStrEqual(pType, "UInt64"))
    return 8;
  else if (udStrEqual(pType, "UInt32"))
    return 4;
  else if (udStrEqual(pType, "UInt16"))
    return 2;
  else if (udStrEqual(pType, "UInt8"))
    return 1;
  else if (udStrEqual(pType, "Float64"))
    return 8;
  else if (udStrEqual(pType, "Float32"))
    return 4;
  else if (udStrEqual(pType, "String"))
    return 0; // TODO handle

  return 0; // TODO handle
}
#endif//vcSceneLayerHelper_h__
