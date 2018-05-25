#include "SDL2/SDL.h"

#ifndef vcSettings_h__
#define vcSettings_h__

struct vcSettings
{
  float cameraSpeed;
  float zNear;
  float zFar;

  float foV;
};

const float vcMinCameraPlane = 0.001f;
const float vcMidCameraPlane = 100.f;
const float vcMaxCameraPlane = 100000.f;

enum
{
  vcMinFOV = 5,
  vcMaxFOV = 120,
};

const float vcMinCameraSpeed = 0.5f;
const float vcMaxCameraSpeed = 250.f;

#endif // !vcSettings_h__
