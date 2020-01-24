#include "vcStringFormat.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

inline uint32_t vcStringFormat_CalculateNewLength(const char *pFormatString, const char **ppStrings, size_t numStrings, uint32_t *pOldLength)
{
  int newChars = 0;
  uint32_t currArg;
  uint32_t i;

  for (i = 0; pFormatString[i] != '\0'; ++i)
  {
    if (pFormatString[i] == '|' && pFormatString[i + 1] != '\0')
    {
      ++i;
      --newChars;
      continue;
    }
    // Format specifier
    if (pFormatString[i] == '{')
    {
      if (pFormatString[i + 1] == '\0' || pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
        continue;

      int length;
      currArg = udStrAtoi((pFormatString + i + 1), &length);
      if (currArg > numStrings || pFormatString[length + i + 1] != '}')
        continue;

      newChars += (int)udStrlen(ppStrings[currArg]);
      newChars -= length + 2;
      i += length - 1;
    }
  }

  if (pOldLength != nullptr)
    *pOldLength = i;

  return i + newChars + 1;
}

const char *vcStringFormat_FillBuffer(char *pBuf, const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  uint32_t currArg;

  uint32_t outPos = 0;
  for (uint32_t i = 0; pFormatString[i] != '\0'; ++i)
  {
    if (pFormatString[i] == '|' && pFormatString[i + 1] != '\0')
    {
      ++i;
      pBuf[outPos] = pFormatString[i];
      ++outPos;
      continue;
    }
    // Format Specifier
    if (pFormatString[i] == '{')
    {
      if (pFormatString[i + 1] == '\0' || pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int length;
      currArg = udStrAtoi((pFormatString + i + 1), &length);
      if (currArg > numStrings || pFormatString[length + i + 1] != '}')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      uint32_t k;
      for (k = 0; ppStrings[currArg][k] != '\0'; ++k)
        pBuf[outPos + k] = ppStrings[currArg][k];

      i += length + 1;
      outPos += k - 1;
    }
    else
    {
      pBuf[outPos] = pFormatString[i];
    }

    ++outPos;
  }
  pBuf[outPos] = '\0';

  return pBuf;
}

const char *vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  uint32_t oldLen;
  uint32_t newLength = vcStringFormat_CalculateNewLength(pFormatString, ppStrings, numStrings, &oldLen);

  if (oldLen == 0)
    return pFormatString;
  
  char *pBuf = udAllocType(char, newLength, udAF_None);

  return vcStringFormat_FillBuffer(pBuf, pFormatString, ppStrings, numStrings);
}

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  uint32_t oldLen;
  uint32_t newLength = vcStringFormat_CalculateNewLength(pFormatString, ppStrings, numStrings, &oldLen);

  if ((uint32_t)bufLen < newLength || oldLen == 0)
    return pFormatString;

  return vcStringFormat_FillBuffer(pBuffer, pFormatString, ppStrings, numStrings);
}

const char *vcStringFormat(const char *pFormatString, const char *pString)
{
  return vcStringFormat(pFormatString, &pString, 1);
}

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString)
{
  return vcStringFormat(pBuffer, bufLen, pFormatString, &pString, 1);
}
