#ifndef vUUID_h__
#define vUUID_h__

#include <stdint.h>
#include "udPlatform.h"

struct vUUID
{
  enum {
    vsUUID_Length = 36
  };

  uint8_t internal_bytes[vsUUID_Length+1]; //+1 for \0
};

void vUUID_Clear(vUUID *pUUID);

udResult vUUID_SetFromString(vUUID *pUUID, const char *pStr); //pStr can be freed after this
const char* vUUID_GetAsString(const vUUID *pUUID); //Do not free, you do not own this

bool vUUID_IsValid(const vUUID *pUUID);
bool vUUID_IsValid(const char *pUUIDStr);

uint64_t vUUID_ToNonce(const vUUID *pUUID);

bool operator ==(const vUUID a, const vUUID b);
bool operator !=(const vUUID a, const vUUID b);

#endif //vUUID_h__
