#ifndef vcSettings_h__
#define vcSettings_h__

#include "SDL2/SDL.h"

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udMath.h"
#include "udChunkedArray.h"
#include "udUUID.h"

#include "vcCamera.h"
#include "vcGLState.h"
#include "vcTexture.h"
#include "vcUnitConversion.h"

#include "imgui.h"

enum vcMapTileBlendMode
{
  vcMTBM_Hybrid,
  vcMTBM_Overlay,
  vcMTBM_Underlay,

  vcMTBM_Count
};

enum vcVisualizatationMode
{
  vcVM_Default,
  vcVM_Colour,
  vcVM_Intensity,
  vcVM_Classification,
  vcVM_DisplacementDistance,
  vcVM_DisplacementDirection,
  vcVM_GPSTime,
  vcVM_ScanAngle,
  vcVM_PointSourceID,
  vcVM_ReturnNumber,
  vcVM_NumberOfReturns,

  vcVM_Count
};

enum vcDisplacementShaderType
{
  vcDST_Absolute,
  vcDST_Signed
};

enum
{
  vcMaxPathLength = 512,
  vcMetadataMaxLength = 256,
  vcMaxProjectHistoryCount = 50
};

enum vcWindowLayout
{
  vcWL_SceneLeft,
  vcWL_SceneRight
};

enum vcSettingsUIRegions
{
  vcSR_Appearance,
  vcSR_Inputs,
  vcSR_Maps,
  vcSR_Visualisations,
  vcSR_Tools,
  vcSR_UnitsOfMeasurement,
  vcSR_KeyBindings,
  vcSR_ConvertDefaults,
  vcSR_Screenshot,
  vcSR_Connection,

  vcSR_ReleaseNotes,
  vcSR_Update,
  vcSR_About,

  vcSR_DiagnosticMissingStrings,
  vcSR_DiagnosticTranslation,

  vcSR_Count
};

enum vcSettingCategory
{
  vcSC_Appearance,
  vcSC_InputControls,
  vcSC_MapsElevation,
  vcSC_Visualization,
  vcSC_Tools,
  vcSC_Convert,
  vcSC_Languages,
  vcSC_Bindings,
  vcSC_Screenshot,
  vcSC_Connection,

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

enum vcSkyboxType
{
  vcSkyboxType_None,
  vcSkyboxType_Colour,
  vcSkyboxType_Simple,
  vcSkyboxType_Atmosphere,
};

struct vcLanguageOption
{
  // Arbitrary limits
  char languageName[40];
  char filename[16];
};

struct vcVisualizationSettings
{
  //These are defined in the LAS 1.4 spec
  static const int16_t s_scanAngleMin = -30'000; //Represents -180 deg
  static const int16_t s_scanAngleMax =  30'000; //Represents  180 deg
  static const uint32_t s_scanAngleRange = s_scanAngleMax - s_scanAngleMin;

  //The maximum number of returns by some modern LiDAR scanners seems to be 6.
  static const uint32_t s_maxReturnNumbers = 6;

  vcVisualizatationMode mode;

  int minIntensity;
  int maxIntensity;

  struct
  {
    udFloat2 bounds;
    
    uint32_t min;
    uint32_t max;
    uint32_t error;
    uint32_t mid;
  } displacement;

  struct
  {
    double minTime;
    double maxTime;
    vcTimeReference inputFormat; //This should ony be GPS or GPSAdjusted 
  } GPSTime;

  struct
  {
    double minAngle;
    double maxAngle;
  } scanAngle;

  struct KV
  {
    uint16_t id;
    uint32_t colour;
  };

  struct
  {
    udChunkedArray<KV> colourMap;
    uint32_t defaultColour;
  } pointSourceID;

  uint32_t returnNumberColours[s_maxReturnNumbers];
  uint32_t numberOfReturnsColours[s_maxReturnNumbers];

  bool useCustomClassificationColours;
  bool customClassificationToggles[256];
  uint32_t customClassificationColors[256];
  const char *customClassificationColorLabels[256];
};

struct vcToolSettings
{
  struct
  {
    float minWidth;
    float maxWidth;
    float width;
    int fenceMode;
    int style;
    udFloat4 colour;
  } line;

  struct
  {
    udFloat4 colour;
  } fill;

  struct
  {
    udFloat4 textColour;
    udFloat4 backgroundColour;
    int textSize;
  } label;
};

struct vcProjectHistoryInfo
{
  bool isServerProject; // or local
  const char *pName;
  const char *pPath;
  // TODO: Local timestamp
};

struct vcSettings
{
  bool noLocalStorage; //If set to true; cannot save or load from local storage
  const char *pSaveFilePath;
  char cacheAssetPath[vcMaxPathLength];

  bool onScreenControls;

  vcUnitConversionData unitConversionData;

  struct
  {
    bool showCameraInfo;
    bool showProjectionInfo;
    bool showDiagnosticInfo;
    bool showEuclideonLogo;
    bool showAdvancedGIS;
    bool sceneExplorerCollapsed; // True if scene explorer is collapsed.

    struct
    {
      vcSkyboxType type;
      udFloat4 colour;

      float timeOfDay;
      float month;
      float exposure;
      bool keepSameTime;
      bool useLiveTime;
    } skybox;

    float saturation;

    float POIFadeDistance;
    float imageRescaleDistance;
    bool limitFPSInBackground;

    int pointMode;

    vcWindowLayout layout;
    int sceneExplorerSize;
    float convertLeftPanelPercentage;
    bool columnSizeCorrect; // Not saved, this is updated when the columns have been set to the correct size
  } presentation;

  struct
  {
    int xpos;
    int ypos;
    int width;
    int height;
    bool maximized;
    bool touchscreenFriendly;

    bool isFullscreen;
    bool useNativeUI;

    char languageCode[16];
  } window;

  struct
  {
    bool rememberServer;
    char serverURL[vcMaxPathLength];

    bool rememberEmail;
    char email[vcMaxPathLength];

    char proxy[vcMaxPathLength];
    char proxyTestURL[vcMaxPathLength];
    char autoDetectProxyURL[vcMaxPathLength];
    char userAgent[vcMaxPathLength];
    bool ignoreCertificateVerification;

    bool requiresProxyAuth;

    bool rememberProxyUsername;
    char proxyUsername[vcMaxPathLength];
    char proxyPassword[vcMaxPathLength];

    // These are not saved but are required testing information
    udInterlockedBool testing;
    bool tested;
    int testStatus;
  } loginInfo;

  vcVisualizationSettings visualization;
  vcToolSettings tools;  

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
      float rainbowRepeat;
      float rainbowIntensity;
    } contours;
  } postVisualization;

  vcCameraSettings camera;

  struct vcMapServer
  {
    char tileServerAddress[vcMaxPathLength];
    char attribution[vcMaxPathLength];
    udUUID tileServerAddressUUID;
  };

  struct
  {
    bool mapEnabled;
    bool demEnabled;

    vcTileRendererMapQuality mapQuality;
    vcTileRendererFlags mapOptions;

    float mapHeight;

    char mapType[32]; // 'custom', 'euc-osm-base', 'euc-az-aerial', 'euc-az-roads'
    vcMapServer customServer;
    vcMapServer activeServer; // The server settings actually in use

    vcMapTileBlendMode blendMode;
    float transparency;
  } maptiles;

  struct
  {
    bool enable;
    udFloat4 colour;
    float thickness;
  } objectHighlighting;

  struct
  {
    char tempDirectory[vcMaxPathLength];
    char author[vcMetadataMaxLength];
    char comment[vcMetadataMaxLength];
    char copyright[vcMetadataMaxLength];
    char license[vcMetadataMaxLength];
  } convertdefaults;

  struct
  {
    bool taking;
    uint8_t* pPixels;

    bool viewShot;

    int resolutionIndex; // Not saved
    udUInt2 resolution;

    char outputPath[vcMaxPathLength];
  } screenshot;

  struct
  {
    udChunkedArray<vcProjectHistoryInfo> projects;
  } projectsHistory;

  // These are experimental features that will eventually be removed or moved to another setting.
  // They will mostly be exposed via the System->Experiments menu to hide them away from most users
  struct
  {
    //No current experiments
  } experimental;

  udChunkedArray<vcLanguageOption> languageOptions;

  struct
  {
    bool enable;
    int range;
  } mouseSnap;
};

// Settings Limits (vcSL prefix)
const double vcSL_GlobalLimit = 40000000.0; // these limits can apply to any sliders/inputs related to position or distance
const double vcSL_GlobalLimitSmall = 1000000.0;
const float vcSL_GlobalLimitf = (float)vcSL_GlobalLimit;
const float vcSL_GlobalLimitSmallf = (float)vcSL_GlobalLimitSmall;

const float vcSL_CameraMinMoveSpeed = 0.5f;
const float vcSL_CameraMaxMoveSpeed = 1000000.f;
const float vcSL_CameraMaxAttachSpeed = 500.f;
const float vcSL_CameraFieldOfViewMin = 5;
const float vcSL_CameraFieldOfViewMax = 100;

const float vcSL_OSCPixelRatio = 100.f;

const float vcSL_POIFaderMin = 0.f;
const float vcSL_POIFaderMax = 1000000.f;

const float vcSL_ImageRescaleMin = 0.f;
const float vcSL_ImageRescaleMax = 1000000.f;

const float vcSL_MapHeightMin = -1000.f;
const float vcSL_MapHeightMax = 1000.f;

const float vcSL_OpacityMin = 0.f;
const float vcSL_OpacityMax = 1.f;

const float vcSL_IntensityMin = 0.f;
const float vcSL_IntensityMax = 65535.f;

const float vcSL_GPSTimeMin = 0.f;

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
const float vcSL_ContourBandHeightMax = 10.f;

const int vcSL_MouseSnapRangeMax = 50;

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings, bool forceReset = false, vcSettingCategory group = vcSC_All);
bool vcSettings_Save(vcSettings *pSettings);

void vcSettings_Cleanup(vcSettings *pSettings);

// Provides a handler for "asset://" files
udResult vcSettings_RegisterAssetFileHandler();

// Various settings helpers
udResult vcSettings_UpdateLanguageOptions(vcSettings *pSettings);
void vcSettings_ApplyMapChange(vcSettings *pSettings);

// Load Branding Info
void vcSettings_LoadBranding(vcState *pState);

void vcSettings_CleanupHistoryProjectItem(vcProjectHistoryInfo *pProjectItem);

#endif // !vcSettings_h__
