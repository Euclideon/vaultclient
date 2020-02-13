#ifndef vcState_h__
#define vcState_h__

#include "udPlatformUtil.h"
#include "udMath.h"
#include "udChunkedArray.h"
#include "udJSON.h"
#include "udWorkerPool.h"

#include "vcImageRenderer.h"
#include "vcSettings.h"
#include "vcSceneItem.h"
#include "vcModel.h"
#include "vcGIS.h"
#include "vcFolder.h"
#include "vcStrings.h"
#include "vcProject.h"

#include "vdkError.h"
#include "vdkContext.h"

#include "imgui_ex/ImGuizmo.h"
#include "imgui_ex/vcFileDialog.h"

#include <vector>

struct SDL_Window;

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
  vcLS_NoServerURL,
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

enum vcErrorSource
{
  vcES_File,
  vcES_ProjectChange
};

struct vcState
{
  bool programComplete;
  SDL_Window *pWindow;
  vcFramebuffer *pDefaultFramebuffer;

  bool openSettings;
  int activeSetting;

  int openModals; // This is controlled inside vcModals.cpp
  bool modalOpen;

  vcCamera camera;
  vcCameraInput cameraInput;

  int settingsErrors;
  struct ErrorItem
  {
    vcErrorSource source;
    const char *pData;
    udResult resultCode;
  };

  udChunkedArray<const char*> loadList;
  udChunkedArray<ErrorItem> errorItems;

  const char *pLoadImage;
  udWorkerPool *pWorkerPool;

  double deltaTime;
  udUInt2 sceneResolution;

  vcGISSpace gis;

  vcTexture *pCompanyLogo;
  vcTexture *pCompanyWatermark;
  vcTexture *pSceneWatermark;
  vcTexture *pUITexture;
  vcTexture *pWhiteTexture;

  bool isUsingAnchorPoint;
  udDouble3 worldAnchorPoint;
  udRay<double> anchorMouseRay;
  udRay<double> worldMouseRay;

  udDouble3 worldMousePosCartesian;
  udDouble3 worldMousePosLongLat;
  bool pickingSuccess;
  int udModelPickedIndex;

  bool finishedStartup;
  bool forceLogout;

  bool hasContext;
  vdkSessionInfo sessionInfo;
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
  udJSON profileInfo;

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

  vcFileDialog fileDialog;

  int64_t lastEventTime;
  vcTranslationInfo languageInfo;
  bool showUI;
  vcDocks changeActiveDock;

  int currentKey;
};

#endif // !vcState_h__
