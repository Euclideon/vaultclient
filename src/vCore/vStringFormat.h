#ifndef vStringFormat_H__
#define vStringFormat_H__

// Returns formatted string, or something
const char *vStringFormat(const char *pFormatString, const char **ppStrings, size_t numStrings);
const char *vStringFormat(const char *pFormatString, const char *pStrings); // Helper for the case where there is only 1 string

#endif // vStringFormat_H__
