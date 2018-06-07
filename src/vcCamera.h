#ifndef vcCamera_h__
#define vcCamera_h__

#include "udPlatform/udMath.h"

enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
};

struct vcCamera;

struct vcCameraSettings
{
  float moveSpeed;
  float nearPlane;
  float farPlane;
  float fieldOfView;
  bool invertX;
  bool invertY;
  int lensIndex;
  vcCameraMoveMode moveMode;
};

void vcCamera_Create(vcCamera **ppCamera);
void vcCamera_Destroy(vcCamera **ppCamera);

// Get camera matrix
udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera);

// Applies movement instruction for next frame
void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset);

// Moves based on buffers
void vcCamera_Update(vcCamera *pCamera, vcCameraSettings *pCamSettings, double deltaTime, float speedModifier = 1.f);


//udDouble3 vcCamera_GetPosition(vcCamera *pCamera);
void vcCamera_SetPosition(vcCamera *pCamera, udDouble3 position);



#endif//vcCamera_h__
