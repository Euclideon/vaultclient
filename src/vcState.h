#ifndef vcState_h__
#define vcState_h__

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udValue.h"

#include "vcSettings.h"
#include "vcModel.h"

struct SDL_Window;

struct vdkContext;

struct vcFramebuffer;
struct vcRenderContext;
struct vcCamera;
struct vcTexture;
struct vcConvertContext;

enum vcPopupTriggerID
{
  vcPopup_ModelProperties,
  vcPopup_About,

  vcPopupCount
};

struct vcState
{
  bool programComplete;
  SDL_Window *pWindow;
  vcFramebuffer *pDefaultFramebuffer;

  bool onScreenControls;

  bool popupTrigger[vcPopupCount];

  vcCamera *pCamera;

  udChunkedArray<vcModel> vcModelList;
  bool lastModelLoaded;

  size_t numSelectedModels;
  size_t prevSelectedModel;

  struct
  {
    udValue *pMetadata;
    size_t index;
    vcTexture *pWatermarkTexture;
    int32_t watermarkWidth;
    int32_t watermarkHeight;
  } selectedModelProperties;

  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  uint16_t currentSRID;

  vcTexture *pWatermarkTexture;

  udDouble3 worldMousePos;
  udDouble3 currentMeasurePoint;
  bool pickingSuccess;
  bool measureMode;

  vcCameraInput cameraInput;

  bool hasContext;
  vdkContext *pVDKContext;
  vcRenderContext *pRenderContext;
  vcConvertContext *pConvertContext;

  char serverURL[vcMaxPathLength];
  char username[vcMaxPathLength];
  char password[vcMaxPathLength];

  char modelPath[vcMaxPathLength];

  vcSettings settings;
  udValue projects;
};

#endif // !vcState_h__
