#ifndef vcStringFormat_H__
#define vcStringFormat_H__

#include "udPlatform.h"

// Returns formatted string
const char* vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings);
const char* vcStringFormat(const char *pFormatString, const char *pStrings); // Helper for the case where there is only 1 string

const char* vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings, size_t *pOutputLen = nullptr);
const char* vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString);

#endif // vcStringFormat_H__
