#ifndef vcState_h__
#define vcState_h__

#include "udPlatformUtil.h"
#include "udMath.h"
#include "udChunkedArray.h"
#include "udJSON.h"

#include "vCore/vWorkerThread.h"

#include "vcImageRenderer.h"
#include "vcSettings.h"
#include "vcSceneItem.h"
#include "vcGIS.h"
#include "vcFolder.h"
#include "vcStrings.h"
#include "vcProject.h"

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
  vdkProjectNode *pParent;
  vdkProjectNode *pItem;
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


  struct FileError
  {
    const char *pFilename;
    udResult resultCode;
  };

  udChunkedArray<const char*> loadList;
  udChunkedArray<FileError> errorFiles;

  const char *pLoadImage;
  vWorkerThreadPool *pWorkerPool;

  double deltaTime;
  udUInt2 sceneResolution;

  vcGISSpace gis;
  udGeoZone defaultGeo;
  char username[64];

  vcTexture *pCompanyLogo;
  vcTexture *pBuildingsTexture;
  vcTexture *pSceneWatermark;
  vcTexture *pUITexture;

  udDouble3 worldMousePos;
  udDouble3 worldMousePosLongLat;
  udDouble3 worldMouseClickedPos;
  bool pickingSuccess;
  udDouble3 previousWorldMousePos;
  bool previousPickingSuccess;
  int previousUDModelPickedIndex;

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

  vcProject activeProject;

  struct
  {
    char selectUUIDWhenPossible[37];
    char movetoUUIDWhenPossible[37];

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

  int64_t lastEventTime;
  vcTranslationInfo languageInfo;
  bool showUI;
  vcDocks changeActiveDock;

  bool getGeo;
  udGeoZone *pGotGeo;
};

#endif // !vcState_h__
