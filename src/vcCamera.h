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
  } moveDirection; // buffer for movement

  struct
  {
    float yaw;
    float pitch;
    float roll;
  } yprDirection; // buffer for rotation
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

// Applies movement instruction for next frame
void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 yprRotation, udDouble3 frvVector);
// Moves based on buffers
void vcCamera_Update(vcCamera *pCamera, vcCameraSettings *pCamSettings, double deltaTime, float speedModifier = 1.f);



#endif//vcCamera_h__
