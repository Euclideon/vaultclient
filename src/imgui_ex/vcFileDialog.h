#ifndef vcFileDialog_h__
#define vcFileDialog_h__

#include <stddef.h>

bool vcFileDialog_Show(char *pPath, size_t pathLength, bool loadOnly = true, const char **ppExtensions = nullptr, size_t extensionCount = 0);

#endif //vcMenuButtons_h__
