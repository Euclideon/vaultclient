#ifndef vcWebFile_h__
#define vcWebFile_h__

#include "udResult.h"

// Caution, this API does not support partial request currently so it will download the full file regardless

udResult vcWebFile_RegisterFileHandlers();

void vcWebFile_OpenBrowser(const char *pWebpageAddress);

#endif //vcWebFile_h__
