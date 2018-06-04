#ifndef vcCamera_h__
#define vcCamera_h__

#include "udPlatform/udMath.h"

enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
};

struct vcCamera
{
  udDouble3 position;
  udDouble3 yprRotation;
  udQuaternion<double> orientation;

  struct
  {
    float vertical;
    float forward;
    float right;
  } moveDirection;
};

struct vcCameraSettings
{
  float moveSpeed;
  float nearPlane;
  float farPlane;
  float fieldOfView;
  bool invertX;
  bool invertY;
  vcCameraMoveMode moveMode;
};

void vcCamera_Create(vcCamera *pCamera);
udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera);


#endif//vcCamera_h__
