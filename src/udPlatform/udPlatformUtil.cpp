#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udFile.h"
#include "udMath.h"
#include "udJSON.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>

#if UDPLATFORM_WINDOWS
#include <io.h>
#include <mmsystem.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include "dirent.h"
#include <time.h>
#include <sys/time.h>
static const uint64_t nsec_per_sec = 1000000000; // 1 billion nanoseconds in one second
static const uint64_t nsec_per_msec = 1000000;   // 1 million nanoseconds in one millisecond
//static const uint64_t usec_per_msec = 1000;      // 1 thousand microseconds in one millisecond
#endif

static char s_udStrEmptyString[] = "";

// *********************************************************************
// Author: Dave Pevreal, October 2016
void udUpdateCamera(double camera[16], double yawRadians, double pitchRadians, double tx, double ty, double tz)
{
  udDouble4x4 rotation = udDouble4x4::create(camera);
  udDouble3 pos = rotation.axis.t.toVector3();
  rotation.axis.t = udDouble4::identity();

  if (yawRadians != 0.0)
    rotation = udDouble4x4::rotationZ(yawRadians) * rotation;   // Yaw on global axis
  if (pitchRadians != 0.0)
    rotation = rotation * udDouble4x4::rotationX(pitchRadians); // Pitch on local axis
  udDouble3 trans = udDouble3::zero();
  trans += rotation.axis.x.toVector3() * tx;
  trans += rotation.axis.y.toVector3() * ty;
  trans += rotation.axis.z.toVector3() * tz;
  rotation.axis.t = udDouble4::create(pos + trans, 1.0);

  memcpy(camera, rotation.a, sizeof(rotation));
}

// *********************************************************************
// Author: Dave Pevreal, October 2016
void udUpdateCamera(float camera[16], float yawRadians, float pitchRadians, float tx, float ty, float tz)
{
  udFloat4x4 rotation = udFloat4x4::create(camera);
  udFloat3 pos = rotation.axis.t.toVector3();
  rotation.axis.t = udFloat4::identity();

  rotation = udFloat4x4::rotationZ(yawRadians) * rotation;   // Yaw on global axis
  rotation = rotation * udFloat4x4::rotationX(pitchRadians); // Pitch on local axis
  pos += rotation.axis.x.toVector3() * tx;
  pos += rotation.axis.y.toVector3() * ty;
  pos += rotation.axis.z.toVector3() * tz;
  rotation.axis.t = udFloat4::create(pos, 1.0);

  memcpy(camera, rotation.a, sizeof(rotation));
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udGetTimeMs()
{
#if UDPLATFORM_WINDOWS
  return timeGetTime();
#else
  //struct timeval now;
  //gettimeofday(&now, NULL);
  //return (uint32_t)(now.tv_usec/usec_per_msec);

  // Unfortunately the above code is unreliable
  // TODO: Check whether this code gives consistent timing values regardless of thread
  struct timespec ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts1);
  return (ts1.tv_sec * 1000) + (ts1.tv_nsec / nsec_per_msec);
#endif
}

// *********************************************************************
// Author: Dave Pevreal, June 2014
uint64_t udPerfCounterStart()
{
#if UDPLATFORM_WINDOWS
  LARGE_INTEGER p;
  QueryPerformanceCounter(&p);
  return p.QuadPart;
#else
  uint64_t nsec_count;
  struct timespec ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts1);
  nsec_count = ts1.tv_nsec + ts1.tv_sec * nsec_per_sec;
  return nsec_count;
#endif
}


// *********************************************************************
// Author: Dave Pevreal, June 2014
float udPerfCounterMilliseconds(uint64_t startValue, uint64_t end)
{
#if UDPLATFORM_WINDOWS
  LARGE_INTEGER p, f;
  if (end)
    p.QuadPart = end;
  else
    QueryPerformanceCounter(&p);
  QueryPerformanceFrequency(&f);

  // TODO: Come back and tidy this up to be integer

  double delta = (double)(p.QuadPart - startValue);

  double ms = (delta) ? (1000.0 / (f.QuadPart / delta)) : 0.0;
  return (float)ms;
#else
  if (!end)
    end = udPerfCounterStart();
  double ms = (end - startValue) / (nsec_per_sec / 1000.0);
  return (float)ms;
#endif
}

// *********************************************************************
// Author: Dave Pevreal, April 2018
float udPerfCounterSeconds(uint64_t startValue, uint64_t end)
{
  return udPerfCounterMilliseconds(startValue, end) / 1000.f;
}

// *********************************************************************
// Author: Dave Pevreal, April 2018
int udDaysUntilExpired(int maxDays, const char **ppExpireDateStr)
{
  // Calculate the build year/month compile-time constants
  #define BUILDDATE __DATE__
  #define BUILDDATEYEAR (BUILDDATE[7]*1000 + BUILDDATE[8]*100 + BUILDDATE[9]*10 + BUILDDATE[10] - 1111*'0')
  #define BUILDDATEMONTH  (((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'n')) ?  1 : ( /* Jan */ \
                           ((BUILDDATE[0] == 'F') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'b')) ?  2 : ( /* Feb */ \
                           ((BUILDDATE[0] == 'M') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'r')) ?  3 : ( /* Mar */ \
                           ((BUILDDATE[0] == 'A') && (BUILDDATE[1] == 'p') && (BUILDDATE[2] == 'r')) ?  4 : ( /* Apr */ \
                           ((BUILDDATE[0] == 'M') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'y')) ?  5 : ( /* May */ \
                           ((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'n')) ?  6 : ( /* Jun */ \
                           ((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'l')) ?  7 : ( /* Jul */ \
                           ((BUILDDATE[0] == 'A') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'g')) ?  8 : ( /* Aug */ \
                           ((BUILDDATE[0] == 'S') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'p')) ?  9 : ( /* Sep */ \
                           ((BUILDDATE[0] == 'O') && (BUILDDATE[1] == 'c') && (BUILDDATE[2] == 't')) ? 10 : ( /* Oct */ \
                           ((BUILDDATE[0] == 'N') && (BUILDDATE[1] == 'o') && (BUILDDATE[2] == 'v')) ? 11 : ( /* Nov */ \
                           ((BUILDDATE[0] == 'D') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'c')) ? 12 : ( /* Dec */ \
                            -1 )))))))))))))
  #define BUILDDATEDAY ((BUILDDATE[4] == ' ' ? '0' : BUILDDATE[4]) * 10 + BUILDDATE[5] - 11*'0')

  time_t nowMoment = time(0), testMoment;
  struct tm nowTm = *localtime(&nowMoment);
  struct tm buildTm = nowTm;
  struct tm testTm;
  buildTm.tm_year = BUILDDATEYEAR - 1900;
  buildTm.tm_mon = BUILDDATEMONTH - 1;
  buildTm.tm_mday = BUILDDATEDAY; // Only field that starts at 1 not zero.
  #undef BUILDDATE
  #undef BUILDDATEYEAR
  #undef BUILDDATEMONTH
  #undef BUILDDATEDAY

  int daysSince = -1;
  do
  {
    testTm = buildTm;
    testTm.tm_mday += ++daysSince;
    testMoment = mktime(&testTm);
  } while (testMoment < nowMoment && (daysSince < maxDays));

  if (ppExpireDateStr)
  {
    static char str[100];
    testTm = buildTm;
    testTm.tm_mday += maxDays;
    time_t expireTime = mktime(&testTm);
    testTm = *localtime(&expireTime);
    udSprintf(str, sizeof(str), "%04d-%02d-%02d", testTm.tm_year + 1900, testTm.tm_mon + 1, testTm.tm_mday);
    *ppExpireDateStr = str;
  }

  return maxDays - daysSince;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrcpy(char *dest, size_t destLen, const char *src)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = s_udStrEmptyString;
  size_t srcChars = strlen(src);
  if ((srcChars + 1) > destLen)
  {
    *dest = 0;
    return 0;
  }
  memcpy(dest, src, srcChars + 1);
  return srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrncpy(char *dest, size_t destLen, const char *src, size_t maxChars)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = s_udStrEmptyString;
  // Find number of characters in string but stop at maxChars (faster than using strlen)
  size_t srcChars = 0;
  while (srcChars < maxChars && src[srcChars])
    ++srcChars;

  if ((srcChars + 1) > destLen)
  {
    *dest = 0;
    return 0;
  }
  memcpy(dest, src, srcChars); // Use crt strcpy as it's likely to be very fast
  dest[srcChars] = 0; // Nul terminate
  return srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrcat(char *pDest, size_t destLen, const char *pSrc)
{
  if (!pDest) return 0;
  if (!pSrc) pSrc = s_udStrEmptyString;

  size_t destChars = strlen(pDest); // Note: Not including terminator
  size_t srcChars = strlen(pSrc);
  if ((destChars + srcChars + 1) > destLen)
    return 0;
  strcat(pDest, pSrc);
  return destChars + srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmp(const char *pStr1, const char *pStr2)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result;
  do
  {
    result = *pStr1 - *pStr2;
  } while (!result && *pStr1++ && *pStr2++);

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmpi(const char *pStr1, const char *pStr2)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result;
  do
  {
    result = tolower(*pStr1) - tolower(*pStr2);
  } while (!result && *pStr1++ && *pStr2++);

  return result;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
int udStrncmp(const char *pStr1, const char *pStr2, size_t maxChars)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result = 0;
  if (maxChars)
  {
    do
    {
      result = *pStr1 - *pStr2;
    } while (!result && *pStr1++ && *pStr2++ && --maxChars);
  }

  return result;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
int udStrncmpi(const char *pStr1, const char *pStr2, size_t maxChars)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result = 0;
  if (maxChars)
  {
    do
    {
      result = tolower(*pStr1) - tolower(*pStr2);
    } while (!result && *pStr1++ && *pStr2++ && --maxChars);
  }

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrlen(const char *pStr)
{
  if (!pStr) pStr = s_udStrEmptyString;

  size_t len = 0;
  while (*pStr++)
    ++len;

  return len;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udStrBeginsWith(const char *pStr, const char *pPrefix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pPrefix) pPrefix = s_udStrEmptyString;

  while (*pPrefix)
  {
    if (*pStr++ != *pPrefix++)
      return false;
  }
  return true;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
bool udStrBeginsWithi(const char *pStr, const char *pPrefix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pPrefix) pPrefix = s_udStrEmptyString;

  while (*pPrefix)
  {
    if (tolower(*pStr++) != tolower(*pPrefix++))
      return false;
  }
  return true;
}

// *********************************************************************
// Author: Dave Pevreal, August 2018
bool udStrEndsWith(const char *pStr, const char *pSuffix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSuffix) pSuffix = s_udStrEmptyString;
  size_t sLen = udStrlen(pStr);
  size_t suffixLen = udStrlen(pSuffix);

  if (sLen < suffixLen)
    return false;
  return udStrcmp(pStr + sLen - suffixLen, pSuffix) == 0;
}

// *********************************************************************
// Author: Dave Pevreal, August 2018
bool udStrEndsWithi(const char *pStr, const char *pSuffix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSuffix) pSuffix = s_udStrEmptyString;
  size_t sLen = udStrlen(pStr);
  size_t suffixLen = udStrlen(pSuffix);

  if (sLen < suffixLen)
    return false;
  return udStrcmpi(pStr + sLen - suffixLen, pSuffix) == 0;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
char *udStrdup(const char *pStr, size_t additionalChars)
{
  if (!pStr && !additionalChars) return nullptr; // This allows us to duplicate null's as null's
  if (!pStr) pStr = s_udStrEmptyString;

  size_t len = udStrlen(pStr) + 1;
  char *pDup = udAllocType(char, len + additionalChars, udAF_None);
  if(pDup)
    memcpy(pDup, pStr, len);

  return pDup;
}


// *********************************************************************
// Author: Dave Pevreal, May 2017
char *udStrndup(const char *pStr, size_t maxChars, size_t additionalChars)
{
  if (!pStr && !additionalChars) return nullptr; // This allows us to duplicate null's as null's
  if (!pStr) pStr = s_udStrEmptyString;
  // Find minimum of maxChars and udStrlen(pStr) without using udStrlen which can be slow
  size_t len = 0;
  while (len < maxChars && pStr[len])
    ++len;
  char *pDup = udAllocType(char, len + 1 + additionalChars, udAF_None);
  if (pDup)
  {
    memcpy(pDup, pStr, len);
    pDup[len] = 0;
  }

  return pDup;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
const char *udStrchr(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t index;
  size_t listLen = udStrlen(pCharList);

  for (index = 0; pStr[index]; ++index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (pStr[index] == pCharList[i])
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        if (pCharListIndex)
          *pCharListIndex = i;
        return pStr + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = index;
  return nullptr;
}


// *********************************************************************
// Author: Samuel Surtees, May 2015
const char *udStrrchr(const char *pStr, const char *pCharList, size_t *pIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t sLen = udStrlen(pStr);
  size_t listLen = udStrlen(pCharList);

  for (ptrdiff_t index = sLen - 1; index >= 0; --index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (pStr[index] == pCharList[i])
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        return pStr + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = sLen;
  return nullptr;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
const char *udStrstr(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSubString) pSubString = s_udStrEmptyString;
  size_t i;
  size_t subStringIndex = 0;

  for (i = 0; pStr[i] && (!sLen || i < sLen); ++i)
  {
    if (pStr[i] == pSubString[subStringIndex])
    {
      if (pSubString[++subStringIndex] == 0)
      {
        // Substring found, move index to beginning of instance
        i = i + 1 - subStringIndex;
        if (pIndex)
          *pIndex = i;
        return pStr + i;
      }
    }
    else
    {
      subStringIndex = 0;
    }
  }

  // Substring not found
  if (pIndex)
    *pIndex = i;
  return nullptr;
}

#if UDPLATFORM_WINDOWS
// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const char *pString)
{
  size_t len = udStrlen(pString) + 1;
  pUTF8 = const_cast<char*>(pString);
  pWide = udAllocType(wchar_t, len, udAF_None);
  pAllocation = pWide;

  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString, -1, pWide, (int)len) == 0)
    *pWide = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const wchar_t *pString)
{
  size_t len = wcslen(pString) + 1;
  size_t allocSize = len * 4;
  pUTF8 = udAllocType(char, allocSize, udAF_None);
  pWide = const_cast<wchar_t*>(pString);
  pAllocation = pUTF8;

  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pString, -1, pUTF8, (int)allocSize, nullptr, nullptr) == 0)
    *pUTF8 = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::~udOSString()
{
  udFree(pAllocation);
}
#endif // UDPLATFORM_WINDOWS

// *********************************************************************
// Author: Dave Pevreal, August 2014
int udStrTokenSplit(char *pLine, const char *pDelimiters, char *pTokenArray[], int maxTokens)
{
  if (pLine == nullptr)
    return 0;

  int tokenCount = 0;
  while (*pLine && tokenCount < maxTokens)
  {
    size_t delimiterIndex;
    pTokenArray[tokenCount++] = pLine;                  // Assign token
    if (udStrchr(pLine, pDelimiters, &delimiterIndex))  // Get the index of the delimiter
    {
      pLine[delimiterIndex] = 0;                        // Null terminate the token
      pLine += delimiterIndex + 1;                      // Move pLine to 1st char after delimiter (possibly another delimiter)
    }
    else
    {
      pLine += delimiterIndex;                          // Move pLine to end of the line
      break;
    }
  }

  // Assign remaining tokens to the end-of-line, this way caller's can
  // depend on all tokens in the array being initialised
  for (int i = tokenCount; i < maxTokens; ++i)
    pTokenArray[i] = pLine;

  return tokenCount;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int32_t udStrAtoi(const char *pStr, int *pCharCount, int radix)
{
  return (int32_t)udStrAtoi64(pStr, pCharCount, radix);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udStrAtou(const char *pStr, int *pCharCount, int radix)
{
  return (uint32_t)udStrAtou64(pStr, pCharCount, radix);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int64_t udStrAtoi64(const char *pStr, int *pCharCount, int radix)
{
  int64_t result = 0;
  int charCount = 0;
  int digitCount = 0;
  bool negate = false;

  if (pStr && radix >= 2 && radix <= 36)
  {
    while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
      ++charCount;
    if (pStr[charCount] == '+')
      ++charCount;
    if (pStr[charCount] == '-')
    {
      negate = true;
      ++charCount;
    }
    result = (int64_t)udStrAtou64(pStr + charCount, &digitCount, radix);
    if (negate)
      result = -result;
  }
  if (pCharCount)
    *pCharCount = charCount + digitCount;
  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint64_t udStrAtou64(const char *pStr, int *pCharCount, int radix)
{
  uint64_t result = 0;
  int charCount = 0;
  if (pStr && radix >= 2 && radix <= 36)
  {
    while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
      ++charCount;
    for (; pStr[charCount]; ++charCount)
    {
      int nextValue = radix; // A sentinal to force end of processing
      if (pStr[charCount] >= '0' && pStr[charCount] <= '9')
        nextValue = pStr[charCount] - '0';
      else if (pStr[charCount] >= 'a' && pStr[charCount] < ('a' + radix - 10))
        nextValue = 10 + (pStr[charCount] - 'a');
      else if (pStr[charCount] >= 'A' && pStr[charCount] < ('A' + radix - 10))
        nextValue = 10 + (pStr[charCount] - 'A');

      if (nextValue >= radix)
        break;

      result = result * radix + nextValue;
    }
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
int udStrUtoa(char *pStr, int strLen, uint64_t value, int radix, int minChars)
{
  int upperCase = (radix < 0) ? 36 : 0;
  radix = udAbs(radix);
  if (radix < 2 || radix > 36)
    return 0;
  static const char *pLetters = "0123456789abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char buf[65]; // Accomodate the largest string which would be 64 binary characters (largest decimal is 20 characters: 18446744073709551615)
  int i = 0;
  while (value || (i < minChars))
  {
    buf[i++] = pLetters[value % radix + upperCase];
    value = value / radix;
  }
  if (pStr)
  {
    int j;
    for (j = 0; j < strLen-1 && j < i; ++j)
    {
      pStr[j] = buf[i-j-1];
    }
    pStr[j] = 0; // nul terminate
  }
  return i; // Return number of characters required, not necessarily number of characters written
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
int udStrItoa(char *pStr, int strLen, int32_t value, int radix, int minChars)
{
  if (!pStr || strLen < 2 || (radix < 2 || radix > 36))
    return 0;
  int minus = 0;
  if (radix != 2 && value < 0) // We don't do + sign for binary
  {
    minus = 1;
    *pStr = '-';
    value = -value;
  }
  return udStrUtoa(pStr + minus, strLen - minus, (uint64_t)value, radix, minChars) + minus;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
int udStrItoa64(char *pStr, int strLen, int64_t value, int radix, int minChars)
{
  if (!pStr || strLen < 2 || (radix < 2 || radix > 36))
    return 0;
  int minus = 0;
  if (radix != 2 && value < 0) // We don't do + sign for binary
  {
    minus = 1;
    *pStr = '-';
    value = -value;
  }
  return udStrUtoa(pStr + minus, strLen - minus, (uint64_t)value, radix, minChars) + minus;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
int udStrFtoa(char *pStr, int strLen, double value, int precision, int minChars)
{
  int64_t temp;
  memcpy(&temp, &value, sizeof(value));
  bool negative = temp < 0; // Only reliable way I've found to detect -0.0
  // Apply the rounding before splitting whole and fraction so that the whole rounds if necessary
  value += pow(10.0, -precision) * (negative ? -0.5 : 0.5);
  // modf does this however it doesn't split the sign out whereas this removes the sign efficiently
  double whole = negative ? -ceil(value) : floor(value);
  double frac = negative ? -(value + whole) : value - whole;

  int charCount = 0;
  if (charCount < (strLen - 1) && negative)
    pStr[charCount++] = '-';
  charCount += udStrUtoa(pStr + charCount, strLen, (uint64_t)whole, 10, udMax(1, minChars - charCount - (precision ? precision + 1 : 0)));
  if (charCount < (strLen-1) && precision > 0)
  {
    pStr[charCount++] = '.';
    while (precision)
    {
      int localPrecision = udMin(precision, 16); // Asciify in small batches to avoid overflowing the whole component of the double
      frac = frac * pow(10.0, localPrecision);
      charCount += udStrItoa64(pStr + charCount, strLen - charCount, (int64_t)frac, 10, localPrecision);
      frac = frac - floor(frac); // no need for proper trunc as frac is always positive
      precision -= localPrecision;
    }
  }

  return charCount;
}

// *********************************************************************
// Author: Dave Pevreal, April 2016
size_t udStrMatchBrace(const char *pLine, char escapeChar)
{
  size_t offset;
  char matchChar;

  switch (*pLine)
  {
    case '{': matchChar = '}'; break;
    case '[': matchChar = ']'; break;
    case '(': matchChar = ')'; break;
    case '<': matchChar = '>'; break;
    case '\"': matchChar = '\"'; break;
    case '\'': matchChar = '\''; break;
    default: return udStrlen(pLine);
  }
  int depth = 1;
  for (offset = 1; pLine[offset]; ++offset)
  {
    if (pLine[offset] == escapeChar && (pLine[offset+1] == matchChar || pLine[offset + 1] == escapeChar))
    {
      ++offset; // Skip escaped characters (only consider match or escape characters)
    }
    else if (pLine[offset] == matchChar)
    {
      if (--depth == 0)
        return offset+1;
    }
    else if (pLine[offset] == *pLine)
    {
      ++depth;
    }
  }
  return offset;
}

// *********************************************************************
// Author: Dave Pevreal, April 2017
const char *udStrSkipWhiteSpace(const char *pLine, int *pCharCount, int *pLineNumber)
{
  int charCount = 0;
  if (pLine)
  {
    while (pLine[charCount] == ' ' || pLine[charCount] == '\t' || pLine[charCount] == '\r' || pLine[charCount] == '\n')
    {
      if (pLineNumber && pLine[charCount] == '\n')
        ++*pLineNumber;
      ++charCount;
    }
  }
  if (pCharCount)
    *pCharCount = charCount;
  return pLine + charCount;
}

// *********************************************************************
// Author: Dave Pevreal, July 2018
const char *udStrSkipToEOL(const char *pLine, int *pCharCount, int *pLineNumber)
{
  int charCount = 0;
  // Skip to NUL, \r or \n
  while (pLine[charCount] != '\0' && pLine[charCount] != '\r' && pLine[charCount] != '\n')
    ++charCount;
  // Do a check for \r\n combination
  if (pLine[charCount] == '\r' && pLine[charCount + 1] == '\n')
    ++charCount;
  // If not a NUL, skip over the EOL character (whatever it may have been)
  if (pLine[charCount] != '\0')
  {
    ++charCount;
    if (pLineNumber)
      ++*pLineNumber;
  }
  if (pCharCount)
    *pCharCount = charCount;
  return pLine + charCount;
}

// *********************************************************************
// Author: Dave Pevreal, April 2016
size_t udStrStripWhiteSpace(char *pLine)
{
  size_t len, i;
  char inQuote = 0; // Use a quote var so later we can handle single quotes if necessary
  for (len = i = 0; pLine[i]; ++i)
  {
    if (pLine[i] == inQuote)
    {
      if (pLine[i-1] != '\\') // Handle escaped quotes in quotes
        inQuote = 0;
    }
    else
    {
      if (pLine[i] == '\"')
        inQuote = pLine[i];
    }
    if (inQuote || (pLine[i] != ' ' && pLine[i] != '\t' && pLine[i] != '\r' && pLine[i] != '\n'))
      pLine[len++] = pLine[i];
  }
  pLine[len++] = pLine[i++];
  return len;
}

// *********************************************************************
// Author: Dave Pevreal, August 2014
const char *udStrEscape(const char *pStr, const char *pCharList, bool freeOriginal)
{
  int escCharCount = 0;
  char *pEscStr = nullptr;
  size_t allocLen;
  for (const char *p = udStrchr(pStr, pCharList); p; p = udStrchr(p + 1, pCharList))
    ++escCharCount;
  if (!escCharCount && freeOriginal)
    return pStr;
  allocLen = udStrlen(pStr) + 1 + escCharCount;
  pEscStr = udAllocType(char, allocLen, udAF_None);
  if (pEscStr)
  {
    const char *pSource = pStr;
    size_t escLen = 0;
    while (*pSource)
    {
      size_t index;
      udStrchr(pSource, pCharList, &index);
      UDASSERT(escLen + index < allocLen, "AllocLen calculation wrong (1)");
      memcpy(pEscStr + escLen, pSource, index);
      escLen += index;
      if (pSource[index])
      {
        UDASSERT(escLen + 2 < allocLen, "AllocLen calculation wrong (2)");
        pEscStr[escLen++] = '\\';
        pEscStr[escLen++] = pSource[index++];
      }
      pSource += index;
    }
    UDASSERT(escLen + 1 == allocLen, "AllocLen calculation wrong (3)");
    pEscStr[escLen++] = 0;
  }
  if (freeOriginal)
    udFree(pStr);
  return pEscStr;
}

// *********************************************************************
// Author: Dave Pevreal, August 2014
float udStrAtof(const char *pStr, int *pCharCount)
{
  if (!pStr) pStr = s_udStrEmptyString;
  int charCount = 0;
  int tmpCharCount = 0;

  float negate = 1.0f;
  while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
    ++charCount;

  // Process negation separately
  if (pStr[charCount] == '-')
  {
    negate = -1.0f;
    ++charCount;
  }

  float result = (float)udStrAtoi(pStr + charCount, &tmpCharCount);
  charCount += tmpCharCount;
  if (pStr[charCount] == '.')
  {
    ++charCount;
    int32_t fraction = udStrAtoi(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    if (result >= 0.f)
      result += fraction / powf(10.f, (float)tmpCharCount);
    else
      result -= fraction / powf(10.f, (float)tmpCharCount);
  }
  if (pStr[charCount] == 'e' || pStr[charCount] == 'E')
  {
    ++charCount;
    float e = (float)udStrAtoi(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    result *= powf(10, e);
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result * negate;
}


// *********************************************************************
// Author: Dave Pevreal, August 2014
double udStrAtof64(const char *pStr, int *pCharCount)
{
  if (!pStr) pStr = s_udStrEmptyString;
  int charCount = 0;
  int tmpCharCount = 0;

  double negate = 1.0f;
  while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
    ++charCount;

  // Process negation separately
  if (pStr[charCount] == '-')
  {
    negate = -1.0f;
    ++charCount;
  }

  double result = (double)udStrAtoi64(pStr + charCount, &tmpCharCount);
  charCount += tmpCharCount;
  if (pStr[charCount] == '.')
  {
    ++charCount;
    int64_t fraction = udStrAtoi64(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    if (result >= 0.0)
      result += fraction / pow(10.0, (double)tmpCharCount);
    else
      result -= fraction / pow(10.0, (double)tmpCharCount);
  }
  if (pStr[charCount] == 'e' || pStr[charCount] == 'E')
  {
    ++charCount;
    double e = (double)udStrAtoi64(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    result *= pow(10, e);
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result * negate;
}

static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// *********************************************************************
// Author: Dave Pevreal, December 2014
udResult udBase64Decode(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength, size_t *pOutputLengthWritten /*= nullptr*/)
{
  udResult result;
  uint32_t accum = 0; // Accumulator for incoming data (read 6 bits at a time)
  int accumBits = 0;
  size_t outputIndex = 0;

  if (!length && pString)
    length = udStrlen(pString);

  if (!pOutput && pOutputLengthWritten)
  {
    outputIndex = length / 4 * 3;
    UD_ERROR_SET(udR_Success);
  }

  UD_ERROR_NULL(pString, udR_InvalidParameter_);
  UD_ERROR_NULL(pOutput, udR_InvalidParameter_);

  for (size_t inputIndex = 0; inputIndex < length; )
  {
    char *c = strchr(b64, pString[inputIndex++]);
    if (c)
    {
      accum |= uint32_t(c - b64) << (16 - accumBits - 6); // Store in accumulator, starting at high byte
      if ((accumBits += 6) >= 8)
      {
        UD_ERROR_IF(outputIndex >= outputLength, udR_BufferTooSmall);
        pOutput[outputIndex++] = uint8_t(accum >> 8);
        accum <<= 8;
        accumBits -= 8;
      }
    }
  }
  result = udR_Success;

epilogue:
  if (pOutputLengthWritten)
    *pOutputLengthWritten = outputIndex;

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, September 2017
udResult udBase64Decode(uint8_t **ppOutput, size_t *pOutputLength, const char * pString)
{
  udResult result;
  uint8_t *pOutput = nullptr;

  UD_ERROR_NULL(ppOutput, udR_InvalidParameter_);
  UD_ERROR_NULL(pOutputLength, udR_InvalidParameter_);
  UD_ERROR_NULL(pString, udR_InvalidParameter_);

  result = udBase64Decode(pString, 0, nullptr, 0, pOutputLength);
  UD_ERROR_HANDLE();
  pOutput = udAllocType(uint8_t, *pOutputLength, udAF_None);
  UD_ERROR_NULL(pOutput, udR_MemoryAllocationFailure);
  result = udBase64Decode(pString, 0, pOutput, *pOutputLength, pOutputLength);
  UD_ERROR_HANDLE();

  *ppOutput = pOutput;
  pOutput = nullptr;
  result = udR_Success;

epilogue:
  udFree(pOutput);
  return result;
}

// *********************************************************************
// Author: Paul Fox, March 2016
udResult udBase64Encode(const void *pBinary, size_t binaryLength, char *pString, size_t strLength, size_t *pOutputLengthWritten /*= nullptr*/)
{
  udResult result;
  uint32_t accum = 0; // Accumulator for data (read 8 bits at a time but only consume 6)
  int accumBits = 0;
  size_t outputIndex = 0;
  size_t expectedOutputLength = (binaryLength + 2) / 3 * 4 + 1; // +1 for nul terminator

  if (!pString && pOutputLengthWritten)
  {
    outputIndex = expectedOutputLength;
    UD_ERROR_SET(udR_Success);
  }

  UD_ERROR_NULL(pString, udR_InvalidParameter_);
  UD_ERROR_NULL(pBinary, udR_InvalidParameter_);

  for (size_t inputIndex = 0; inputIndex < binaryLength; ++inputIndex)
  {
    accum = (accum << 8) | ((uint8_t*)pBinary)[inputIndex];
    accumBits += 8;
    while (accumBits >= 6)
    {
      UD_ERROR_IF(outputIndex >= strLength, udR_BufferTooSmall);
      pString[outputIndex] = b64[((accum >> (accumBits - 6)) & 0x3F)];
      ++outputIndex;
      accumBits -= 6;
    }
  }

  if (accumBits == 2)
  {
    UD_ERROR_IF(outputIndex >= strLength + 3, udR_BufferTooSmall);
    pString[outputIndex] = b64[(accum & 0x3) << 4];
    pString[outputIndex+1] = '='; //Pad chars
    pString[outputIndex+2] = '=';
    outputIndex += 3;
  }
  else if (accumBits == 4)
  {
    UD_ERROR_IF(outputIndex + 2 >= strLength, udR_BufferTooSmall);
    pString[outputIndex] = b64[(accum & 0xF) << 2];
    pString[outputIndex+1] = '='; //Pad chars
    outputIndex += 2;
  }
  pString[outputIndex++] = 0; // Null terminate if room in the string

  UD_ERROR_IF(outputIndex != expectedOutputLength, udR_InternalError); // Okay, the horse may have bolted at this point.

  result = udR_Success;

epilogue:
  if (pOutputLengthWritten)
    *pOutputLengthWritten = outputIndex;

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, May 2017
udResult udBase64Encode(const char **ppDestStr, const void *pBinary, size_t binaryLength)
{
  udResult result;
  size_t expectedOutputLength = (binaryLength + 2) / 3 * 4 + 1; // +1 for nul terminator
  char *pStr = nullptr;

  UD_ERROR_NULL(ppDestStr, udR_InvalidParameter_);
  pStr = udAllocType(char, expectedOutputLength, udAF_None);
  UD_ERROR_NULL(pStr, udR_MemoryAllocationFailure);
  result = udBase64Encode(pBinary, binaryLength, pStr, expectedOutputLength);
  UD_ERROR_HANDLE();
  *ppDestStr = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString)
{
  uint32_t offset = 0;
  int addStrLen = (int)udStrlen(addString); // Length of string to be added

  while (offset < *pStringTableLength)
  {
    int curStrLen = (int)udStrlen(pStringTable + offset);
    if (offset + curStrLen > *pStringTableLength)
      break; // A catch-all in case something's gone wrong
    if (curStrLen >= addStrLen && udStrcmp(pStringTable + offset + curStrLen - addStrLen, addString) == 0)
      return offset + curStrLen - addStrLen; // Found a match
    else
      offset += curStrLen + 1;
  }
  int newLength = offset + addStrLen + 1;
  char *newCache = (char*)udRealloc(pStringTable, newLength);
  if (!newCache)
    return -1; // A nasty case where memory allocation has failed
  strcpy(newCache + offset, addString);
  pStringTable = newCache;
  *pStringTableLength = offset + addStrLen + 1;
  return offset;
}

// *********************************************************************
int udGetHardwareThreadCount()
{
#if UDPLATFORM_WINDOWS
  DWORD_PTR processMask;
  DWORD_PTR systemMask;

  if (::GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
  {
    int hardwareThreadCount = 0;
    while (processMask)
    {
      ++hardwareThreadCount;
      processMask &= processMask - 1; // Clear LSB
    }
    return hardwareThreadCount;
  }

  return 1;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

// *********************************************************************
bool udFilename::SetFromFullPath(const char *pFormat, ...)
{
  *path = 0;
  filenameIndex = 0;
  extensionIndex = 0;
  if (pFormat)
  {
    va_list args;
    va_start(args, pFormat);
    int len = udSprintfVA(path, sizeof(path), pFormat, args);
    va_end(args);
    CalculateIndices();
    if (len > (int)sizeof(path))
      return false;
  }
  return true;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFolder(const char *pFolder)
{
  char newPath[MaxPath];
  size_t i = udStrcpy(newPath, sizeof(path), pFolder);
  if (!i)
    return false;

  // If the path doesn't have a trailing seperator, look for one so we can
  // append one already being used. That is c:\path\ or c:/path/
  if (i > 2 && newPath[i-2] != '/' && newPath[i-2] != '\\' && newPath[i-2] != ':')
  {
    for (--i; i > 0; --i)
    {
      if (newPath[i-1] == '\\' || newPath[i-1] == ':')
      {
        udStrcat(newPath, sizeof(newPath), "\\");
        break;
      }
      if (newPath[i-1] == '/')
      {
        udStrcat(newPath, sizeof(newPath), "/");
        break;
      }
    }
    // Nothing was found so add a /. TODO: Get correct separator from system
    if (i == 0)
      udStrcat(newPath, sizeof(newPath), "/");
  }

  if (!udStrcat(newPath, sizeof(newPath), GetFilenameWithExt()))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameNoExt(const char *pFilenameOnlyComponent)
{
  char newPath[MaxPath];

  ExtractFolder(newPath, sizeof(newPath));
  if (!udStrcat(newPath, sizeof(newPath), pFilenameOnlyComponent))
    return false;
  if (!udStrcat(newPath, sizeof(newPath), GetExt()))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameWithExt(const char *pFilenameWithExtension)
{
  char newPath[MaxPath];

  ExtractFolder(newPath, sizeof(newPath));
  if (!udStrcat(newPath, sizeof(newPath), pFilenameWithExtension))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetExtension(const char *pExtComponent)
{
  char newPath[MaxPath];

  if (!udStrcpy(newPath, sizeof(newPath), path))
    return false;
  newPath[extensionIndex] = 0; // Truncate the extension
  if (!udStrcat(newPath, sizeof(newPath), pExtComponent))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFolder(char *pFolder, int folderLen)
{
  int folderChars = filenameIndex;
  if (pFolder)
    udStrncpy(pFolder, folderLen, path, folderChars);
  return folderChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFilenameOnly(char *pFilename, int filenameLen)
{
  int filenameChars = extensionIndex - filenameIndex;
  if (pFilename)
  {
    udStrncpy(pFilename, filenameLen, path + filenameIndex, filenameChars);
  }
  return filenameChars + 1;
}

// ---------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
void udFilename::CalculateIndices()
{
  int len = (int)strlen(path);
  // Set filename and extension indices to null terminator as a sentinal
  filenameIndex = -1;
  extensionIndex = len; // If no extension, point extension to nul terminator

  for (--len; len > 0 && (filenameIndex == -1 || extensionIndex == -1); --len)
  {
    if (path[extensionIndex] == 0 && path[len] == '.') // Last period
      extensionIndex = len;
    if (filenameIndex == -1 && (path[len] == '/' || path[len] == '\\' || path[len] == ':'))
      filenameIndex = len + 1;
  }
  // If no path indicators were found the filename is the beginning of the path
  if (filenameIndex == -1)
    filenameIndex = 0;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
void udFilename::Debug()
{
  char folder[260];
  char name[50];

  ExtractFolder(folder, sizeof(folder));
  ExtractFilenameOnly(name, sizeof(name));
  udDebugPrintf("folder<%s> name<%s> ext<%s> filename<%s> -> %s\n", folder, name, GetExt(), GetFilenameWithExt(), path);
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
udResult udURL::SetURL(const char *pURL)
{
  udFree(pURLText);
  pScheme = s_udStrEmptyString;
  pDomain = s_udStrEmptyString;
  pPath = s_udStrEmptyString;
  static const char specialChars[]   = { ' ',  '#',   '%',   '+',   '?',   '\0', }; // Made for extending later, not wanting to encode any more than we need to
  static const char *pSpecialSubs[] = { "%20", "%23", "%25", "%2B", "%3F", "", };

  if (pURL)
  {
    // Take a copy of the entire string
    char *p = pURLText = udStrdup(pURL, udStrlen(pURL) * 3 + 2); // Add an extra chars for nul terminate domain, and URL encoding switches for every character (worst case)
    if (!pURLText)
      return udR_MemoryAllocationFailure;

    size_t i, charListIndex;

    // Isolate the scheme
    udStrchr(p, ":/", &i); // Find the colon that breaks the scheme, but don't search past a slash

    if (p[i] == ':') // Test in case of malformed url (potentially allowing a default scheme such as 'http'
    {
      pScheme = p;
      p[i] = 0; // null terminate the scheme
      p = p + i + 1;
      // Skip over the // at start of domain (if present)
      if (p[0] == '/' && p[1] == '/')
        p += 2;
    }

    // Isolate the domain - this is slightly painful because ipv6 has colons
    pDomain = p;
    udStrchr(p, p[0] == '[' ? "/]" : "/:", &i); // Find the character that breaks the domain, but don't search past a slash
    if (p[0] == '[' && p[i] == ']') ++i; // Skip over closing bracket of ipv6 address
    if (p[i] == ':')
    {
      // A colon is present, so decode the port number
      int portChars;
      port = udStrAtoi(&p[i+1], &portChars);
      p[i] = 0; // null terminate the domain
      i += portChars + 1;
    }
    else
    {
      // Otherwise assume port 443 for https, or 80 for anything else (should be http)
      port = udStrEqual(pScheme, "https") ? 443 : 80;
      // Because no colon character exists to terminate the domain, move it forward by 1 byte
      if (p[i] != 0)
      {
        memmove(p + i + 1, p + i, udStrlen(p + i) + 1); // Move the string one to the right to retain the separator (note: 1 byte was added to allocation when udStrdup called)
        p[i++] = 0; // null terminate the domain
      }
    }
    p += i;

    // Finally, the path is the last component (for now, unless the class is extended to support splitting the query string too)
    pPath = p;

    // Now, find any "special" URL characters in the path and encode them
    while ((p = (char*)udStrchr(p, specialChars, nullptr, &charListIndex)) != nullptr)
    {
      size_t len = udStrlen(pSpecialSubs[charListIndex]);
      memmove(p + len - 1, p, udStrlen(p) + 1);
      memcpy(p, pSpecialSubs[charListIndex], len);
      p += len;
    }
  }

  return udR_Success; // TODO: Perhaps return an error if the url isn't formed properly
}


#pragma pack(push)
#pragma pack(2)
struct udBMPHeader
{
    uint16_t  bfType;            // must be 'BM'
    uint32_t  bfSize;            // size of the whole .bmp file
    uint16_t  bfReserved1;       // must be 0
    uint16_t  bfReserved2;       // must be 0
    uint32_t  bfOffBits;

    uint32_t  biSize;            // size of the structure
    int32_t   biWidth;           // image width
    int32_t   biHeight;          // image height
    uint16_t  biPlanes;          // bitplanes
    uint16_t  biBitCount;        // resolution
    uint32_t  biCompression;     // compression
    uint32_t  biSizeImage;       // size of the image
    int32_t   biXPelsPerMeter;   // pixels per meter X
    int32_t   biYPelsPerMeter;   // pixels per meter Y
    uint32_t  biClrUsed;         // colors used
    uint32_t  biClrImportant;    // important colors
};
#pragma pack(pop)

// ***************************************************************************************
// Author: Dave Pevreal, August 2014
udResult udSaveBMP(const char *pFilename, int width, int height, uint32_t *pColorData, int pitchInBytes)
{
  udBMPHeader header;
  memset(&header, 0, sizeof(header));
  if (!pitchInBytes)
    pitchInBytes = width * 4;

  udFile *pFile = nullptr;
  int paddedLineSize = (width * 3 + 3) & ~3;
  udResult result = udR_MemoryAllocationFailure;
  uint8_t *pLine = udAllocType(uint8_t, paddedLineSize, udAF_Zero);
  if (!pLine)
    goto error;

  header.bfType = 0x4d42;       // 0x4d42 = 'BM'
  header.bfReserved1 = 0;
  header.bfReserved2 = 0;
  header.bfSize = sizeof(header) + paddedLineSize * height;
  header.bfOffBits = 0x36;
  header.biSize = 0x28; // sizeof(BITMAPINFOHEADER);
  header.biWidth = width;
  header.biHeight = height;
  header.biPlanes = 1;
  header.biBitCount = 24;
  header.biCompression = 0; /*BI_RGB*/
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0x0ec4;
  header.biYPelsPerMeter = 0x0ec4;
  header.biClrUsed = 0;
  header.biClrImportant = 0;

  result = udFile_Open(&pFile, pFilename, udFOF_Write | udFOF_Create);
  if (result != udR_Success)
    goto error;

  result = udFile_Write(pFile, &header, sizeof(header));
  if (result != udR_Success)
    goto error;

  for (int y = height - 1; y >= 0 ; --y)
  {
    for (int x = 0; x < width; ++x)
      memcpy(&pLine[x*3], &((uint8_t*)pColorData)[y * pitchInBytes + x * 4], 3);

    result = udFile_Write(pFile, pLine, paddedLineSize);
    if (result != udR_Success)
      goto error;
  }

error:
  udFree(pLine);
  udFile_Close(&pFile);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2014
udResult udLoadBMP(const char *pFilename, int *pWidth, int *pHeight, uint32_t **ppColorData)
{
  udResult result;
  udBMPHeader header;
  memset(&header, 0, sizeof(header));
  udFile *pFile = nullptr;
  uint8_t *pColors = nullptr;
  uint8_t *pLine = nullptr;
  int paddedLineSize;

  UD_ERROR_NULL(pFilename, udR_InvalidParameter_);
  UD_ERROR_NULL(ppColorData, udR_InvalidParameter_);
  UD_ERROR_IF(!pWidth || !pHeight, udR_InvalidParameter_);

  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Read));
  UD_ERROR_CHECK(udFile_Read(pFile, &header, sizeof(header)));

  *pWidth = header.biWidth;
  *pHeight = header.biHeight;
  paddedLineSize = (*pWidth * 3 + 3) & ~3;
  pColors = udAllocType(uint8_t, *pWidth * *pHeight * 4, udAF_None);
  UD_ERROR_NULL(pColors, udR_MemoryAllocationFailure);
  pLine = udAllocType(uint8_t, paddedLineSize, udAF_None);
  UD_ERROR_NULL(pLine, udR_MemoryAllocationFailure);

  for (int y = *pHeight - 1; y >= 0 ; --y)
  {
    UD_ERROR_CHECK(udFile_Read(pFile, pLine, paddedLineSize));
    uint8_t *p = pColors + y * *pWidth * 4;
    for (int x = 0; x < *pWidth; ++x)
    {
      *p++ = pLine[x * 3 + 0];
      *p++ = pLine[x * 3 + 1];
      *p++ = pLine[x * 3 + 2];
      *p++ = 0xff;
    }
  }

  *ppColorData = (uint32_t*)pColors;
  pColors = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFile_Close(&pFile);
  udFree(pLine);
  udFree(pColors);

  return result;
}

#define SMALLSTRING_BUFFER_COUNT 32
#define SMALLSTRING_BUFFER_SIZE 64
static char s_smallStringBuffers[SMALLSTRING_BUFFER_COUNT][SMALLSTRING_BUFFER_SIZE]; // 32 cycling buffers of 64 characters
static volatile int32_t s_smallStringBufferIndex = 0;  // Cycling index, always and with (SMALLSTRING_BUFFER_COUNT-1) to get buffer index

// ****************************************************************************
// Author: Dave Pevreal, May 2018
const char *udTempStr(const char *pFormat, ...)
{
  int32_t bufIndex = udInterlockedPostIncrement(&s_smallStringBufferIndex) & (SMALLSTRING_BUFFER_COUNT - 1);
  size_t bufferSize = SMALLSTRING_BUFFER_SIZE;

retry:
  UDASSERT(bufIndex < SMALLSTRING_BUFFER_COUNT, "buffer index out of range");
  UDASSERT(bufIndex * SMALLSTRING_BUFFER_SIZE + bufferSize <= (int)sizeof(s_smallStringBuffers), "bufferSize would lead to overrun");
  char *pBuf = s_smallStringBuffers[bufIndex];
  va_list args;
  va_start(args, pFormat);
  int charCount = udSprintfVA(pBuf, bufferSize, pFormat, args);
  va_end(args);

  if (charCount >= (int)bufferSize && bufferSize < (SMALLSTRING_BUFFER_COUNT * SMALLSTRING_BUFFER_SIZE))
  {
    // The output buffer wasn't big enough, so look for a series of contiguous buffers
    // To keep things simple, attempt to undo the allocation done on the first line of the function
    int previous = bufIndex | (s_smallStringBufferIndex & ~(SMALLSTRING_BUFFER_COUNT - 1));
    // Reset to previous iff current value is exactly previous + 1
    udInterlockedCompareExchange(&s_smallStringBufferIndex, previous, previous + 1);

    int requiredBufferCount = udMin(SMALLSTRING_BUFFER_COUNT, (charCount + SMALLSTRING_BUFFER_SIZE) / SMALLSTRING_BUFFER_SIZE);
    bufferSize = requiredBufferCount * SMALLSTRING_BUFFER_SIZE;
    // Try to allocate a number of sequential buffers, understanding that another thread can allocate one also
    while ((((bufIndex = s_smallStringBufferIndex) & (SMALLSTRING_BUFFER_COUNT - 1)) + requiredBufferCount) <= SMALLSTRING_BUFFER_COUNT)
    {
      if (udInterlockedCompareExchange(&s_smallStringBufferIndex, bufIndex + requiredBufferCount, bufIndex) == bufIndex)
      {
        // The bufIndex has upper bits set for the compareExchange, clear them before retrying
        bufIndex &= (SMALLSTRING_BUFFER_COUNT - 1);
        goto retry;
      }
    }
    // We need to wrap and hopefully no other thread is still using their string.
    if (requiredBufferCount > (s_smallStringBufferIndex & (SMALLSTRING_BUFFER_COUNT - 1)))
      udDebugPrintf("Warning: very long string (%d chars) created using udTempStr - NOT THREADSAFE\n", charCount + 1);
    s_smallStringBufferIndex = requiredBufferCount;
    bufIndex = 0;
    goto retry;
  }
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, October 2015
const char *udTempStr_CommaInt(int64_t n)
{
  char *pBuf = s_smallStringBuffers[udInterlockedPostIncrement(&s_smallStringBufferIndex) & (SMALLSTRING_BUFFER_COUNT-1)];
  uint64_t v = (uint64_t)n;

  int i = 0;
  if (n < 0)
  {
    pBuf[i++] = '-';
    v = (uint64_t)-n;
  }

  // Get the column variable to be the first column containing anything
  uint64_t col = 1;
  int digitCount = 1;
  while (v >= col * 10)
  {
    col = col * 10;
    ++digitCount;
  }
  do
  {
    uint64_t c = v / col;
    pBuf[i++] = (char)('0' + c);
    if ((--digitCount % 3) == 0 && digitCount)
      pBuf[i++] = ',';
    v -= c * col;
    col = col / 10;
  } while (digitCount);
  pBuf[i++] = 0;

  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, September 2018
const char *udTempStr_TrimDouble(double v, int maxDecimalPlaces, int minDecimalPlaces, bool undoRounding)
{
  char *pBuf = s_smallStringBuffers[udInterlockedPostIncrement(&s_smallStringBufferIndex) & (SMALLSTRING_BUFFER_COUNT - 1)];
  udStrFtoa(pBuf, SMALLSTRING_BUFFER_SIZE, v, maxDecimalPlaces + (undoRounding ? 1 : 0));
  size_t pointIndex;
  if (udStrchr(pBuf, ".", &pointIndex))
  {
    size_t i = udStrlen(pBuf) - 1;
    // If requested, truncate the last decimal place (to undo rounding)
    if (undoRounding && i > pointIndex)
      pBuf[i--] = '\0';
    for (; i > (pointIndex + minDecimalPlaces); --i)
    {
      if (pBuf[i] == '0')
        pBuf[i] = '\0';
      else
        break;
    }
    if (minDecimalPlaces == 0 && pBuf[pointIndex + 1] == '\0')
      pBuf[pointIndex] = '\0';
  }
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
const char *udTempStr_ElapsedTime(int seconds, bool trimHours)
{
  int hours = seconds / (60 * 60);
  int minutes = (seconds / 60) % 60;
  int secs = seconds % 60;
  const char *pBuf = udTempStr("%d:%02d:%02d", hours, minutes, secs);
  if (trimHours && !hours)
    pBuf += 2; // Skip leading 0: when hours is zero
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
const char *udTempStr_HumanMeasurement(double measurement)
{
  static const char *pSuffixStrings[] = { "m", "cm", "mm" };
  static double suffixMult[] = { 1, 100, 1000 };
  size_t suffixIndex = 0;
  while ((suffixIndex + 1) < UDARRAYSIZE(pSuffixStrings) && (measurement * suffixMult[suffixIndex]) < 1.0)
    ++suffixIndex;

  // Generate float scale to 6 decimal places
  char temp[32];
  int charCount = udStrFtoa(temp, sizeof(temp), measurement * suffixMult[suffixIndex], 6);

  // Trim unnecessary trailing zeros or decimal point for human friendly number
  while (charCount > 1)
  {
    char c = temp[--charCount];
    if (c == '0' || c == '.')
      temp[charCount] = 0;
    if (c != '0')
      break;
  }
  return udTempStr("%s%s", temp, pSuffixStrings[suffixIndex]);
}

// ----------------------------------------------------------------------------
struct udFindDirData : public udFindDir
{
#if UDPLATFORM_WINDOWS
  HANDLE hFind;
  WIN32_FIND_DATAW findFileData;
  char utf8Filename[2048];

  void SetMembers()
  {
    // Take a copy of the filename after translation from wide-char to utf8
    udStrcpy(utf8Filename, sizeof(utf8Filename), udOSString(findFileData.cFileName));
    pFilename = utf8Filename;
    isDirectory = !!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  DIR *pDir;
  struct dirent *pDirent;

  void SetMembers()
  {
    pFilename = pDirent->d_name;
    isDirectory = !!(pDirent->d_type & DT_DIR);
  }
#elif UDPLATFORM_NACL
  //Do nothing
#else
#error "Unsupported platform"
#endif
};

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes)
{
#if UD_32BIT
# define UD_STAT_STRUCT stat
# define UD_STAT_FUNC stat
#elif UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS
  // Apple made these 64bit and deprecated the 64bit variants
# define UD_STAT_STRUCT stat
# define UD_STAT_FUNC stat
#elif UDPLATFORM_WINDOWS
# define UD_STAT_STRUCT _stat64
# define UD_STAT_FUNC _wstat64
#else
# define UD_STAT_STRUCT stat64
# define UD_STAT_FUNC stat64
#endif

  struct UD_STAT_STRUCT st;
  memset(&st, 0, sizeof(st));
  if (UD_STAT_FUNC(udOSString(pFilename), &st) == 0)
  {
    if (pFileLengthInBytes)
      *pFileLengthInBytes = (int64_t)st.st_size;
    return udR_Success;
  }
  else
  {
    return udR_ObjectNotFound;
  }

#undef UD_STAT_STRUCT
#undef UD_STAT_FUNC
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileDelete(const char *pFilename)
{
  return remove(pFilename) == -1 ? udR_Failure_ : udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder)
{
  udResult result;
  udFindDirData *pFindData = nullptr;

  pFindData = udAllocType(udFindDirData, 1, udAF_Zero);
  UD_ERROR_NULL(pFindData, udR_MemoryAllocationFailure);

#if UDPLATFORM_WINDOWS
  {
    udFilename fn;
    fn.SetFolder(pFolder);
    fn.SetFilenameWithExt("*.*");
    pFindData->hFind = FindFirstFileW(udOSString(fn.GetPath()), &pFindData->findFileData);
    UD_ERROR_IF(pFindData->hFind == INVALID_HANDLE_VALUE, udR_OpenFailure);
    pFindData->SetMembers();
  }
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  pFindData->pDir = opendir(pFolder);
  UD_ERROR_NULL(pFindData->pDir, udR_OpenFailure);
  pFindData->pDirent = readdir(pFindData->pDir);
  UD_ERROR_NULL(pFindData->pDirent, udR_ObjectNotFound);
  pFindData->SetMembers();
#elif UDPLATFORM_NACL
  // TODO: See if this implementation is required
  UD_ERROR_SET(udR_ObjectNotFound);
#else
  #error "Unsupported Platform"
#endif

  result = udR_Success;
  *ppFindDir = pFindData;
  pFindData = nullptr;

epilogue:
  if (pFindData)
    udCloseDir((udFindDir**)&pFindData);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udReadDir(udFindDir *pFindDir)
{
  if (!pFindDir)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  if (!FindNextFileW(pFindData->hFind, &pFindData->findFileData))
    return udR_ObjectNotFound;
  pFindData->SetMembers();
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  pFindData->pDirent = readdir(pFindData->pDir);
  if (!pFindData->pDirent)
    return udR_ObjectNotFound;
  pFindData->SetMembers();
#elif UDPLATFORM_NACL
  // Do nothing
#else
  #error "Unsupported Platform"
#endif
  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udCloseDir(udFindDir **ppFindDir)
{
  if (!ppFindDir || !*ppFindDir)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->hFind != INVALID_HANDLE_VALUE)
    FindClose(pFindData->hFind);
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->pDir)
    closedir(pFindData->pDir);
#elif UDPLATFORM_NACL
  // Do nothing
#else
  #error "Unsupported Platform"
#endif

  udFree(*ppFindDir);
  return udR_Success;
}

// ****************************************************************************
// Author: Samuel Surtees, May 2016
udResult udCreateDir(const char *pFolder)
{
  udResult ret = udR_Success;

  // TODO: Handle creating intermediate directories that don't exist already?
  // TODO: Have udFile_Open call this for the user when udFOF_Create is used?
#if UDPLATFORM_WINDOWS
  // Returns 0 on fail
  if (CreateDirectoryW(udOSString(pFolder), NULL) == 0)
#else
  // Returns -1 on fail
  if (mkdir(pFolder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
#endif
    ret = udR_Failure_;

  return ret;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2018
udResult udRemoveDir(const char *pFolder)
{
#if UDPLATFORM_WINDOWS
  // Returns 0 on fail
  if (RemoveDirectoryW(udOSString(pFolder)) == 0)
    return udR_Failure_;
#else
  // Returns -1 on fail
  if (rmdir(pFolder) != 0)
    return udR_Failure_;
#endif

  return udR_Success;
}

// ****************************************************************************
// Author: Paul Fox, September 2015
int udSprintf(char *pDest, size_t destlength, const char *pFormat, ...)
{
  va_list args;
  va_start(args, pFormat);
  int length = udSprintfVA(pDest, destlength, pFormat, args);
  va_end(args);

  return length;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2017
int udSprintfVA(char *pDest, size_t destLength, const char *pFormat, va_list args)
{
#if 1
  return vsnprintf(pDest, destLength, pFormat, args);
#else
  int errorCode = 0; // -1 == unknown specifier, -2 == unexpected width/precision specifier
  size_t length = 0;
  if (!pDest || !pFormat)
    return 0;
  // Keep processing until we're out of format string or destination string
  while (*pFormat && !errorCode)
  {
    if (*pFormat == '%')
    {
      char padChar = ' ';
      bool hashSpec = false; // # prefixes hex numbers with 0x or forces point for decimals
      bool longSpec = false;
      bool leftJustify = false;
      bool forcePlus = false;
      bool widthSpec = false;
      size_t width = 0;
      int precision = 1;
      bool precisionSpec = false; // Need a flag to know precision was specified, as floats have a different default value (6)
      ++pFormat;

      while (*pFormat)
      {
        const char *pInjectStr = nullptr;
        size_t injectLen = 0;
        int charCount = 0;
        char numericBuffer[70]; // enough characters for any known number (64) plus 0x or decimal etc

        charCount = 1; // As a default unless a number is processed
        // Process formatted string
        switch (*pFormat)
        {
          case '0': padChar = '0'; break;
          case 'l': longSpec = true; break;
          case '+': forcePlus = true; break;
          case '-': leftJustify = true; break;
          case '#': hashSpec = true; break;
          case '%': pInjectStr = pFormat; injectLen = 1; break;
          case '*': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {
              int value = *pFormat == '*' ? udAbs(va_arg(args, int)) : udStrAtoi(pFormat, &charCount);
              if (!widthSpec)
                width = (size_t)value;
              else
                errorCode = -2;
              widthSpec = true;
            }
            break;
          case '.':
            {
              ++pFormat;
              int value = *pFormat == '*' ? udAbs(va_arg(args, int)) : udStrAtoi(pFormat, &charCount);
              precision = (size_t)value;
              precisionSpec = true;
            }
            break;
          case 'c':
            {
              numericBuffer[0] = (char)va_arg(args, int); // Chars are passed as integers
              pInjectStr = numericBuffer;
              injectLen = 1;
            }
            break;
          case 's':
            {
              pInjectStr = va_arg(args, char*);
              injectLen = udStrlen(pInjectStr);
            }
            break;
          case 'd':
          case 'i':
            if (longSpec)
              udStrItoa64(numericBuffer, sizeof(numericBuffer), va_arg(args, int64_t), 10, precision);
            else
              udStrItoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint32_t), 10, precision);
            pInjectStr = numericBuffer;
            injectLen = udStrlen(pInjectStr);
            break;
          case 'u':
            if (longSpec)
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint64_t), 10, precision);
            else
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint32_t), 10, precision);
            pInjectStr = numericBuffer;
            injectLen = udStrlen(pInjectStr);
            break;
          case 'x':
          case 'X':
            udStrcpy(numericBuffer, sizeof(numericBuffer), isupper(*pFormat) ? "0X" : "0x");
            if (longSpec)
              udStrUtoa(numericBuffer + 2, sizeof(numericBuffer) - 2, va_arg(args, uint64_t), isupper(*pFormat) ? -16 : 16, precision);
            else
              udStrUtoa(numericBuffer + 2, sizeof(numericBuffer) - 2, va_arg(args, uint32_t), isupper(*pFormat) ? -16 : 16, precision);
            pInjectStr = numericBuffer + ((hashSpec) ? 0 : 2);
            injectLen = udStrlen(pInjectStr);
            break;
          case 'b':
            if (longSpec)
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint64_t), 2, precision);
            else
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint32_t), 2, precision);
            pInjectStr = numericBuffer;
            injectLen = udStrlen(pInjectStr);
            break;
          case 'p':
          case 'P':
            if (sizeof(pInjectStr) == 8)
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint64_t), isupper(*pFormat) ? -16 : 16, precision);
            else
              udStrUtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, uint32_t), isupper(*pFormat) ? -16 : 16, precision);
            pInjectStr = numericBuffer;
            injectLen = udStrlen(pInjectStr);
            break;
          case 'f':
            udStrFtoa(numericBuffer, sizeof(numericBuffer), va_arg(args, double), precisionSpec ? precision : 6, padChar == ' ' ? 1 : udMax((int)width, 1));
            pInjectStr = numericBuffer;
            injectLen = udStrlen(pInjectStr);
            break;

          default:
            errorCode = -1;
        }
        pFormat += charCount;

        if (pInjectStr)
        {
          size_t padLen = (width > injectLen) ? width - injectLen : 0;
          if (padLen && !leftJustify && padChar == ' ')
          {
            for (size_t i = 0; i < padLen && (length + i) < destLength; ++i)
              pDest[length + i] = padChar;
            length += padLen;
          }
          if (injectLen)
          {
            for (size_t i = 0; i < injectLen && (length + i) < destLength; ++i)
              pDest[length + i] = pInjectStr[i];
            length += injectLen;
          }
          if (padLen && leftJustify)
          {
            for (size_t i = 0; i < padLen && (length + i) < destLength; ++i)
              pDest[length + i] = ' '; // Always use spaces for post-padding
            length += padLen;
          }
          break;
        }
      }
    }
    else
    {
      if (length < destLength)
        pDest[length] = *pFormat;
      ++length;
      ++pFormat;
    }
  }
  if (length < destLength)
    pDest[length] = '\0';
  else if (destLength > 0)
    pDest[destLength - 1] = '\0';

  return errorCode ? errorCode : (int)length;
#endif
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udSprintf(const char **ppDest, const char *pFormat, ...)
{
  size_t length;
  char *pStr = nullptr;

  va_list ap;
  va_start(ap, pFormat);
  length = udSprintfVA(nullptr, 0, pFormat, ap);
  va_end(ap);

  pStr = udAllocType(char, length + 1, udAF_None);
  if (!pStr)
    return udR_MemoryAllocationFailure;

  va_start(ap, pFormat);
  udSprintfVA(pStr, length + 1, pFormat, ap);
  va_end(ap);
  udFree(*ppDest);
  *ppDest = pStr;

  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
udResult udParseWKT(udJSON *pValue, const char *pString, int *pCharCount)
{
  udResult result = udR_Success;;
  size_t idLen;
  udJSON temp;
  int tempCharCount = 0;
  int parameterNumber = 0;
  const char *pStartString = pString;

  UD_ERROR_NULL(pValue, udR_InvalidParameter_);
  UD_ERROR_NULL(pString, udR_InvalidParameter_);

  pString = udStrSkipWhiteSpace(pString);
  while (*pString && *pString != ']')
  {
    g_udBreakOnError = false;
    udResult parseResult = temp.Parse(pString, &tempCharCount);
    g_udBreakOnError = true;
    if (parseResult == udR_Success)
    {
      if (!parameterNumber && temp.IsString())
        pValue->Set(&temp, "name");
      else
        pValue->Set(&temp, "values[]");
      ++parameterNumber;
      pString += tempCharCount;
    }
    else
    {
      // Assume an object identifier, or an unquoted constant
      udStrchr(pString, "[],", &idLen);
      if (pString[idLen] == '[')
      {
        temp.Set("type = '%.*s'", (int)idLen, pString);
        result = udParseWKT(&temp, pString + idLen + 1, &tempCharCount);
        UD_ERROR_HANDLE();
        pValue->Set(&temp, "values[]");
        pString += idLen + 1 + tempCharCount;
      }
      else // Any non-parsable is now considered a string, eg AXIS["Easting",EAST] parses as AXIS["Easting","EAST"]
      {
        pValue->Set("values[] = '%.*s'", (int)idLen, pString);
        ++parameterNumber;
        pString += idLen;
      }
    }
    if (*pString == ',')
      ++pString;
    pString = udStrSkipWhiteSpace(pString);
  }
  if (*pString == ']')
    ++pString;
  if (pCharCount)
    *pCharCount = int(pString - pStartString);

epilogue:
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
static udResult GetWKTElementStr(const char **ppOutput, const udJSON &value)
{
  udResult result;
  const char *pElementSeperator = ""; // Changed to "," after first element is written
  const char *pStr = nullptr;
  const char *pElementStr = nullptr;
  const char *pTypeStr;
  const char *pNameStr;
  size_t valuesCount;

  pTypeStr = value.Get("type").AsString();
  UD_ERROR_NULL(pTypeStr, udR_ParseError);
  pNameStr = value.Get("name").AsString();
  valuesCount = value.Get("values").ArrayLength();
  UD_ERROR_CHECK(udSprintf(&pStr, "%s[", pTypeStr));
  if (pNameStr)
  {
    UD_ERROR_CHECK(udSprintf(&pStr, "%s\"%s\"", pStr, pNameStr));
    pElementSeperator = ",";
  }
  for (size_t i = 0; i < valuesCount; ++i)
  {
    const udJSON &arrayValue = value.Get("values[%d]", (int)i);
    if (arrayValue.IsObject())
      UD_ERROR_CHECK(GetWKTElementStr(&pElementStr, arrayValue));
    else if (arrayValue.IsString() && i == 0 && udStrEqual(pTypeStr, "AXIS")) // Special case for AXIS, output second string unquoted
      UD_ERROR_CHECK(udSprintf(&pElementStr, "%s", arrayValue.AsString()));
    else if (arrayValue.IsString())
      UD_ERROR_CHECK(udSprintf(&pElementStr, "\"%s\"", arrayValue.AsString()));
    else
      UD_ERROR_CHECK(arrayValue.ToString(&pElementStr));
    UD_ERROR_CHECK(udSprintf(&pStr, "%s%s%s", pStr, pElementSeperator, pElementStr));
    udFree(pElementStr);
    pElementSeperator = ",";
  }
  // Put the closing brace on
  UD_ERROR_CHECK(udSprintf(&pStr, "%s]", pStr));

  // Transfer ownership
  *ppOutput = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  udFree(pElementStr);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
udResult udExportWKT(const char **ppOutput, const udJSON *pValue)
{
  udResult result;
  const char *pStr = nullptr;
  const char *pElementStr = nullptr;
  const char *pTemp = nullptr;
  size_t elemCount;

  UD_ERROR_NULL(ppOutput, udR_InvalidParameter_);
  UD_ERROR_NULL(pValue, udR_InvalidParameter_);

  elemCount = pValue->Get("values").ArrayLength();
  for (size_t i = 0; i < elemCount; ++i)
  {
    UD_ERROR_CHECK(GetWKTElementStr(&pElementStr, pValue->Get("values[%d]", (int)i)));
    if (!pStr)
    {
      pStr = pElementStr;
      pElementStr = nullptr;
    }
    else
    {
      pTemp = pStr;
      pStr = nullptr;
      UD_ERROR_CHECK(udSprintf(&pStr, "%s%s", pTemp, pElementStr));
    }
    udFree(pElementStr);
    udFree(pTemp);
  }
  // Transfer ownership
  *ppOutput = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  udFree(pElementStr);
  udFree(pTemp);
  return result;
}
