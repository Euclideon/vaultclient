#ifndef vcState_h__
#define vcState_h__

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udValue.h"

#include "vcSettings.h"
#include "vcModel.h"
#include "vcGIS.h"

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

  udChunkedArray<const char *> loadList;
  udChunkedArray<vcModel> vcModelList;

  size_t numSelectedModels;
  size_t prevSelectedModel;

  struct
  {
    size_t index;
    udValue *pMetadata;
    vcTexture *pWatermarkTexture;
  } selectedModelProperties;

  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  vcGISSpace gis;

  vcTexture *pCompanyLogo;
  vcTexture *pSceneWatermark;

  udDouble3 worldMousePos;
  bool pickingSuccess;

  vcCameraInput cameraInput;

  bool hasContext;
  vdkContext *pVDKContext;
  vcRenderContext *pRenderContext;
  vcConvertContext *pConvertContext;

  char password[vcMaxPathLength];
  const char *pLoginErrorMessage;

  char modelPath[vcMaxPathLength];

  vcSettings settings;
  udValue projects;
};

#endif // !vcState_h__
