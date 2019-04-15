#ifndef vcState_h__
#define vcState_h__

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udJSON.h"

#include "vCore/vWorkerThread.h"

#include "vcImageRenderer.h"

#include "vcSettings.h"
#include "vcScene.h"
#include "vcGIS.h"
#include "vcFolder.h"
#include "vdkError.h"

#include "imgui_ex/ImGuizmo.h"

#include <vector>

struct SDL_Window;

struct vdkContext;

struct vcFramebuffer;
struct vcRenderContext;
struct vcCamera;
struct vcTexture;
struct vcConvertContext;

struct vcSceneItemRef
{
  vcFolder *pParent;
  size_t index;
};

enum vcLoginStatus
{
  vcLS_NoStatus, // Used temporarily at startup and after logout to set focus on the correct fields
  vcLS_EnterCredentials,
  vcLS_Pending,
  vcLS_ConnectionError,
  vcLS_AuthError,
  vcLS_TimeSync,
  vcLS_SecurityError,
  vcLS_NegotiationError,
  vcLS_ProxyError,
  vcLS_ProxyAuthRequired,
  vcLS_ProxyAuthPending,
  vcLS_ProxyAuthFailed,
  vcLS_OtherError
};

struct vcState
{
  bool programComplete;
  SDL_Window *pWindow;
  vcFramebuffer *pDefaultFramebuffer;

  int openModals; // This is controlled inside vcModals.cpp
  bool modalOpen;

  vcCamera *pCamera;

  std::vector<const char*> loadList;
  const char *pLoadImage;
  vWorkerThreadPool *pWorkerPool;

  double deltaTime;
  udUInt2 sceneResolution;

  vcGISSpace gis;
  char username[64];

  vcTexture *pCompanyLogo;
  vcTexture *pBuildingsTexture;
  vcTexture *pSceneWatermark;
  vcTexture *pUITexture;

  udDouble3 worldMousePos;
  bool pickingSuccess;
  udDouble3 previousWorldMousePos;
  bool previousPickingSuccess;

  vcCameraInput cameraInput;

  bool hasContext;
  bool forceLogout;
  double lastServerAttempt;
  double lastServerResponse;
  vdkContext *pVDKContext;
  vcRenderContext *pRenderContext;
  vcConvertContext *pConvertContext;

  char password[vcMaxPathLength];

  vcLoginStatus loginStatus;
  vdkError logoutReason;

  const char *pReleaseNotes; //Only loaded when requested
  bool passFocus;

  char modelPath[vcMaxPathLength];

  int renaming;
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

  struct
  {
    vcFolder *pItems;

    vcSceneItemRef insertItem;
    vcSceneItemRef clickedItem;
    std::vector<vcSceneItemRef> selectedItems;
  } sceneExplorer;

  struct ImageInfo
  {
    vcTexture *pImage;
    int width;
    int height;

    vcImageType imageType;
  } image;

  bool firstRun;
  vdkError currentError;
  int64_t lastEventTime;
  bool showUI;
  vcDocks changeActiveDock;
};

#endif // !vcState_h__
