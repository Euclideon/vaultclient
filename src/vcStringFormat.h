#ifndef vcStringFormat_H__
#define vcStringFormat_H__

#include "udPlatform.h"

// Returns formatted string
const char* vcStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings);
const char* vcStringFormat(const char *pFormatString, const char *pStrings); // Helper for the case where there is only 1 string

// Outputs as much as will fit along with a null terminator, pTotalWritten if set will be the total number of input characters written out, not including the null terminator
const char* vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char **ppStrings, size_t numStrings, size_t *pTotalWritten = nullptr);
const char* vcStringFormat(char *pBuffer, size_t bufLen, const char *pFormatString, const char *pString, size_t *pTotalWritten = nullptr);

#endif // vcStringFormat_H__
