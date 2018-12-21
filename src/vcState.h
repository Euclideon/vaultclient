#ifndef vcState_h__
#define vcState_h__

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udJSON.h"

#include "vCore/vWorkerThread.h"

#include "vcSettings.h"
#include "vcScene.h"
#include "vcGIS.h"

#include "imgui_ex/ImGuizmo.h"

#include <vector>

struct SDL_Window;

struct vdkContext;

struct vcFramebuffer;
struct vcRenderContext;
struct vcCamera;
struct vcTexture;
struct vcConvertContext;

struct vcState
{
  bool programComplete;
  SDL_Window *pWindow;
  vcFramebuffer *pDefaultFramebuffer;

  bool onScreenControls;
  int openModals; // This is controlled inside vcModals.cpp

  vcCamera *pCamera;

  std::vector<const char*> loadList;
  std::vector<vcSceneItem*> sceneList;
  vWorkerThreadPool *pWorkerPool;

  size_t numSelectedModels;
  size_t prevSelectedModel;

  struct
  {
    size_t index;
    udJSON *pMetadata;
  } selectedModelProperties;

  double deltaTime;
  udUInt2 sceneResolution;

  vcGISSpace gis;
  char username[64];

  vcTexture *pCompanyLogo;
  vcTexture *pSceneWatermark;
  vcTexture *pUITexture;

  udDouble3 worldMousePos;
  bool pickingSuccess;
  udDouble3 previousWorldMousePos;
  bool previousPickingSuccess;

  vcCameraInput cameraInput;

  bool hasContext;
  bool forceLogout;
  int64_t lastServerAttempt;
  int64_t lastServerResponse;
  vdkContext *pVDKContext;
  vcRenderContext *pRenderContext;
  vcConvertContext *pConvertContext;

  char password[vcMaxPathLength];
  const char *pLoginErrorMessage;
  const char *pReleaseNotes; //Only loaded when requested
  bool passFocus = true;

  char modelPath[vcMaxPathLength];

  int renaming = -1;
  char renameText[30];

  vcSettings settings;
  udJSON projects;
  udJSON packageInfo;

  struct
  {
    vcGizmoOperation operation;
    vcGizmoCoordinateSystem coordinateSystem;
  } gizmo;

  struct
  {
    vcTexture *pServerIcon;
    volatile void *pImageData;
    volatile int64_t loadStatus; // >0 is the size of pImageData
  } tileModal;

  bool firstRun = true;
};

#endif // !vcState_h__
