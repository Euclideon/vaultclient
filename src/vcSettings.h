#ifndef vcSettings_h__
#define vcSettings_h__

struct vcSettings
{
  float cameraSpeed;
  float zNear;
  float zFar;

  float foV;
};

enum
{
  vcMinCameraPlane = 0,
  vcMidCameraPlane = 100,
  vcMaxCameraPlane = 10000,
};

enum
{
  vcMinFOV = 60,
  vcMaxFOV = 120,
};

const float vcMinCameraSpeed = 0.5f;
const float vcMaxCameraSpeed = 30.f;

#endif // !vcSettings_h__
