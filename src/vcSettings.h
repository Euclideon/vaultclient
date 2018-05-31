#include "SDL2/SDL.h"
#include "udPlatform/udPlatform.h"

#ifndef vcSettings_h__
#define vcSettings_h__

#if UDPLATFORM_WINDOWS
#define ASSETDIR "../assets/"
#elif UDPLATFORM_OSX
#define ASSETDIR "./Resources/"
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#define ASSETDIR "./"
#elif UDPLATFORM_LINUX
#define ASSETDIR "../assets/"
#elif UDPLATFORM_ANDROID
#define ASSETDIR "./" // TBD
#endif
enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
};

struct vcSettings
{
  const char *pUserDirectory; // Where settings, cache files etc live

  struct
  {
    //uint32_t width;
    //uint32_t height;
    //bool maximized;
    //bool fullscreen;
  } window;

  struct
  {
    float moveSpeed;
    float nearPlane;
    float farPlane;
    float fieldOfView;
    vcCameraMoveMode moveMode;
  } camera;
};

// Settings Limits (vcSL prefix)
const float vcSL_CameraNearPlaneMin = 0.001f;
const float vcSL_CameraNearPlaneMax = 100.f;

const float vcSL_CameraFarPlaneMin = vcSL_CameraNearPlaneMax;
const float vcSL_CameraFarPlaneMax = 100000;

const float vcSL_CameraMinMoveSpeed = 0.5f;
const float vcSL_CameraMaxMoveSpeed = 250.f;

const float vcSL_CameraFieldOfViewMin = 5;
const float vcSL_CameraFieldOfViewMax = 100;

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings);
bool vcSettings_Save(vcSettings *pSettings);

#endif // !vcSettings_h__
