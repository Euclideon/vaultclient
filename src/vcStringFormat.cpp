#include "vcStringFormat.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

static uint32_t vcStringFormat_CalculateNewLength(const char *pFormatString, const char **ppStrings, size_t numStrings, uint32_t *pOldLength)
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
      currArg = udStrAtou((pFormatString + i + 1), &length);
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

const char *vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  uint32_t oldLen;
  uint32_t newLength = vcStringFormat_CalculateNewLength(pFormatString, ppStrings, numStrings, &oldLen);

  if (oldLen == 0)
    return pFormatString;
  
  char *pBuf = udAllocType(char, newLength, udAF_None);

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
      if (pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int length;
      currArg = udStrAtou((pFormatString + i + 1), &length);
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

const char *vcStringFormat(char *pBuf, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings, size_t *pTotalWritten)
{
  uint32_t currArg;
  uint32_t remaining = 0;
  uint32_t outPos = 0;
  for (uint32_t i = 0; pFormatString[i] != '\0'; ++i)
  {
    remaining = uint32_t(bufLen - outPos);

    if (remaining <= 1)
    {
      ++outPos;
      break;
    }

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
      if (pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int length;
      currArg = udStrAtou((pFormatString + i + 1), &length);
      
      if (currArg > numStrings || pFormatString[length + i + 1] != '}')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      uint32_t argLen = uint32_t(udStrlen(ppStrings[currArg]));
      if (argLen >= remaining)
        argLen = remaining - 1;

      uint32_t k;
      for (k = 0; k < argLen; ++k)
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

  if (pTotalWritten != nullptr)
    *pTotalWritten = outPos;

  return pBuf;
}

const char *vcStringFormat(const char *pFormatString, const char *pString)
{
  return vcStringFormat(pFormatString, &pString, 1);
}

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString, size_t *pTotalWritten)
{
  return vcStringFormat(pBuffer, bufLen, pFormatString, &pString, 1, pTotalWritten);
}
