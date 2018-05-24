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
  vcMaxCameraPlane = 100000,
};

enum
{
  vcMinFOV = 5,
  vcMaxFOV = 120,
};

const float vcMinCameraSpeed = 0.5f;
const float vcMaxCameraSpeed = 250.f;

#endif // !vcSettings_h__
