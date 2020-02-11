#ifndef vcFileDialog_h__
#define vcFileDialog_h__

#include <stddef.h>
#include "udMath.h"
#include "udCallback.h"

struct vcProgramState;
using vcFileDialogCallback = udCallback<void(void)>;

static const char *SupportedFileTypes_Images[] = { ".jpg", ".png", ".tga", ".bmp", ".gif" };
static const char *SupportedFileTypes_SceneItems[] = { ".uds", ".ssf", ".udg", ".jpg", ".png" };

static const char *SupportedFileTypes_ProjectsExport[] = { ".json" };
static const char *SupportedFileTypes_ProjectsImport[] = { ".json", ".udp" };

static const char *SupportedTileTypes_ConvertExport[] = { ".uds" };
static const char *SupportedFileTypes_ConvertImport[] = { ".uds", ".ssf", ".udg", ".las", ".laz", ".e57", ".txt", ".csv", ".asc", ".pts", ".ptx", ".xyz", ".obj"
#ifdef FBXSDK_ON
  , ".fbx"
#endif
};

enum vcFileDialogType
{
  vcFDT_OpenFile,
  vcFDT_SaveFile,
  vcFDT_SelectDirectory
};

struct vcFileDialog
{
  bool showDialog;
  bool folderOnly;
  vcFileDialogType dialogType; //Alternative is saveFile
  char *pPath;
  size_t pathLen;

  const char **ppExtensions; // Should be static arrays only
  size_t numExtensions;

  vcFileDialogCallback onSelect;
};

void vcFileDialog_Show(vcFileDialog *pDialog, char *pPath, size_t pathLen, const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback callback);

template <size_t N> inline void vcFileDialog_Show(vcFileDialog *pDialog, char(&path)[N], const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback callback)
{
  vcFileDialog_Show(pDialog, path, N, ppExtensions, numExtensions, dialogType, callback);
}

template <size_t M> inline void vcFileDialog_Show(vcFileDialog *pDialog, char *pPath, size_t pathLen, const char *(&extensions)[M], vcFileDialogType dialogType, vcFileDialogCallback callback)
{
  vcFileDialog_Show(pDialog, pPath, pathLen, extensions, M, dialogType, callback);
}

template <size_t N, size_t M> inline void vcFileDialog_Show(vcFileDialog *pDialog, char(&path)[N], const char *(&extensions)[M], vcFileDialogType dialogType, vcFileDialogCallback callback)
{
  vcFileDialog_Show(pDialog, path, N, extensions, M, dialogType, callback);

  // These unused are required for compiling to work
  udUnused(SupportedFileTypes_Images);
  udUnused(SupportedFileTypes_SceneItems);
  udUnused(SupportedFileTypes_ProjectsExport);
  udUnused(SupportedFileTypes_ProjectsImport);
  udUnused(SupportedTileTypes_ConvertExport);
  udUnused(SupportedFileTypes_ConvertImport);
}

bool vcFileDialog_DrawImGui(char *pPath, size_t pathLength, vcFileDialogType dialogType = vcFDT_OpenFile, const char **ppExtensions = nullptr, size_t extensionCount = 0);

#endif //vcMenuButtons_h__
