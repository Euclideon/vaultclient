#include "vcStringFormat.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

#define VC_STRING_FORMAT_BUFFER_COUNT 16
static volatile int32_t s_vcStringBufferIndex = 0;
static char s_vcStringBuffers[VC_STRING_FORMAT_BUFFER_COUNT][64];

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

// ****************************************************************************
// Author: Damian Madden, February 2020
const char *vcStringFormat_ImGuiClean(const char *pStr)
{
  const char *pPos = pStr;

  char *pBuf = s_vcStringBuffers[udInterlockedPostIncrement(&s_vcStringBufferIndex) & (VC_STRING_FORMAT_BUFFER_COUNT - 1)];
  char *pOut = pBuf;

  int len = (int)udStrlen(pStr);
  if (len > 64)
    return nullptr;

  char hexCode[2];

  for (int i = 0; i < len; ++i)
  {
    char c = *pPos;

    switch (c)
    {
    // Processes URI encoding like %0xXX
    case '%':
      char temp;
      int chars;

      if (i + 2 < len)
      {
        hexCode[0] = *(pPos + 1);
        hexCode[1] = *(pPos + 2);
      }

      temp = (char)udStrAtoi(hexCode, &chars, 16);

      if (chars != 2)
      {
        // Escape the % symbol
        *pOut = '\\';
        *(++pOut) = c;
      }
      else
      {
        *pOut = temp;
      }

      break;
    case '|':
      --pOut;
      ++pPos;
      break;
    default:
      *pOut = c;
      break;
    }

    ++pOut;
    ++pPos;
  }

  *pOut = '\0';

  return pBuf;
}
