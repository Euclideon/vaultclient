#ifndef vcCamera_h__
#define vcCamera_h__

#include "udMath.h"
#include "vcMath.h"

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
  vcCPM_Forward,
};

enum vcCameraScrollWheelMode
{
  vcCSWM_Dolly,
  vcCSWM_ChangeMoveSpeed
};

struct vcCamera
{
  udDouble3 position;
  udDouble3 positionInLongLat;
  udDouble3 eulerRotation;

  udRay<double> worldMouseRay;

  struct
  {
    udDouble4x4 camera;
    udDouble4x4 view;

    udDouble4x4 projection;
    udDouble4x4 projectionUD;
    udDouble4x4 projectionNear; // Used for skybox

    udDouble4x4 viewProjection;
    udDouble4x4 inverseViewProjection;
  } matrices;
};

struct vcState;

enum vcInputState
{
  vcCIS_None,
  vcCIS_Orbiting,
  vcCIS_MovingToPoint,
  vcCIS_LookingAtPoint,
  vcCIS_CommandZooming,
  vcCIS_PinchZooming,
  vcCIS_Panning,
  vcCIS_MovingForward,
  vcCIS_FlyThrough,

  vcCIS_Count
};

enum vcCameraMode
{
  vcCM_FreeRoam,
  vcCM_OrthoMap,

  vcCM_Count,
};

struct vcCameraInput
{
  vcInputState inputState;

  udDouble3 startPosition; // for zoom to
  udDouble3 lookAtPosition; // for 'look at'
  udDoubleQuat startAngle;
  double progress;
  double progressMultiplier;

  vcCameraPivotMode currentPivotMode;

  udDouble3 keyboardInput;
  udDouble3 mouseInput;
  udDouble3 controllerDPADInput;
  void *pObjectInfo;

  int flyThroughPoint;
  bool transitioningToMapMode;
  bool stabilize;

  udDouble3 smoothTranslation;
  udDouble3 smoothRotation;
  double smoothOrthographicChange;
};

struct vcCameraSettings
{
  float moveSpeed;
  float nearPlane;
  float farPlane;
  float fieldOfView;
  bool invertMouseX;
  bool invertMouseY;
  bool invertControllerX;
  bool invertControllerY;
  int lensIndex;
  vcCameraMoveMode moveMode;
  vcCameraPivotMode cameraMouseBindings[3]; // bindings for camera settings
  vcCameraScrollWheelMode scrollWheelMode;

  vcCameraMode cameraMode;
  double orthographicSize;
};

// 0.05745 per mm of FOV
static const double vcCamera_HeightToOrthoFOVRatios[] =
{
  1.0, // custom: TODO fix me
  0.86175,
  1.3788,
  1.7235,
  2.8725,
  4.0215,
  5.745,
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

// Applies movement to camera
void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove, udFloat2 windowSize, udFloat2 mousePos);

void vcCamera_SwapMapMode(vcState *pProgramState);
void vcCamera_LookAt(vcState *pProgramState, const udDouble3 &targetPosition, double speedMultiplier = 1.0);

void vcCamera_UpdateMatrices(vcCamera *pCamera, const vcCameraSettings &settings, vcCameraInput *pCamInput, const udFloat2 &windowSize, const udFloat2 *pMousePos = nullptr);

#endif//vcCamera_h__
