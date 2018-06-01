#include "SDL2/SDL.h"
#include "udPlatform/udPlatform.h"

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
enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
};

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

  struct
  {
    float moveSpeed;
    float nearPlane;
    float farPlane;
    float fieldOfView;
    vcCameraMoveMode moveMode;
  } camera;

  struct
  {
    char resourceBase[vcMaxPathLength]; // Could be any of: http://uds.domain.local or /mnt/uds or R:/
  } server;

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

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings, bool forceReset = false);
bool vcSettings_Save(vcSettings *pSettings);

#endif // !vcSettings_h__
