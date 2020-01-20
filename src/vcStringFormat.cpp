#include "vcStringFormat.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

const char *vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings)
{
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
      if (currArg > (int) numStrings || currArg < 0 || pFormatString[length + i + 1] != '}')
        continue;

      newChars += (int) udStrlen(ppStrings[currArg]);
      newChars -= length + 2;
      i += length - 1;
    }
  }

  len = i;

  if (len == 0)
    return pFormatString;

  int newLength = len + newChars + 1;
  char *pBuf = udAllocType(char, newLength, udAF_None);

  int outPos = 0;
  for (i = 0; pFormatString[i] != '\0'; ++i)
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
      if (len <= i + 1 || pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int length;
      currArg = udStrAtoi((pFormatString + i + 1), &length);
      if (currArg > (int) numStrings || currArg < 0 || pFormatString[length + i + 1] != '}')
      {
        pBuf[outPos] = pFormatString[i];
        ++outPos;
        continue;
      }

      int k;
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

const char *vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings)
{
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
      if (currArg > (int)numStrings || currArg < 0 || pFormatString[length + i + 1] != '}')
        continue;

      newChars += (int)udStrlen(ppStrings[currArg]);
      newChars -= length + 2;
      i += length - 1;
    }
  }

  len = i;
  int newLength = len + newChars + 1;

  if ((int)bufLen < newLength || len == 0)
    return pFormatString;

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
      if (currArg > (int)numStrings || currArg < 0 || pFormatString[length + i + 1] != '}')
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
