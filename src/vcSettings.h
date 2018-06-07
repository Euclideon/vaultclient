#include "SDL2/SDL.h"
#include "udPlatform/udPlatform.h"
#include "udPlatform/udMath.h"
#include "vcCamera.h"

#ifndef vcSettings_h__
#define vcSettings_h__

#if UDPLATFORM_WINDOWS
#define ASSETDIR "assets/"
#elif UDPLATFORM_OSX
#define ASSETDIR "./Resources/"
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#define ASSETDIR "./"
#elif UDPLATFORM_LINUX
#define ASSETDIR "assets/"
#elif UDPLATFORM_ANDROID
#define ASSETDIR "./" // TBD
#endif

enum vcMapTileBlendMode
{
  vcMTBM_Hybrid,
  vcMTBM_Overlay,
};

enum vcDocks
{
  vcdScene,
  vcdSettings,
  vcdSceneExplorer,

  vcdStyling,
  vcdUIDemo,

  vcdTotalDocks
};

enum
{
  vcMaxPathLength = 512,
};

struct vcSettings
{
  bool noLocalStorage; //If set to true; cannot save or load from local storage
  const char *pSaveFilePath;
  char resourceBase[vcMaxPathLength]; // Could be any of: http://uds.domain.local or /mnt/uds or R:/

  bool showFPS;

  struct
  {
    int xpos;
    int ypos;
    int width;
    int height;
    bool maximized;
    bool fullscreen;

    bool windowsOpen[vcdTotalDocks];
  } window;

  vcCameraSettings camera;

  struct
  {
    bool mapEnabled;

    float mapHeight;
    char tileServerAddress[vcMaxPathLength];

    vcMapTileBlendMode blendMode;
    float transparency;
  } maptiles;
};

// Settings Limits (vcSL prefix)
const float vcSL_CameraNearPlaneMin = 0.001f;
const float vcSL_CameraNearPlaneMax = 100.f;

const float vcSL_CameraFarPlaneMin = vcSL_CameraNearPlaneMax;
const float vcSL_CameraFarPlaneMax = 100000;

const float vcSL_CameraNearFarPlaneRatioMax = 10000.f;

const float vcSL_CameraMinMoveSpeed = 0.5f;
const float vcSL_CameraMaxMoveSpeed = 2500.f;

const float vcSL_CameraFieldOfViewMin = 5;
const float vcSL_CameraFieldOfViewMax = 100;

const float vcSL_OSCPixelRatio = 100.f;

// Lens Sizes
// - Using formula 2*atan(35/(2*lens)) in radians
const float vcLens7mm =   2.38058f;
const float vcLens11mm =  2.01927f;
const float vcLens15mm =  1.72434f;
const float vcLens24mm =  1.26007f;
const float vcLens30mm =  1.05615f;
const float vcLens50mm =  0.67335f;
const float vcLens70mm =  0.48996f;
const float vcLens100mm = 0.34649f;

enum vcLensSizes
{
  vcLS_Custom = 0,
  vcLS_7mm,
  vcLS_11mm,
  vcLS_15mm,
  vcLS_24mm,
  vcLS_30mm,
  vcLS_50mm,
  vcLS_70mm,
  vcLS_100mm,
};

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings, bool forceReset = false);
bool vcSettings_Save(vcSettings *pSettings);

#endif // !vcSettings_h__
