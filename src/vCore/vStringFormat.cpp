#include "udPlatform/udPlatformUtil.h"
#include "vStringFormat.h"

const char *vStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings)
{
  int len = (int) udStrlen(pFormatString);
  char *pBuf = nullptr;
  int newChars = 0;
  int currArg;

  for (int i = 0; pFormatString[i] != '\0'; ++i)
  {
    // Format specifier
    if (pFormatString[i] == '{')
    {
      if (len <= i + 1 || pFormatString[i + 1] < '0' || pFormatString[i + 1] > '9')
        continue;

      int length;
      currArg = udStrAtoi((pFormatString + i + 1), &length);
      if (currArg > (int) numStrings || currArg < 0 || pFormatString[length + i] != '}')
        continue;

      newChars += (int) udStrlen(ppStrings[currArg]);
    }
  }
  int newLength = len + newChars + 1;
  pBuf = udAllocType(char, newLength, udAF_None);

  int outPos = 0;
  for (int i = 0; pFormatString[i] != '\0'; ++i)
  {
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
      if (currArg > (int) numStrings || currArg < 0 || pFormatString[length + i] != '}')
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
