#include "SDL2/SDL.h"
#include "udPlatform/udPlatform.h"
#include "udPlatform/udMath.h"
#include "vcCamera.h"

#ifndef vcSettings_h__
#define vcSettings_h__

enum vcMapTileBlendMode
{
  vcMTBM_Hybrid,
  vcMTBM_Overlay,
  vcMTBM_Underlay,

  vcMTBM_Count
};

enum vcDocks
{
  vcDocks_Scene,
  vcDocks_Settings,
  vcDocks_SceneExplorer,
  vcDocks_Convert,

  vcDocks_Count
};

enum vcVisualizatationMode
{
  vcVM_Colour,
  vcVM_Intensity,
  vcVM_Classification,

  vcVM_Count
};

enum vcAnchorStyle
{
  vcAS_None,
  vcAS_Orbit,
  vcAS_Compass,

  vcAS_Count
};

enum
{
  vcMaxPathLength = 512,
};

enum vcPresentationMode
{
  vcPM_Hide,
  vcPM_Show,
  vcPM_Responsive
};

enum vcSettingCategory
{
  vcSC_Appearance,
  vcSC_InputControls,
  vcSC_Viewport,
  vcSC_MapsElevation,
  vcSC_Visualization,
  vcSC_All
};

enum vcTileRendererMapQuality
{
  vcTRMQ_Low,
  vcTRMQ_Medium,
  vcTRMQ_High,

  vcTRMQ_Total,
};

enum vcTileRendererFlags
{
  vcTRF_None = 0,

  vcTRF_OnlyRequestVisibleTiles = 0x1,
};

struct vcSettings
{
  bool noLocalStorage; //If set to true; cannot save or load from local storage
  const char *pSaveFilePath;

  bool onScreenControls;

  struct
  {
    int styleIndex;
    bool showCameraInfo;
    bool showProjectionInfo;
    bool showDiagnosticInfo;
    bool showAdvancedGIS;

    vcAnchorStyle mouseAnchor;
    bool showCompass;
    float POIFadeDistance;
    bool limitFPSInBackground;

    int pointMode;
  } presentation;

  struct
  {
    int xpos;
    int ypos;
    int width;
    int height;
    bool maximized;
    bool touchscreenFriendly;

    bool presentationMode = false;

    bool windowsOpen[vcDocks_Count];

    char languageCode[5]; //4 + nullterminator
  } window;

  struct
  {
    bool rememberServer;
    char serverURL[vcMaxPathLength];

    bool rememberUsername;
    char username[vcMaxPathLength];

    char proxy[vcMaxPathLength];
    char proxyTestURL[vcMaxPathLength];
    bool autoDetectProxy;
    char autoDetectProxyURL[vcMaxPathLength];
    bool ignoreCertificateVerification;
  } loginInfo;

  struct
  {
    vcVisualizatationMode mode;

    int minIntensity;
    int maxIntensity;

    bool useCustomClassificationColours;
    uint32_t customClassificationColors[256];
    const char *customClassificationColorLabels[256];
  } visualization;

  struct
  {
    struct
    {
      bool enable;
      int width;
      float threshold;
      udFloat4 colour;
    } edgeOutlines;

    struct
    {
      bool enable;
      udFloat4 minColour;
      udFloat4 maxColour;
      float startHeight;
      float endHeight;
    } colourByHeight;

    struct
    {
      bool enable;
      udFloat4 colour;
      float startDepth;
      float endDepth;
    } colourByDepth;

    struct
    {
      bool enable;
      udFloat4 colour;
      float distances;
      float bandHeight;
    } contours;
  } postVisualization;

  vcCameraSettings camera;

  struct
  {
    bool mapEnabled;

    vcTileRendererMapQuality mapQuality;
    vcTileRendererFlags mapOptions;

    float mapHeight;
    char tileServerAddress[vcMaxPathLength];
    char tileServerExtension[4];

    vcMapTileBlendMode blendMode;
    float transparency;

    bool mouseInteracts;
  } maptiles;

  vcPresentationMode responsiveUI;
  int hideIntervalSeconds;
};

// Settings Limits (vcSL prefix)
const double vcSL_GlobalLimit = 40000000.0; // these limits can apply to any sliders/inputs related to position or distance
const double vcSL_GlobalLimitSmall = 1000000.0;
const float vcSL_GlobalLimitf = (float)vcSL_GlobalLimit;
const float vcSL_GlobalLimitSmallf = (float)vcSL_GlobalLimitSmall;

const float vcSL_CameraNearPlaneMin = 0.01f;
const float vcSL_CameraNearPlaneMax = 1000.f;
const float vcSL_CameraFarPlaneMin = vcSL_CameraNearPlaneMax;
const float vcSL_CameraFarPlaneMax = 1000000.f;
const float vcSL_CameraNearFarPlaneRatioMax = 20000.f;
const float vcSL_CameraMinMoveSpeed = 0.5f;
const float vcSL_CameraMaxMoveSpeed = 10000.f;
const float vcSL_CameraFieldOfViewMin = 5;
const float vcSL_CameraFieldOfViewMax = 100;

const float vcSL_OSCPixelRatio = 100.f;

const float vcSL_POIFaderMin = 0.f;
const float vcSL_POIFaderMax = 1000000.f;

const float vcSL_MapHeightMin = -1000.f;
const float vcSL_MapHeightMax = 1000.f;

const float vcSL_OpacityMin = 0.f;
const float vcSL_OpacityMax = 1.f;

const float vcSL_IntensityMin = 0.f;
const float vcSL_IntensityMax = 65535.f;

const int vcSL_EdgeHighlightMin = 1;
const int vcSL_EdgeHighlightMax = 10;
const float vcSL_EdgeHighlightThresholdMin = 0.001f;
const float vcSL_EdgeHighlightThresholdMax = 10.f;

const float vcSL_ColourByHeightMin = 0.f;
const float vcSL_ColourByHeightMax = 1000.f;
const float vcSL_ColourByDepthMin = 0.f;
const float vcSL_ColourByDepthMax = 1000.f;
const float vcSL_ContourDistanceMin = 0.f;
const float vcSL_ContourDistanceMax = 1000.f;
const float vcSL_ContourBandHeightMin = 0.f;
const float vcSL_ContourBandHeightMax = 1000.f;

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings, bool forceReset = false, vcSettingCategory group = vcSC_All);
bool vcSettings_Save(vcSettings *pSettings);

// Uses udTempStr internally.
const char *vcSettings_GetAssetPath(const char *pFilename);

// Provides a handler for "asset://" files
udResult vcSettings_RegisterAssetFileHandler();

#endif // !vcSettings_h__
