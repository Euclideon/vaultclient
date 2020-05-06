#include "vcStringFormat.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

const char *vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  size_t requiredLen = 0;

  vcStringFormat(nullptr, 0, pFormatString, ppStrings, numStrings, &requiredLen);

  if (requiredLen == 0)
    return nullptr;

  char *pBuffer = udAllocType(char, requiredLen, udAF_None);

  return vcStringFormat(pBuffer, requiredLen, pFormatString, ppStrings, numStrings, &requiredLen);
}

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings, size_t *pOutputLen /* = nullptr*/)
{
  if (pFormatString == nullptr || ppStrings == nullptr || numStrings == 0)
    return nullptr;

  int len;
  int newChars = 0;
  int currArg;
  int i;

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
      if (currArg >= (int)numStrings || currArg < 0 || pFormatString[length + i + 1] != '}')
        continue;

      newChars += (int)udStrlen(ppStrings[currArg]);
      newChars -= length + 2;
      i += length - 1;
    }
  }

  len = i;
  int newLength = len + newChars + 1;

  if (pOutputLen != nullptr)
    *pOutputLen = newLength;

  if ((int)bufLen < newLength || len == 0 || pBuffer == nullptr || bufLen == 0)
  {
    if (pBuffer != nullptr && bufLen > 1)
      udStrncpy(pBuffer, bufLen, pFormatString, bufLen - 1);

    return pBuffer;
  }

  int outPos = 0;
  for (i = 0; pFormatString[i] != '\0'; ++i)
  {
    if (pFormatString[i] == '|' && pFormatString[i + 1] != '\0')
    {
      ++i;
      pBuffer[outPos] = pFormatString[i];
      ++outPos;
      continue;
    }
    // Format Specifier
    if (pFormatString[i] == '{')
    {
      if (pFormatString[i + 1] == '\0' || pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
      {
        pBuffer[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int length;
      currArg = udStrAtoi((pFormatString + i + 1), &length);
      if (currArg >= (int)numStrings || currArg < 0 || pFormatString[length + i + 1] != '}' || ppStrings[currArg] == nullptr)
      {
        pBuffer[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int k;
      for (k = 0; ppStrings[currArg][k] != '\0'; ++k)
        pBuffer[outPos + k] = ppStrings[currArg][k];

      i += length + 1;
      outPos += k - 1;
    }
    else
    {
      pBuffer[outPos] = pFormatString[i];
    }

    ++outPos;
  }
  pBuffer[outPos] = '\0';

  return pBuffer;
}

const char *vcStringFormat(const char *pFormatString, const char *pString)
{
  return vcStringFormat(pFormatString, &pString, 1);
}

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString)
{
  return vcStringFormat(pBuffer, bufLen, pFormatString, &pString, 1);
}
