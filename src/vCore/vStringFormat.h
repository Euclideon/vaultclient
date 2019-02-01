#ifndef vStringFormat_H__
#define vStringFormat_H__

#include "udPlatform/udPlatform.h"

// Returns formatted string
const char* vStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings);
const char* vStringFormat(const char *pFormatString, const char *pStrings); // Helper for the case where there is only 1 string

const char* vStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings);
const char* vStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString);

#endif // vStringFormat_H__
