#include "vUUID.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"

void vUUID_Clear(vUUID *pUUID)
{
  memset(pUUID, 0, sizeof(vUUID));
}

udResult vUUID_SetFromString(vUUID *pUUID, const char *pStr)
{
  if (!vUUID_IsValid(pStr))
    return udR_InvalidParameter_;

  memset(pUUID, 0, sizeof(vUUID));
  memcpy(pUUID->internal_bytes, pStr, udMin((size_t)vUUID::vsUUID_Length, udStrlen(pStr)));

  return udR_Success;
}

const char* vUUID_GetAsString(const vUUID *pUUID)
{
  if (pUUID == nullptr)
    return nullptr;

  return (const char*)pUUID->internal_bytes;
}

bool vUUID_IsValid(const char *pUUIDStr)
{
  if (pUUIDStr == nullptr)
    return false;

  for (int i = 0; i < vUUID::vsUUID_Length; ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (pUUIDStr[i] == '-')
        continue;
    }
    else if ((pUUIDStr[i] >= '0' && pUUIDStr[i] <= '9') || (pUUIDStr[i] >= 'a' && pUUIDStr[i] <= 'f') || (pUUIDStr[i] >= 'A' && pUUIDStr[i] <= 'F'))
    {
      continue;
    }

    return false;
  }

  return true;
}

bool vUUID_IsValid(const vUUID *pUUID)
{
  return vUUID_IsValid((const char*)pUUID->internal_bytes);
}

uint64_t vUUID_ToNonce(const vUUID *pUUID)
{
  uint64_t accumA = udStrAtou64((const char *)&pUUID->internal_bytes[0], nullptr, 16);
  uint64_t accumB = udStrAtou64((const char *)&pUUID->internal_bytes[9], nullptr, 16);
  uint64_t accumC = udStrAtou64((const char *)&pUUID->internal_bytes[13], nullptr, 16);
  uint64_t accumD = udStrAtou64((const char *)&pUUID->internal_bytes[19], nullptr, 16);
  uint64_t accumE = udStrAtou64((const char *)&pUUID->internal_bytes[24], nullptr, 16);

  uint64_t partA = (accumA << 32) | (accumB << 16) | (accumC);
  uint64_t partB = (accumD << 48) | accumE;

  return (partA ^ partB);
}

bool operator ==(const vUUID a, const vUUID b)
{
  return (udStrncmpi((const char*)a.internal_bytes, (const char*)b.internal_bytes, vUUID::vsUUID_Length) == 0);
}

bool operator !=(const vUUID a, const vUUID b)
{
  return !(a == b);
}
