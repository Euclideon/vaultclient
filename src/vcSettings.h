#include "SDL2/SDL.h"

#ifndef vcSettings_h__
#define vcSettings_h__

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
  } camera;
};

enum vsSettingsLimits
{
  vcSL_CameraNearPlaneMin = 0,
  vcSL_CameraNearPlaneMax = 100,

  vcSL_CameraFarPlaneMin = vcSL_CameraNearPlaneMax,
  vcSL_CameraFarPlaneMax = 100000,

  vcSL_CameraFieldOfViewMin = 5,
  vcSL_CameraFieldOfViewMax = 120,
};

const float vcSL_Camera_MinMoveSpeed = 0.5f;
const float vcSL_Camera_MaxMoveSpeed = 250.f;

bool vcSettings_Load(vcSettings *pSettings);
bool vcSettings_Save(vcSettings *pSettings);

#endif // !vcSettings_h__
