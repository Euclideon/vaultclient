#include "vdkError.h"
#include "vcState.h"

#ifndef vcStrings_h__
#define vcStrings_h__

extern const char *pStrLoginConnectionError;
extern const char *pStrLoginAuthError;
extern const char *pStrLoginSyncError;
extern const char *pStrLoginSecurityError;
extern const char *pStrLoginServerError;
extern const char *pStrLoginProxyError;
extern const char *pStrLoginOtherError;

extern const char *pStrGeographicInfo;
extern const char *pStrNotGeolocated;
extern const char *pStrUnsupportedSRID;
extern const char *pStrMousePointInfo;
extern const char *pStrMousePointWGS;

extern const char *pStrOverrideSRID;
extern const char *pStrCameraSettings;
extern const char *pStrLockAltitude;
extern const char *pStrCameraInfo;
extern const char *pStrProjectionInfo;
extern const char *pStrGizmoTranslate;
extern const char *pStrGizmoRotate;
extern const char *pStrGizmoScale;
extern const char *pStrGizmoLocalSpace;
extern const char *pStrFullscreen;

extern const char *pStrCameraPosition;
extern const char *pStrCameraRotation;
extern const char *pStrMoveSpeed;
extern const char *pStrLat;
extern const char *pStrLong;
extern const char *pStrAlt;
extern const char *pStrCameraOutOfBounds;

extern const char *pStrOnScrnControls;
extern const char *pStrControls;
extern const char *pStrMoveCamera;
extern const char *pStrModelWatermark;
extern const char *pStrMapCopyright;
extern const char *pStrMapData;

extern const char *pStrMenuSystem;
extern const char *pStrMenuLogout;
extern const char *pStrMenuRestoreDefaults;
extern const char *pStrMenuAbout;
extern const char *pStrMenuReleaseNotes;
extern const char *pStrMenuQuit;

extern const char *pStrWindows;
extern const char *pStrScene;
extern const char *pStrSceneExplorer;
extern const char *pStrSettings;
extern const char *pStrProjects;
extern const char *pStrNewScene;
extern const char *pStrEndBarFiles;
extern const char *pStrInactive;
extern const char *pStrUpdateAvailable;
extern const char *pStrFPS;
extern const char *pStrLicense;
extern const char *pStrSecs;
extern const char *pStrLicenseExpired;
extern const char *pStrLicenseQueued;

extern const char *pStrRender;
extern const char *pStrConvert;

extern const char *pStrConnectionStatus;
extern const char *pStrPending;
extern const char *pStrLoading;
extern const char *pStrModelOpenFailure;
extern const char *pStrModelLoadFailure;
extern const char *pStrWatermark;
extern const char *pStrChecking;
extern const char *pStrLogin;
extern const char *pStrServerURL;
extern const char *pStrUsername;
extern const char *pStrShow;
extern const char *pStrPassword;
extern const char *pStrCredentials;
extern const char *pStrLoginButton;
extern const char *pStrCapsWarning;
extern const char *pStrAdvancedSettings;
extern const char *pStrProxyAddress;
extern const char *pStrIgnoreCert;
extern const char *pStrIgnoreCertWarning;
extern const char *pStrLoginScreenPopups;
extern const char *pStrReleaseNotes;
extern const char *pStrAbout;
extern const char *pStrAddUDS;
extern const char *pStrAddPOI;
extern const char *pStrAddAOI;
extern const char *pStrAddLines;
extern const char *pStrAddFolder;
extern const char *pStrRemove;

extern const char *pStrAxisFlip;
extern const char *pStrUseProjection;
extern const char *pStrMoveTo;
extern const char *pStrProperties;

extern const char *pStrTheme;
extern const char *pStrShowDiagnostics;
extern const char *pStrAdvancedGIS;
extern const char *pStrLimitFPS;
extern const char *pStrShowCompass;
extern const char *pStrMouseAnchor;
extern const char *pStrVoxelShape;
extern const char *pStrOnScreenControls;
extern const char *pStrTouchUI;
extern const char *pStrInvertX;
extern const char *pStrInvertY;
extern const char *pStrMousePivot;
extern const char *pStrTumble;
extern const char *pStrOrbit;
extern const char *pStrPan;
extern const char *pStrDolly;
extern const char *pStrChangeMoveSpeed;
extern const char *pStrLeft;
extern const char *pStrMiddle;
extern const char *pStrRight;
extern const char *pStrScrollWheel;

extern const char *pStrNearPlane;
extern const char *pStrFarPlane;
extern const char *pStrCameraLense;
extern const char *pStrFOV;
extern const char *pStrMapTiles;
extern const char *pStrMouseLock;
extern const char *pStrTileServer;
extern const char *pStrMapHeight;
extern const char *pStrHybrid;
extern const char *pStrOverlay;
extern const char *pStrUnderlay;
extern const char *pStrBlending;
extern const char *pStrTransparency;
extern const char *pStrSetHeight;
extern const char *pStrColour;
extern const char *pStrIntensity;
extern const char *pStrClassification;
extern const char *pStrDisplayMode;
extern const char *pStrMinIntensity;
extern const char *pStrMaxIntensity;
extern const char *pStrShowColorTable;

extern const char *pStrColorNeverClassified;
extern const char *pStrColorUnclassified;
extern const char *pStrColorGround;
extern const char *pStrColorLowVegetation;
extern const char *pStrColorMediumVegetation;
extern const char *pStrColorHighVegetation;
extern const char *pStrColorBuilding;
extern const char *pStrColorLowPoint;
extern const char *pStrColorKeyPoint;
extern const char *pStrColorWater;
extern const char *pStrColorRail;
extern const char *pStrColorRoadSurface;
extern const char *pStrColorReserved;
extern const char *pStrColorWireGuard;
extern const char *pStrColorWireConductor;
extern const char *pStrColorTransmissionTower;
extern const char *pStrColorWireStructureConnector;
extern const char *pStrColorBridgeDeck;
extern const char *pStrColorHighNoise;
extern const char *pStrColorReservedColors;

extern const char *pStrReserved;
extern const char *pStrColorUserDefinable;
extern const char *pStrUserDefined;
extern const char *pStrRename;
extern const char *pStrEdgeHighlighting;
extern const char *pStrEdgeWidth;
extern const char *pStrEdgeThreshold;
extern const char *pStrEdgeColour;
extern const char *pStrColourHeight;
extern const char *pStrColourStart;
extern const char *pStrColourEnd;
extern const char *pStrColourHeightStart;
extern const char *pStrColourHeightEnd;
extern const char *pStrColourDepth;
extern const char *pStrDepthColour;
extern const char *pStrColourDepthStart;
extern const char *pStrColourDepthEnd;
extern const char *pStrEnableContours;
extern const char *pStrContoursColour;
extern const char *pStrContoursDistances;
extern const char *pStrContoursBandHeight;

extern const char *pStrModelProperties;
extern const char *pStrName;
extern const char *pStrPath;
extern const char *pStrNoModelInfo;
extern const char *pStrClose;
extern const char *pStrSet;

extern const char *pStrLoggedOut;
extern const char *pStrLogged;
extern const char *pStrReleaseShort;
extern const char *pStrCurrentVersion;
extern const char *pStrReleaseNotesFail;
extern const char *pStrVersion;
extern const char *pStr3rdPartyLic;
extern const char *pStrUpdateAvailableLong;
extern const char *pStrInYourVaultServer;
extern const char *pStrLicenses;
extern const char *pStrNewVersionAvailable;
extern const char *pStrNewVersion;
extern const char *pStrDownloadPrompt;
extern const char *pStrImageFormat;
extern const char *pStrLoadingWait;
extern const char *pStrErrorFetching;
extern const char *pStrSceneAddUDS;
extern const char *pStrPathURL;
extern const char *pStrLoad;
extern const char *pStrCancel;
extern const char *pStrNotImplemented;
extern const char *pStrNotAvailable;
extern const char *pStrSRID;
extern const char *pStrPresentationUI;

extern const char *pStrHide;
extern const char *pStrShow;
extern const char *pStrResponsive;
extern const char *pStrNone;
extern const char *pStrCompass;
extern const char *pStrRectangles;
extern const char *pStrCubes;
extern const char *pStrPoints;
extern const char *pStrDark;
extern const char *pStrLight;

extern const char *pStrAppName;

extern const char *pStrVisualization;
extern const char *pStrElevation;
extern const char *pStrDegrees;
extern const char *pStrViewport;
extern const char *pStrInputControls;
extern const char *pStrAppearance;

extern const char *pStrRemember;

extern const char *pStrDeleteKey;
extern const char *pStrLockAltKey;
extern const char *pStrTranslateKey;
extern const char *pStrRotateKey;
extern const char *pStrScaleKey;
extern const char *pStrLocalKey;
extern const char *pStrFullscreenKey;

extern const char *pStrLoginWaiting;
extern const char *pStrRememberServer;
extern const char *pStrRememberUser;
extern const char *pStrSettingsAppearance;
extern const char *pStrThemeOptions;
extern const char *pStrResponsiveOptions;
extern const char *pStrAnchorOptions;
extern const char *pStrVoxelOptions;
extern const char *pStrInputControlsID;
extern const char *pStrViewportID;
extern const char *pStrDegreesFormat;
extern const char *pStrElevationFormat;
extern const char *pStrVisualizationFormat;
extern const char *pStrRestoreColorsID;
extern const char *pStrLatLongAlt;
extern const char *pStrInactiveSlash;

extern const char *pStrPackageUpdate;

vdkError vcStrings_LoadStrings(vcState *pProgramState, const char *pFilename = nullptr);
void vcStrings_FreeStrings();

#endif //vcStrings_h__
