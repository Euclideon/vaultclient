#ifndef UDPLATFORM_UTIL_H
#define UDPLATFORM_UTIL_H

#include "udPlatform.h"
#include <stdio.h>
#include <stdarg.h>

class udJSON;

#define UDNAMEASSTRING(s) #s
#define UDSTRINGIFY(s) UDNAMEASSTRING(s)


// *********************************************************************
// Some simple utility template functions
// *********************************************************************

template <typename SourceType, typename DestType>
inline DestType udCastToTypeOf(const SourceType &source, const DestType &) { return DestType(source); }

// A utility macro to isolate a bit-field style value from an integer, using an enumerate convention
#define udBitFieldGet(composite, id)        (((composite) >> id##Shift) & id##Mask)                                       // Retrieve a field from a composite
#define udBitFieldClear(composite, id)      ((composite) & ~(udCastToTypeOf(id##Mask, composite) << id##Shift)            // Clear a field, generally used before updating just one field of a composite
#define udBitFieldSet(composite, id, value) ((composite) | ((udCastToTypeOf(value, composite) & id##Mask) << id##Shift))  // NOTE! Field should be cleared first, most common case is building a composite from zero

template <typename T>
T *udAddBytes(T *ptr, ptrdiff_t bytes) { return (T*)(bytes + (char*)ptr); }

// Template to read from a pointer, does a hard cast to allow reading into const char arrays
template <typename T, typename P>
inline udResult udReadFromPointer(T *pDest, P *&pSrc, int *pBytesRemaining = nullptr, int arrayCount = 1)
{
  int bytesRequired = (int)sizeof(T) * arrayCount;
  if (pBytesRemaining)
  {
    if (*pBytesRemaining < bytesRequired)
      return udR_CountExceeded;
    *pBytesRemaining -= bytesRequired;
  }
  memcpy((void*)pDest, pSrc, bytesRequired);
  pSrc = udAddBytes(pSrc, bytesRequired);
  return udR_Success;
}
// Template to write to a pointer, the complementary function to udReadFromPointer
template <typename T, typename P>
inline udResult udWriteToPointer(T *pSrc, P *&pDest, int *pBytesRemaining = nullptr, int arrayCount = 1)
{
  int bytesRequired = (int)sizeof(T) * arrayCount;
  if (pBytesRemaining)
  {
    if (*pBytesRemaining < bytesRequired)
      return udR_CountExceeded;
    *pBytesRemaining -= bytesRequired;
  }
  memcpy((void*)pDest, pSrc, bytesRequired);
  pDest = udAddBytes(pDest, bytesRequired);
  return udR_Success;
}
template <typename T, typename P>
inline udResult udWriteValueToPointer(T value, P *&pDest, int *pBytesRemaining = nullptr)
{
  return udWriteToPointer(&value, pDest, pBytesRemaining);
}

// *********************************************************************
// Camera helpers
// *********************************************************************
// Update in-place a camera matrix given yaw, pitch and translation parameters
void udUpdateCamera(double camera[16], double yawRadians, double pitchRadians, double tx, double ty, double tz);
void udUpdateCamera(float camera[16], float yawRadians, float pitchRadians, float tx, float ty, float tz);

// *********************************************************************
// Time and timing
// *********************************************************************
uint32_t udGetTimeMs(); // Get a millisecond-resolution timer that is thread-independent - timeGetTime() on windows
uint64_t udPerfCounterStart(); // Get a starting point value for now (thread dependent)
float udPerfCounterMilliseconds(uint64_t startValue, uint64_t end = 0); // Get elapsed time since previous start (end value is "now" by default)
float udPerfCounterSeconds(uint64_t startValue, uint64_t end = 0); // Get elapsed time since previous start (end value is "now" by default)
int udDaysUntilExpired(int maxDays, const char **ppExpireDateStr); // Return the number of days until the build expires

// *********************************************************************
// Some string functions that are safe,
//  - on success all return the length of the result in characters
//    (including null) which is always greater than zero
//  - on overflow a zero returned, also note that
//    a function designed to overwrite the destination (eg udStrcpy)
//    will clear the destination. Functions designed to work
//    with a valid destination (eg udStrcat) will leave the
//    the destination unaltered.
// *********************************************************************
size_t udStrcpy(char *dest, size_t destLen, const char *src);
size_t udStrncpy(char *dest, size_t destLen, const char *src, size_t maxChars);
size_t udStrcat(char *dest, size_t destLen, const char *src);
size_t udStrlen(const char *str);


// *********************************************************************
// String maniplulation functions, NULL-safe
// *********************************************************************
// udStrdup behaves much like strdup, optionally allocating additional characters and replicating NULL
char *udStrdup(const char *s, size_t additionalChars = 0);
// udStrndup behaves much like strndup, optionally allocating additional characters and replicating NULL
char *udStrndup(const char *s, size_t maxChars, size_t additionalChars = 0);
// udStrchr behaves much like strchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrchr(const char *s, const char *pCharList, size_t *pIndex = nullptr, size_t *pCharListIndex = nullptr);
// udStrrchr behaves much like strrchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrrchr(const char *s, const char *pCharList, size_t *pIndex = nullptr);
// udStrstr behaves much like strstr, though the length of s can be supplied for safety
// (zero indicates assume nul-terminated). Optionally the function can provide the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrstr(const char *s, size_t sLen, const char *pSubString, size_t *pIndex = nullptr);
// udStrAtoi behaves much like atoi, optionally giving the number of characters parsed
// and the radix of 2 - 36 can be supplied to parse numbers such as hex(16) or binary(2)
// No overflow testing is performed at this time
int32_t udStrAtoi(const char *s, int *pCharCount = nullptr, int radix = 10);
uint32_t udStrAtou(const char *s, int *pCharCount = nullptr, int radix = 10);
int64_t udStrAtoi64(const char *s, int *pCharCount = nullptr, int radix = 10);
uint64_t udStrAtou64(const char *s, int *pCharCount = nullptr, int radix = 10);
// udStrAtof behaves much like atol, but much faster and optionally gives the number of characters parsed
float udStrAtof(const char *s, int *pCharCount = nullptr);
double udStrAtof64(const char *s, int *pCharCount = nullptr);
// udStr*toa convert numbers to ascii, returning the number of characters required
int udStrUtoa(char *pStr, int strLen, uint64_t value, int radix = 10, int minChars = 1);
int udStrItoa(char *pStr, int strLen, int32_t value, int radix = 10, int minChars = 1);
int udStrItoa64(char *pStr, int strLen, int64_t value, int radix = 10, int minChars = 1);
int udStrFtoa(char *pStr, int strLen, double value, int precision, int minChars = 1);

// Split a line into an array of tokens
int udStrTokenSplit(char *pLine, const char *pDelimiters, char *pTokenArray[], int maxTokens);
// Find the offset of the character FOLLOWING the matching brace character pointed to by pLine
// (may point to null terminator if not found). Brace characters: ({[<"' matched with )}]>"'
size_t udStrMatchBrace(const char *pLine, char escapeChar = 0);
// Helper to skip white space (space,8,10,13), optionally incrementing a line number appropriately
const char *udStrSkipWhiteSpace(const char *pLine, int *pCharCount = nullptr, int *pLineNumber = nullptr);
// Helper to skip to the end of line or null character, optionally incrementing a line number appropriately
const char *udStrSkipToEOL(const char *pLine, int *pCharCount = nullptr, int *pLineNumber = nullptr);
// In-place remove all non-quoted white space (newlines, spaces, tabs), returns new length
size_t udStrStripWhiteSpace(char *pLine);
// Return new string with \ char before any chars in pCharList, optionally freeing pStr
const char *udStrEscape(const char *pStr, const char *pCharList, bool freeOriginal);

// *********************************************************************
// String comparison functions that can be relied upon, NULL-safe
// *********************************************************************
int udStrcmp(const char *s1, const char *s2);
int udStrcmpi(const char *s1, const char *s2);
int udStrncmp(const char *s1, const char *s2, size_t maxChars);
int udStrncmpi(const char *s1, const char *s2, size_t maxChars);
bool udStrBeginsWith(const char *s, const char *prefix);
bool udStrBeginsWithi(const char *s, const char *prefix);
inline bool udStrEqual(const char *s1, const char *s2) { return udStrcmp(s1, s2) == 0; }
inline bool udStrEquali(const char *s1, const char *s2) { return udStrcmpi(s1, s2) == 0; }

#if UDPLATFORM_WINDOWS
// *********************************************************************
// Helper to convert a UTF8 string to wide char for Windows OS calls
// *********************************************************************
class udOSString
{
public:
  udOSString(const char *pString);
  udOSString(const wchar_t *pString);
  ~udOSString();

  operator const wchar_t *() { return m_pWide; }
  operator const char *() { return m_pUTF8; }
  void *m_pAllocation;
  wchar_t *m_pWide;
  char *m_pUTF8;
};
#else
# define udOSString(p) p
#endif

// *********************************************************************
// Helper for growing an array as required TODO: Use configured memory allocators
// *********************************************************************
template<typename T>
bool udSizeArray(T *&ptr, uint32_t &currentLength, uint32_t requiredLength, int32_t allocationMultiples = 0)
{
  if (requiredLength > currentLength || allocationMultiples < 0)
  {
    uint32_t newLength;
    if (allocationMultiples > 1)
      newLength = ((requiredLength + allocationMultiples - 1) / allocationMultiples) * allocationMultiples;
    else
      newLength = requiredLength;
    void *resized = udRealloc(ptr, newLength * sizeof(T));
    if (!resized)
      return false;

    ptr = (T*)resized;
    currentLength = newLength;
  }
  return true;
}

// *********************************************************************
// Count the number of bits set
// *********************************************************************

inline uint32_t udCountBits32(uint32_t v)
{
  v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

inline uint32_t udCountBits64(uint64_t v)
{
  return udCountBits32((uint32_t)v) + udCountBits32((uint32_t)(v >> 32));
}

inline uint32_t udCountBits8(uint8_t a_number)
{
  static const uint8_t bits[256] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8 };
  return bits[a_number];
}


// *********************************************************************
// Create (or optionally update) a standard 32-bit CRC (polynomial 0xedb88320)
uint32_t udCrc(const void *pBuffer, size_t length, uint32_t updateCrc = 0);

// Create (or optionally update) a iSCSI standard 32-bit CRC (polynomial 0x1edc6f41)
uint32_t udCrc32c(const void *pBuffer, size_t length, uint32_t updateCrc = 0);


// *********************************************************************
// Simple base64 decoder, output can be same memory as input
// Pass nullptr for pOutput to count output bytes
udResult udBase64Decode(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength, size_t *pOutputLengthWritten = nullptr);
udResult udBase64Decode(uint8_t **ppOutput, size_t *pOutputLength, const char *pString);

// *********************************************************************
// Simple base64 encoder, unlike decode output CANNOT be the same memory as input
// encodes binaryLength bytes from pBinary to strLength characters in pString
// Pass nullptr for pString to count output bytes without actually writing
udResult udBase64Encode(const void *pBinary, size_t binaryLength, char *pString, size_t strLength, size_t *pOutputLengthWritten = nullptr);
udResult udBase64Encode(const char **ppDestStr, const void *pBinary, size_t binaryLength);

// *********************************************************************
// Add a string to a dynamic table of unique strings.
// Initialise pStringTable to NULL and stringTableLength to 0,
// and table will be reallocated as necessary
// *********************************************************************
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString);

// *********************************************************************
// Threading and concurrency
// *********************************************************************
int udGetHardwareThreadCount();


// *********************************************************************
// A helper class for dealing with filenames, no memory allocation
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udFilename
{
public:
  // Construct either empty as default or from a path
  // No destructor (or memory allocation) to keep the object simple and copyable
  udFilename() { Construct(); }
  udFilename(const char *path) { SetFromFullPath(path); }
  void Construct() { SetFromFullPath(NULL); }

  enum { MaxPath = 260 };

  //Cast operator for convenience
  operator const char *() { return m_path; }

  //
  // Set methods: (set all or part of the internal fully pathed filename, return false if path too long)
  //    FromFullPath      - Set all components from a fully pathed filename
  //    Folder            - Set the folder, retaining the existing filename and extension
  //    SetFilenameNoExt  - Set the filename without extension, retaining the existing folder and extension
  //    SetExtension      - Set the extension, retaining the folder and filename
  //
  bool SetFromFullPath(const char *pFormat, ...);
  bool SetFolder(const char *folder);
  bool SetFilenameNoExt(const char *filenameOnlyComponent);
  bool SetFilenameWithExt(const char *filenameWithExtension);
  bool SetExtension(const char *extComponent);

  //
  // Get methods: (all return a const pointer to the internal fully pathed filename)
  //    GetPath             - Get the complete path (eg pass to fopen)
  //    GetFilenameWithExt  - Get the filename and extension, without the path
  //    GetExt              - Get just the extension (starting with the dot)
  const char *GetPath() const            { return m_path; }
  const char *GetFilenameWithExt() const { return m_path + m_filenameIndex; }
  const char *GetExt() const             { return m_path + m_extensionIndex; }

  //
  // Extract methods: (take portions from within the full path to a user supplied buffer, returning size required)
  //    ExtractFolder       - Extract the folder, including the trailing slash
  //    ExtractFilenameOnly - Extract the filename without extension
  //
  int ExtractFolder(char *folder, int folderLen);
  int ExtractFilenameOnly(char *filename, int filenameLen);

  //
  // Test methods: to determine what is present in the filename
  bool HasFilename() const              { return m_path[m_filenameIndex] != 0; }
  bool HasExt() const                   { return m_path[m_extensionIndex] != 0; }

  // Temporary function to output debug info until unit tests are done to prove reliability
  void Debug();

protected:
  void CalculateIndices();
  int m_filenameIndex;      // Index to starting character of filename
  int m_extensionIndex;     // Index to starting character of extension
  char m_path[MaxPath];         // Buffer for the path, set to 260 characters
};


// *********************************************************************
// A helper class for dealing with filenames, allocates memory due to unknown maximum length of url
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udURL
{
public:
  udURL(const char *pURLText = nullptr) { Construct(); SetURL(pURLText); }
  ~udURL() { SetURL(nullptr); }
  void Construct() { m_pURLText = nullptr; }

  udResult SetURL(const char *pURLText);
  const char *GetScheme()         { return m_pScheme; }
  const char *GetDomain()         { return m_pDomain; }
  int GetPort()                   { return m_port; }
  const char *GetPathWithQuery()  { return m_pPath; }

protected:
  char *m_pURLText;
  const char *m_pScheme;
  const char *m_pDomain;
  const char *m_pPath;
  int m_port;
};

struct udRenderCounters
{
  long renderThreadCount;
  long perspRecursions;
  long orthoRecursions;
  long patchRecursions;
  long orthoEntries;
  long drawRectCalls;
  long zRejections;
  long maskRejections;
  long perspClips;
  long orthoClips;
  long patchClips;
  long orthoMaskRejections;
  long orthoDrawCalls;
  long orthoDrawCallsZCompareOn;
  long getChildCalls;
};


// Load or Save a BMP
udResult udSaveBMP(const char *pFilename, int width, int height, uint32_t *pColorData, int pitch = 0);
udResult udLoadBMP(const char *pFilename, int *pWidth, int *pHeight, uint32_t **pColorData);

// *********************************************************************
// Some helper functions that make use of internal cycling buffers for convenience (threadsafe)
// The caller has the responsibility not to hold the pointer or do stupid
// things with these functions. They are generally useful for passing a
// string directly to a function, the buffer can be overwritten.
// Do not assume the buffer can be used beyond the string null terminator
// *********************************************************************

// Create a temporary small string using one of a number of cycling static buffers
const char *udTempStr(const char *pFormat, ...);

// Give back pretty version (ie with commas) of an int using one of a number of cycling static buffers
const char *udTempStr_CommaInt(int64_t n);
inline const char *udCommaInt(int64_t n) { return udTempStr_CommaInt(n); } // DEPRECATED NAME

// Give back a double whose trailing zeroes are trimmed if possible (0 will trim decimal point also if possible)
const char *udTempStr_TrimDouble(double v, int maxDecimalPlaces, int minDecimalPlaces = 0, bool undoRounding = false);

// Give back a H:MM:SS format string, optionally trimming to MM:SS if hours is zero using one of a number of cycling static buffers
const char *udTempStr_ElapsedTime(int seconds, bool trimHours = true);
inline const char *udSecondsToString(int seconds, bool trimHours = true) { return udTempStr_ElapsedTime(seconds, trimHours); } // DEPRECATED NAME

// Return a human readable measurement string such as 1cm for 0.01, 2mm for 0.002 etc using one of a number of cycling static buffers
const char *udTempStr_HumanMeasurement(double measurement);


// *********************************************************************
// Directory iteration for OS file system only
struct udFindDir
{
  const char *pFilename;
  bool isDirectory;
};

// Test for existence of a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes = nullptr);

// Delete a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileDelete(const char *pFilename);

// Open a folder for reading
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder);

// Read next entry in the folder
udResult udReadDir(udFindDir *pFindDir);

// Free resources associated with the directory
udResult udCloseDir(udFindDir **ppFindDir);

// Create a folder
udResult udCreateDir(const char *pFolder);

// Removes a folder
udResult udRemoveDir(const char *pFolder);

// Write a formatted string to the buffer
int udSprintf(char *pDest, size_t destlength, const char *pFormat, ...);
int udSprintfVA(char *pDest, size_t destlength, const char *pFormat, va_list args);
// Create an allocated string to fit the output, *ppDest will be freed if non-null, and may be an argument to the string
udResult udSprintf(const char **ppDest, const char *pFormat, ...);

// *********************************************************************
// Geospatial helper functions
// *********************************************************************

// Parse a WKT string to a udJSON, each WKT element is parsed as an object
// with element 'type' defining the type string, if the first parameter of
// the element is a string it is assigned to the element 'name', all other
// parameters are stored in an array element 'values'
udResult udParseWKT(udJSON *pValue, const char *pWKTString, int *pCharCount = nullptr);

// Export a previously parsed WKT back to WKT format
udResult udExportWKT(const char **ppOutput, const udJSON *pValue);


#endif // UDPLATFORM_UTIL_H
