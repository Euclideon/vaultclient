#ifndef vcFileDialog_h__
#define vcFileDialog_h__

#include <stddef.h>
#include "udMath.h"
#include "udCallback.h"

struct vcState;

using vcFileDialogCallback = udCallback<void(void)>;

static const char *SupportedFileTypes_Images[] = { ".jpg", ".png", ".tga", ".bmp", ".gif" };
static const char *SupportedFileTypes_SceneItems[] = { ".uds", ".ssf", ".udg", ".jpg", ".png", ".slpk", ".obj", ".vsm" };
static const char *SupportedFileTypes_PolygonModels[] = { ".obj", ".vsm" };

static const char *SupportedFileTypes_ProjectsExport[] = { ".json" };
static const char *SupportedFileTypes_ProjectsImport[] = { ".json", ".udp" };

static const char *SupportedTileTypes_ConvertExport[] = { ".uds" };
static const char *SupportedTileTypes_QueryExport[] = { ".uds", ".las" };
static const char *SupportedFileTypes_ConvertImport[] = { ".uds", ".ssf", ".udg", ".las", ".laz", ".e57", ".txt", ".csv", ".asc", ".pts", ".ptx", ".xyz", ".obj"
#ifdef FBXSDK_ON
  , ".fbx", ".dae", ".dxf"
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
  const char *pLabel;

  vcFileDialogType dialogType; //Alternative is saveFile
  char *pPath;
  size_t pathLen;

  const char **ppExtensions; // Should be static arrays only
  size_t numExtensions;

  vcFileDialogCallback onSelect;
};

// Any Begin/BeginChild that contains a call to vcFileDialog_Open needs to call vcFileDialog_ShowModal
void vcFileDialog_Open(vcState *pProgramState, const char *pLabel, char *pPath, size_t pathLen, const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback callback);

template <size_t N, size_t M> inline void vcFileDialog_Open(vcState *pProgramState, const char *pLabel, char(&path)[N], const char *(&extensions)[M], vcFileDialogType dialogType, vcFileDialogCallback callback)
{
  vcFileDialog_Open(pProgramState, pLabel, path, N, extensions, M, dialogType, callback);

  // These unused are required for compiling to work
  udUnused(SupportedFileTypes_Images);
  udUnused(SupportedFileTypes_SceneItems);
  udUnused(SupportedFileTypes_PolygonModels);
  udUnused(SupportedFileTypes_ProjectsExport);
  udUnused(SupportedFileTypes_ProjectsImport);
  udUnused(SupportedTileTypes_ConvertExport);
  udUnused(SupportedTileTypes_QueryExport);
  udUnused(SupportedFileTypes_ConvertImport);
}

void vcFileDialog_ShowModal(vcState *pProgramState);

#endif //vcMenuButtons_h__
