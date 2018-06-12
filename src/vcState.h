#ifndef vcState_h__
#define vcState_h__

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udValue.h"

#include "vcSettings.h"

struct SDL_Window;

struct vdkContext;

struct vcRenderContext;
struct vcCamera;
struct vcTexture;

typedef unsigned int GLuint;

struct vcState
{
  bool programComplete;
  SDL_Window *pWindow;
  GLuint defaultFramebuffer;

  bool onScreenControls;

  vcCamera *pCamera;

  size_t numSelectedModels;
  size_t prevSelectedModel;

  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  uint16_t currentSRID;

  vcTexture *pWatermarkTexture;

  udDouble3 worldMousePos;
  udDouble3 currentMeasurePoint;

  udDouble3 orbitPos;
  udDouble3 storedDeltaAngle;

  bool hasContext;
  vdkContext *pContext;
  vcRenderContext *pRenderContext;

  char serverURL[vcMaxPathLength];
  char username[vcMaxPathLength];
  char password[vcMaxPathLength];

  char modelPath[vcMaxPathLength];

  vcSettings settings;
  udValue projects;
};

#endif // !vcState_h__
