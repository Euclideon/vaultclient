#ifndef vcCamera_h__
#define vcCamera_h__

#include "udPlatform/udMath.h"

enum vcCameraMoveMode
{
  vcCMM_Plane,
  vcCMM_Helicopter,
};

enum vcCameraPivotMode
{
  vcCPM_Tumble,
  vcCPM_Orbit,
  vcCPM_Pan,
};

struct vcCamera
{
  udDouble3 position;
  udDouble3 yprRotation;
};

struct vcState;

enum vcInputState
{
  vcCIS_None,
  vcCIS_Orbiting,
  vcCIS_MovingToPoint,
  vcCIS_CommandZooming,
  vcCIS_PinchZooming,
  vcCIS_Panning,

  vcCIS_Count
};

struct vcCameraInput
{
  bool isFocused;

  vcInputState inputState;

  udDouble3 focusPoint;
  udDouble3 startPosition; // for zoom to
  udDouble3 storedRotation; // for orbiting
  double progress;

  vcCameraPivotMode currentPivotMode;

  udDouble3 keyboardInput;
  udDouble3 mouseInput;
};

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
  vcCameraPivotMode cameraMouseBindings[3]; // bindings for camera settings
};

// Lens Sizes
// - Using formula 2*atan(35/(2*lens)) in radians
const float vcLens15mm = 1.72434f;
const float vcLens24mm = 1.26007f;
const float vcLens30mm = 1.05615f;
const float vcLens50mm = 0.67335f;
const float vcLens70mm = 0.48996f;
const float vcLens100mm = 0.34649f;

enum vcLensSizes
{
  vcLS_Custom = 0,
  vcLS_15mm,
  vcLS_24mm,
  vcLS_30mm,
  vcLS_50mm,
  vcLS_70mm,
  vcLS_100mm,

  vcLS_TotalLenses
};

const char** vcCamera_GetLensNames();

void vcCamera_Create(vcCamera **ppCamera);
void vcCamera_Destroy(vcCamera **ppCamera);

// Get camera matrix
udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera);

udDouble3 vcCamera_CreateStoredRotation(vcCamera *pCamera, udDouble3 orbitPosition);

// Applies movement to camera
void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraInput *pCamInput, double deltaTime, float speedModifier = 1.f);

void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove);

#endif//vcCamera_h__
