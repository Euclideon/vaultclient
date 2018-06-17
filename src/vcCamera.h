#ifndef vcCamera_h__
#define vcCamera_h__

#include "udPlatform/udMath.h"

enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
  vcCMM_Orbit,
};

struct vcCamera;
struct vcState;

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

// Lens Sizes
// - Using formula 2*atan(35/(2*lens)) in radians
const float vcLens7mm = 2.38058f;
const float vcLens11mm = 2.01927f;
const float vcLens15mm = 1.72434f;
const float vcLens24mm = 1.26007f;
const float vcLens30mm = 1.05615f;
const float vcLens50mm = 0.67335f;
const float vcLens70mm = 0.48996f;
const float vcLens100mm = 0.34649f;

enum vcLensSizes
{
  vcLS_Custom = 0,
  vcLS_7mm,
  vcLS_11mm,
  vcLS_15mm,
  vcLS_24mm,
  vcLS_30mm,
  vcLS_50mm,
  vcLS_70mm,
  vcLS_100mm,

  vcLS_TotalLenses
};

void vcCamera_Create(vcCamera **ppCamera);
void vcCamera_Destroy(vcCamera **ppCamera);

// Get camera matrix
udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera);

udDouble3 vcCamera_CreateStoredRotation(vcCamera *pCamera, udDouble3 orbitPosition);

// Applies movement to camera
void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, float speedModifier = 1.f);
void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, bool orbitActive, udDouble3 orbitPosition, udDouble3 storedRotation, float speedModifier = 1.f);

void vcCamera_TravelZoomPath(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcState *pProgramState, double deltaTime);

void vcCamera_SetPosition(vcCamera *pCamera, udDouble3 position);
void vcCamera_SetRotation(vcCamera *pCamera, udDouble3 yprRotation);

const char* const* vcCamera_GetLensNames();

#endif//vcCamera_h__
