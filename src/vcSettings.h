#include "SDL2/SDL.h"

#ifndef vcSettings_h__
#define vcSettings_h__

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
const float vcSL_CameraFieldOfViewMax = 120;

// Settings Functions
bool vcSettings_Load(vcSettings *pSettings);
bool vcSettings_Save(vcSettings *pSettings);

#endif // !vcSettings_h__
