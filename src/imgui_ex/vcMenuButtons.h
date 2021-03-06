#ifndef vcMenuButtons_h__
#define vcMenuButtons_h__

#include "udMath.h"
#include "vcHotkey.h"

enum vcMenuBarButtonIcon
{
  vcMBBI_None = -1,

  vcMBBI_Translate = 0,
  vcMBBI_Rotate = 1,
  vcMBBI_Scale = 2,
  vcMBBI_ShowCameraSettings = 3,
  vcMBBI_LockAltitude = 4,
  vcMBBI_Help = 5,
  vcMBBI_MeasureLine = 6,
  vcMBBI_MeasureArea = 7,
  vcMBBI_MeasureVolume = 8,
  vcMBBI_UseLocalSpace = 9,

  vcMBBI_AddPointCloud = 10,
  vcMBBI_AddPointOfInterest = 11,
  vcMBBI_AddAreaOfInterest = 12,
  vcMBBI_AddLines = 13,
  vcMBBI_AddFolder = 14,
  vcMBBI_Remove = 15,
  vcMBBI_AddOther = 16,
  vcMBBI_Media = 17,
  vcMBBI_Settings = 18,
  vcMBBI_Inspect = 19,

  vcMBBI_MeasureHeight = 20,
  vcMBBI_AngleTool = 21,
  vcMBBI_ProjectSettings = 22,
  vcMBBI_AddBoxFilter = 23,
  vcMBBI_AddSphereFilter = 24,
  vcMBBI_AddCylinderFilter = 25,
  vcMBBI_AddCrossSectionFilter = 26,
  vcMBBI_SaveViewport = 27,
  vcMBBI_SaveViewportWithVis = 28,
  vcMBBI_SplitViewport = 29,

  vcMBBI_Burger = 30,
  vcMBBI_Layers = 31,
  vcMBBI_Select = 32,
  vcMBBI_FullScreen = 33,
  vcMBBI_MapMode = 34,
  vcMBBI_Geospatial = 35,
  vcMBBI_Grid = 36,
  vcMBBI_ExpertGrid = 37,
  vcMBBI_StorageCloud = 38,
  vcMBBI_StorageLocal = 39,

  vcMBBI_Crosshair = 40,
  vcMBBI_Visualization = 41,
  vcMBBI_Share = 42,
  vcMBBI_Save = 43,
  vcMBBI_NewProject = 44,
  vcMBBI_Convert = 45,
  vcMBBI_Open = 46,

  //Reserved = 47-49

  vcMBBI_ViewShed = 50,
  vcMBBI_UDS = 51,

  //Reserved = 5X+
};

enum vcMenuBarButtonGap
{
  vcMBBG_FirstItem, // This is the first item
  vcMBBG_SameGroup, // Small Gap from previous item
  vcMBBG_NewGroup, // Large Gap from previous item
};

struct vcTexture;

bool vcMenuBarButton(vcTexture *pUITexture, const char *pButtonName, vcBind shortcut, const vcMenuBarButtonIcon buttonIndex, vcMenuBarButtonGap gap, bool selected = false, float scale = 1.f, const char *pID = nullptr);

udFloat4 vcGetIconUV(vcMenuBarButtonIcon iconIndex);

#endif //vcMenuButtons_h__
