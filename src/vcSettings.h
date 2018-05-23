#ifndef  vcSettings_h__
#define vcSettings_h__

struct vcSettings
{
  float cameraSpeed;
  float zNear;
  float zFar;
};

enum
{
  vcMinCameraPlane = 0,
  vcMidCameraPlane = 100,
  vcMaxCameraPlane = 10000,
};

#endif // ! vcSettings_h__
