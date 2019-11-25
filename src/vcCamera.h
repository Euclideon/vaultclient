#ifndef vcCamera_h__
#define vcCamera_h__

#include "udMath.h"
#include "vcMath.h"

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
class vcSceneItem;

enum vcInputState
{
  vcCIS_None,
  vcCIS_Orbiting,
  vcCIS_MovingToPoint,
  vcCIS_ZoomTo,
  vcCIS_PinchZooming,
  vcCIS_Panning,
  vcCIS_MovingForward,

  vcCIS_Count
};

struct vcCameraInput
{
  vcInputState inputState;

  udDouble3 startPosition; // for zoom to
  udDoubleQuat startAngle;
  double progress;

  vcCameraPivotMode currentPivotMode;

  udDouble3 keyboardInput;
  udDouble3 mouseInput;
  udDouble3 controllerDPADInput;

  bool stabilize;

  udDouble3 smoothTranslation;
  udDouble3 smoothRotation;
  
  vcSceneItem *pAttachedToSceneItem; // This does nothing in the camera module but the scene item is allowed to override the camera if this variable is set
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
  bool lockAltitude;
  vcCameraPivotMode cameraMouseBindings[3]; // bindings for camera settings
  vcCameraScrollWheelMode scrollWheelMode;
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

// Applies movement to camera
void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove, udFloat2 windowSize, udFloat2 mousePos);

void vcCamera_UpdateMatrices(vcCamera *pCamera, const vcCameraSettings &settings, const udFloat2 &windowSize, const udFloat2 *pMousePos = nullptr);

#endif//vcCamera_h__
