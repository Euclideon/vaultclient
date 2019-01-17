#include "vcStrings.h"
#include "vCore/vStringFormat.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udJSON.h"

const char *pStrLoginConnectionError;
const char *pStrLoginAuthError;
const char *pStrLoginSyncError;
const char *pStrLoginSecurityError;
const char *pStrLoginServerError;
const char *pStrLoginProxyError;
const char *pStrLoginOtherError;

const char *pStrGeographicInfo;
const char *pStrNotGeolocated;
const char *pStrUnsupportedSRID;
const char *pStrMousePointInfo;
const char *pStrMousePointWGS;

const char *pStrOverrideSRID;
const char *pStrCameraSettings;
const char *pStrLockAltitude;
const char *pStrCameraInfo;
const char *pStrProjectionInfo;
const char *pStrGizmoTranslate;
const char *pStrGizmoRotate;
const char *pStrGizmoScale;
const char *pStrGizmoLocalSpace;
const char *pStrFullscreen;

const char *pStrCameraPosition;
const char *pStrCameraRotation;
const char *pStrMoveSpeed;
const char *pStrLat;
const char *pStrLong;
const char *pStrAlt;
const char *pStrCameraOutOfBounds;

const char *pStrOnScrnControls;
const char *pStrControls;
const char *pStrMoveCamera;
const char *pStrModelWatermark;
const char *pStrMapCopyright;
const char *pStrMapData;

const char *pStrMenuSystem;
const char *pStrMenuLogout;
const char *pStrMenuRestoreDefaults;
const char *pStrMenuAbout;
const char *pStrMenuReleaseNotes;
const char *pStrMenuQuit;

const char *pStrWindows;
const char *pStrScene;
const char *pStrSceneExplorer;
const char *pStrSettings;
const char *pStrProjects;
const char *pStrNewScene;
const char *pStrEndBarFiles;
const char *pStrInactive;
const char *pStrUpdateAvailable;
const char *pStrFPS;
const char *pStrLicense;
const char *pStrSecs;
const char *pStrLicenseExpired;
const char *pStrLicenseQueued;

const char *pStrRender;
const char *pStrConvert;

const char *pStrConnectionStatus;
const char *pStrPending;
const char *pStrLoading;
const char *pStrModelOpenFailure;
const char *pStrModelLoadFailure;
const char *pStrWatermark;
const char *pStrChecking;
const char *pStrLogin;
const char *pStrServerURL;
const char *pStrUsername;
const char *pStrShow;
const char *pStrPassword;
const char *pStrCredentials;
const char *pStrLoginButton;
const char *pStrCapsWarning;
const char *pStrAdvancedSettings;
const char *pStrProxyAddress;
const char *pStrIgnoreCert;
const char *pStrIgnoreCertWarning;
const char *pStrLoginScreenPopups;
const char *pStrReleaseNotes;
const char *pStrAbout;
const char *pStrAddUDS;
const char *pStrAddPOI;
const char *pStrAddAOI;
const char *pStrAddLines;
const char *pStrAddFolder;
const char *pStrRemove;

const char *pStrAxisFlip;
const char *pStrUseProjection;
const char *pStrMoveTo;
const char *pStrProperties;

const char *pStrTheme;
const char *pStrShowDiagnostics;
const char *pStrAdvancedGIS;
const char *pStrLimitFPS;
const char *pStrShowCompass;
const char *pStrMouseAnchor;
const char *pStrVoxelShape;
const char *pStrOnScreenControls;
const char *pStrTouchUI;
const char *pStrInvertX;
const char *pStrInvertY;
const char *pStrMousePivot;
const char *pStrTumble;
const char *pStrOrbit;
const char *pStrPan;
const char *pStrDolly;
const char *pStrChangeMoveSpeed;
const char *pStrLeft;
const char *pStrMiddle;
const char *pStrRight;
const char *pStrScrollWheel;

const char *pStrNearPlane;
const char *pStrFarPlane;
const char *pStrCameraLense;
const char *pStrFOV;
const char *pStrMapTiles;
const char *pStrMouseLock;
const char *pStrTileServer;
const char *pStrMapHeight;
const char *pStrHybrid;
const char *pStrOverlay;
const char *pStrUnderlay;
const char *pStrBlending;
const char *pStrTransparency;
const char *pStrSetHeight;
const char *pStrColour;
const char *pStrIntensity;
const char *pStrClassification;
const char *pStrDisplayMode;
const char *pStrMinIntensity;
const char *pStrMaxIntensity;
const char *pStrShowColorTable;

const char *pStrColorNeverClassified;
const char *pStrColorUnclassified;
const char *pStrColorGround;
const char *pStrColorLowVegetation;
const char *pStrColorMediumVegetation;
const char *pStrColorHighVegetation;
const char *pStrColorBuilding;
const char *pStrColorLowPoint;
const char *pStrColorKeyPoint;
const char *pStrColorWater;
const char *pStrColorRail;
const char *pStrColorRoadSurface;
const char *pStrColorReserved;
const char *pStrColorWireGuard;
const char *pStrColorWireConductor;
const char *pStrColorTransmissionTower;
const char *pStrColorWireStructureConnector;
const char *pStrColorBridgeDeck;
const char *pStrColorHighNoise;
const char *pStrColorReservedColors;

const char *pStrReserved;
const char *pStrColorUserDefinable;
const char *pStrUserDefined;
const char *pStrRename;
const char *pStrEdgeHighlighting;
const char *pStrEdgeWidth;
const char *pStrEdgeThreshold;
const char *pStrEdgeColour;
const char *pStrColourHeight;
const char *pStrColourStart;
const char *pStrColourEnd;
const char *pStrColourHeightStart;
const char *pStrColourHeightEnd;
const char *pStrColourDepth;
const char *pStrDepthColour;
const char *pStrColourDepthStart;
const char *pStrColourDepthEnd;
const char *pStrEnableContours;
const char *pStrContoursColour;
const char *pStrContoursDistances;
const char *pStrContoursBandHeight;

const char *pStrModelProperties;
const char *pStrName;
const char *pStrPath;
const char *pStrNoModelInfo;
const char *pStrClose;
const char *pStrSet;

const char *pStrLoggedOut;
const char *pStrLogged;
const char *pStrReleaseShort;
const char *pStrCurrentVersion;
const char *pStrReleaseNotesFail;
const char *pStrVersion;
const char *pStr3rdPartyLic;
const char *pStrUpdateAvailableLong;
const char *pStrInYourVaultServer;
const char *pStrLicenses;
const char *pStrNewVersionAvailable;
const char *pStrNewVersion;
const char *pStrDownloadPrompt;
const char *pStrImageFormat;
const char *pStrLoadingWait;
const char *pStrErrorFetching;
const char *pStrSceneAddUDS;
const char *pStrPathURL;
const char *pStrLoad;
const char *pStrCancel;
const char *pStrNotImplemented;
const char *pStrNotAvailable;
const char *pStrSRID;
const char *pStrPresentationUI;

const char *pStrHide;
const char *pStrResponsive;
const char *pStrNone;
const char *pStrCompass;
const char *pStrRectangles;
const char *pStrCubes;
const char *pStrPoints;
const char *pStrDark;
const char *pStrLight;

const char *pStrAppName;

const char *pStrVisualization;
const char *pStrElevation;
const char *pStrDegrees;
const char *pStrViewport;
const char *pStrInputControls;
const char *pStrAppearance;
const char *pStrRemember;

const char *pStrDeleteKey;
const char *pStrLockAltKey;
const char *pStrTranslateKey;
const char *pStrRotateKey;
const char *pStrScaleKey;
const char *pStrLocalKey;
const char *pStrFullscreenKey;

const char *pStrLoginWaiting;
const char *pStrRememberServer;
const char *pStrRememberUser;
const char *pStrSettingsAppearance;
const char *pStrThemeOptions;
const char *pStrResponsiveOptions;
const char *pStrAnchorOptions;
const char *pStrVoxelOptions;
const char *pStrInputControlsID;
const char *pStrViewportID;
const char *pStrDegreesFormat;
const char *pStrElevationFormat;
const char *pStrVisualizationFormat;
const char *pStrRestoreColorsID;
const char *pStrLatLongAlt;
const char *pStrInactiveSlash;

const char *pStrPackageUpdate;

vdkError vcStrings_LoadStrings(vcState *pProgramState, const char *pFilename = "English.json")
{
  char *pPos;
  if (udFile_Load(pFilename, (void **)&pPos) != udR_Success)
    return vE_OpenFailure;

  udJSON strings;

  if (strings.Parse(pPos) != udR_Success)
    return vE_InvalidParameter;

  udFree(pPos);

  pStrLoginConnectionError = udStrdup(strings.Get("LoginConnectionError").AsString("Could not connect to server."));
  pStrLoginAuthError = udStrdup(strings.Get("LoginAuthError").AsString("Username or Password incorrect."));
  pStrLoginSyncError = udStrdup(strings.Get("LoginSyncError").AsString("Your clock doesn't match the remote server clock."));
  pStrLoginSecurityError = udStrdup(strings.Get("LoginSecurityError").AsString("Could not open a secure channel to the server."));
  pStrLoginServerError = udStrdup(strings.Get("LoginServerError").AsString("Unable to negotiate with server, please confirm the server address"));
  pStrLoginProxyError = udStrdup(strings.Get("LoginProxyError").AsString("Unable to negotiate with proxy server, please confirm the proxy server address"));
  pStrLoginOtherError = udStrdup(strings.Get("LoginOtherError").AsString("Unknown error occurred, please try again later."));

  pStrGeographicInfo = udStrdup(strings.Get("GeographicInfo").AsString("Geographic Information"));
  pStrNotGeolocated = udStrdup(strings.Get("NotGeolocated").AsString("Not Geolocated"));
  pStrUnsupportedSRID = udStrdup(strings.Get("UnsupportedSRID").AsString("Unsupported SRID"));
  pStrMousePointInfo = udStrdup(strings.Get("MousePointInfo").AsString("Mouse Point (Projected)"));
  pStrMousePointWGS = udStrdup(strings.Get("MousePointWGS").AsString("Mouse Point (WGS84)"));

  pStrOverrideSRID = udStrdup(strings.Get("OverrideSRID").AsString("Override SRID"));
  pStrCameraSettings = udStrdup(strings.Get("CameraSettings").AsString("Camera Settings"));
  pStrLockAltitude = udStrdup(strings.Get("LockAltitude").AsString("Lock Altitude"));
  pStrCameraInfo = udStrdup(strings.Get("CameraInfo").AsString("Show Camera Information"));
  pStrProjectionInfo = udStrdup(strings.Get("ProjectionInfo").AsString("Show Projection Information"));
  pStrGizmoTranslate = udStrdup(strings.Get("GizmoTranslate").AsString("Gizmo Translate"));
  pStrGizmoRotate = udStrdup(strings.Get("GizmoRotate").AsString("Gizmo Rotate"));
  pStrGizmoScale = udStrdup(strings.Get("GizmoScale").AsString("Gizmo Scale"));
  pStrGizmoLocalSpace = udStrdup(strings.Get("GizmoLocalSpace").AsString("Gizmo Local Space"));
  pStrFullscreen = udStrdup(strings.Get("Fullscreen").AsString("Fullscreen"));

  pStrCameraPosition = udStrdup(strings.Get("CameraPosition").AsString("Camera Position"));
  pStrCameraRotation = udStrdup(strings.Get("CameraRotation").AsString("Camera Rotation"));
  pStrMoveSpeed = udStrdup(strings.Get("MoveSpeed").AsString("Move Speed"));
  pStrLat = udStrdup(strings.Get("Lat").AsString("Lat"));
  pStrLong = udStrdup(strings.Get("Long").AsString("Long"));
  pStrAlt = udStrdup(strings.Get("Alt").AsString("Alt"));
  pStrCameraOutOfBounds = udStrdup(strings.Get("CameraOutOfBounds").AsString("Camera is outside recommended limits of this GeoZone"));

  pStrOnScrnControls = udStrdup(strings.Get("OnScrnControls").AsString("OnScreenControls"));
  pStrControls = udStrdup(strings.Get("Controls").AsString("Controls"));
  pStrMoveCamera = udStrdup(strings.Get("MoveCamera").AsString("Move Camera"));
  pStrModelWatermark = udStrdup(strings.Get("ModelWatermark").AsString("ModelWatermark"));
  pStrMapCopyright = udStrdup(strings.Get("MapCopyright").AsString("MapCopyright"));
  pStrMapData = udStrdup(strings.Get("MapData").AsString("Map Data \xC2\xA9 OpenStreetMap contributors"));

  pStrMenuSystem = udStrdup(strings.Get("MenuSystem").AsString("System"));
  pStrMenuLogout = udStrdup(strings.Get("MenuLogout").AsString("Logout"));
  pStrMenuRestoreDefaults = udStrdup(strings.Get("MenuRestoreDefaults").AsString("Restore Defaults"));
  pStrMenuAbout = udStrdup(strings.Get("MenuAbout").AsString("About"));
  pStrMenuReleaseNotes = udStrdup(strings.Get("MenuReleaseNotes").AsString("Release Notes"));
  pStrMenuQuit = udStrdup(strings.Get("MenuQuit").AsString("Quit"));

  pStrWindows = udStrdup(strings.Get("Windows").AsString("Windows"));
  pStrScene = udStrdup(strings.Get("Scene").AsString("Scene"));
  pStrSceneExplorer = udStrdup(strings.Get("SceneExplorer").AsString("Scene Explorer"));
  pStrSettings = udStrdup(strings.Get("Settings").AsString("Settings"));
  pStrProjects = udStrdup(strings.Get("Projects").AsString("Projects"));
  pStrNewScene = udStrdup(strings.Get("NewScene").AsString("New Scene"));
  pStrEndBarFiles = udStrdup(strings.Get("EndBarFiles").AsString("Files Queued"));
  pStrInactive = udStrdup(strings.Get("Inactive").AsString("Inactive"));
  pStrUpdateAvailable = udStrdup(strings.Get("UpdateAvailable").AsString("Update Available"));
  pStrFPS = udStrdup(strings.Get("FPS").AsString("FPS"));
  pStrLicense = udStrdup(strings.Get("License").AsString("License"));
  pStrSecs = udStrdup(strings.Get("Secs").AsString("secs"));
  pStrLicenseExpired = udStrdup(strings.Get("LicenseExpired").AsString("License (expired)"));
  pStrLicenseQueued = udStrdup(strings.Get("LicenseQueued").AsString("in Queue"));

  pStrRender = udStrdup(strings.Get("Render").AsString("Render"));
  pStrConvert = udStrdup(strings.Get("Convert").AsString("Convert"));

  pStrConnectionStatus = udStrdup(strings.Get("ConnectionStatus").AsString("Connection Status"));
  pStrPending = udStrdup(strings.Get("Pending").AsString("Pending"));
  pStrLoading = udStrdup(strings.Get("Loading").AsString("Loading"));
  pStrModelOpenFailure = udStrdup(strings.Get("ModelOpenFailure").AsString("Could not open the model, perhaps it is missing or you don't have permission to access it."));
  pStrModelLoadFailure = udStrdup(strings.Get("ModelLoadFailure").AsString("Failed to load model"));
  pStrWatermark = udStrdup(strings.Get("Watermark").AsString("Watermark"));
  pStrChecking = udStrdup(strings.Get("Checking").AsString("Checking with server..."));
  pStrLogin = udStrdup(strings.Get("Login").AsString("Login"));
  pStrServerURL = udStrdup(strings.Get("ServerURL").AsString("ServerURL"));
  pStrUsername = udStrdup(strings.Get("Username").AsString("Username"));
  pStrShow = udStrdup(strings.Get("Show").AsString("Show"));
  pStrPassword = udStrdup(strings.Get("Password").AsString("Password"));
  pStrCredentials = udStrdup(strings.Get("Credentials").AsString("Please enter your credentials..."));
  pStrLoginButton = udStrdup(strings.Get("LoginButton").AsString("Login!"));
  pStrCapsWarning = udStrdup(strings.Get("CapsWarning").AsString("Caps Lock is Enabled!"));
  pStrAdvancedSettings = udStrdup(strings.Get("AdvancedSettings").AsString("Advanced Connection Settings"));
  pStrProxyAddress = udStrdup(strings.Get("ProxyAddress").AsString("Proxy Address"));
  pStrIgnoreCert = udStrdup(strings.Get("IgnoreCert").AsString("Ignore Certificate Verification"));
  pStrIgnoreCertWarning = udStrdup(strings.Get("IgnoreCertWarning").AsString("THIS IS A DANGEROUS SETTING, ONLY SET THIS ON REQUEST FROM YOUR SYSTEM ADMINISTRATOR"));
  pStrLoginScreenPopups = udStrdup(strings.Get("LoginScreenPopups").AsString("LoginScreenPopups"));
  pStrReleaseNotes = udStrdup(strings.Get("ReleaseNotes").AsString("Release Notes"));
  pStrAbout = udStrdup(strings.Get("About").AsString("About"));
  pStrAddUDS = udStrdup(strings.Get("AddUDS").AsString("Add UDS"));
  pStrAddPOI = udStrdup(strings.Get("AddPOI").AsString("Add Point of Interest"));
  pStrAddAOI = udStrdup(strings.Get("AddAOI").AsString("Add Area of Interest"));
  pStrAddLines = udStrdup(strings.Get("AddLines").AsString("Add Lines"));
  pStrAddFolder = udStrdup(strings.Get("AddFolder").AsString("Add Folder"));
  pStrRemove = udStrdup(strings.Get("Remove").AsString("Remove Selected"));

  pStrAxisFlip = udStrdup(strings.Get("AxisFlip").AsString("Flip Y/Z Up"));
  pStrUseProjection = udStrdup(strings.Get("UseProjection").AsString("Use Projection"));
  pStrMoveTo = udStrdup(strings.Get("MoveTo").AsString("Move To"));
  pStrProperties = udStrdup(strings.Get("Properties").AsString("Properties"));

  pStrTheme = udStrdup(strings.Get("Theme").AsString("Theme"));
  pStrShowDiagnostics = udStrdup(strings.Get("ShowDiagnostics").AsString("Show Diagnostic Information"));
  pStrAdvancedGIS = udStrdup(strings.Get("AdvancedGIS").AsString("Show Advanced GIS Settings"));
  pStrLimitFPS = udStrdup(strings.Get("LimitFPS").AsString("Limit FPS In Background"));
  pStrShowCompass = udStrdup(strings.Get("ShowCompass").AsString("Show Compass On Screen"));
  pStrMouseAnchor = udStrdup(strings.Get("MouseAnchor").AsString("Mouse Anchor Style"));
  pStrVoxelShape = udStrdup(strings.Get("VoxelShape").AsString("Voxel Shape"));
  pStrOnScreenControls = udStrdup(strings.Get("OnScreenControls").AsString("On Screen Controls"));
  pStrTouchUI = udStrdup(strings.Get("TouchUI").AsString("Touch Friendly UI"));
  pStrInvertX = udStrdup(strings.Get("InvertX").AsString("Invert X-axis"));
  pStrInvertY = udStrdup(strings.Get("InvertY").AsString("Invert Y-axis"));
  pStrMousePivot = udStrdup(strings.Get("MousePivot").AsString("Mouse Pivot Bindings"));
  pStrTumble = udStrdup(strings.Get("Tumble").AsString("Tumble"));
  pStrPan = udStrdup(strings.Get("Pan").AsString("Pan"));
  pStrDolly = udStrdup(strings.Get("Dolly").AsString("Dolly"));
  pStrChangeMoveSpeed = udStrdup(strings.Get("ChangeMoveSpeed").AsString("Change Move Speed"));
  pStrLeft = udStrdup(strings.Get("Left").AsString("Left"));
  pStrMiddle = udStrdup(strings.Get("Middle").AsString("Middle"));
  pStrRight = udStrdup(strings.Get("Right").AsString("Right"));
  pStrScrollWheel = udStrdup(strings.Get("ScrollWheel").AsString("Scroll Wheel"));

  pStrNearPlane = udStrdup(strings.Get("NearPlane").AsString("Near Plane"));
  pStrFarPlane = udStrdup(strings.Get("FarPlane").AsString("Far Plane"));
  pStrCameraLense = udStrdup(strings.Get("CameraLense").AsString("Camera Lens (fov)"));
  pStrFOV = udStrdup(strings.Get("FOV").AsString("Field Of View"));
  pStrMapTiles = udStrdup(strings.Get("MapTiles").AsString("Map Tiles"));
  pStrMouseLock = udStrdup(strings.Get("MouseLock").AsString("Mouse can lock to maps"));
  pStrTileServer = udStrdup(strings.Get("TileServer").AsString("Tile Server"));
  pStrMapHeight = udStrdup(strings.Get("MapHeight").AsString("Map Height"));
  pStrHybrid = udStrdup(strings.Get("Hybrid").AsString("Hybrid"));
  pStrOverlay = udStrdup(strings.Get("Overlay").AsString("Overlay"));
  pStrUnderlay = udStrdup(strings.Get("Underlay").AsString("Underlay"));
  pStrBlending = udStrdup(strings.Get("Blending").AsString("Blending"));
  pStrTransparency = udStrdup(strings.Get("Transparency").AsString("Transparency"));
  pStrSetHeight = udStrdup(strings.Get("SetHeight").AsString("Set to Camera Height"));
  pStrColour = udStrdup(strings.Get("Colour").AsString("Colour"));
  pStrIntensity = udStrdup(strings.Get("Intensity").AsString("Intensity"));
  pStrClassification = udStrdup(strings.Get("Classification").AsString("Classification"));
  pStrDisplayMode = udStrdup(strings.Get("DisplayMode").AsString("Display Mode"));
  pStrMinIntensity = udStrdup(strings.Get("MinIntensity").AsString("Min Intensity"));
  pStrMaxIntensity = udStrdup(strings.Get("MaxIntensity").AsString("Max Intensity"));
  pStrShowColorTable = udStrdup(strings.Get("ShowColorTable").AsString("Show Custom Classification Color Table"));

  pStrColorNeverClassified = udStrdup(strings.Get("ColorNeverClassified").AsString("0. Never Classified"));
  pStrColorUnclassified = udStrdup(strings.Get("ColorUnclassified").AsString("1. Unclassified"));
  pStrColorGround = udStrdup(strings.Get("ColorGround").AsString("2. Ground"));
  pStrColorLowVegetation = udStrdup(strings.Get("ColorLowVegetation").AsString("3. Low Vegetation"));
  pStrColorMediumVegetation = udStrdup(strings.Get("ColorMediumVegetation").AsString("4. Medium Vegetation"));
  pStrColorHighVegetation = udStrdup(strings.Get("ColorHighVegetation").AsString("5. High Vegetation"));
  pStrColorBuilding = udStrdup(strings.Get("ColorBuilding").AsString("6. Building"));
  pStrColorLowPoint = udStrdup(strings.Get("ColorLowPoint").AsString("7. Low Point / Noise"));
  pStrColorKeyPoint = udStrdup(strings.Get("ColorKeyPoint").AsString("8. Key Point / Reserved"));
  pStrColorWater = udStrdup(strings.Get("ColorWater").AsString("9. Water"));
  pStrColorRail = udStrdup(strings.Get("ColorRail").AsString("10. Rail"));
  pStrColorRoadSurface = udStrdup(strings.Get("ColorRoadSurface").AsString("11. Road Surface"));
  pStrColorReserved = udStrdup(strings.Get("ColorReserved").AsString("12. Reserved"));
  pStrColorWireGuard = udStrdup(strings.Get("ColorWireGuard").AsString("13. Wire Guard / Shield"));
  pStrColorWireConductor = udStrdup(strings.Get("ColorWireConductor").AsString("14. Wire Conductor / Phase"));
  pStrColorTransmissionTower = udStrdup(strings.Get("ColorTransmissionTower").AsString("15. Transmission Tower"));
  pStrColorWireStructureConnector = udStrdup(strings.Get("ColorWireStructureConnector").AsString("16. Wire Structure Connector"));
  pStrColorBridgeDeck = udStrdup(strings.Get("ColorBridgeDeck").AsString("17. Bridge Deck"));
  pStrColorHighNoise = udStrdup(strings.Get("ColorHighNoise").AsString("18. High Noise"));
  pStrColorReservedColors = udStrdup(strings.Get("ColorReservedColors").AsString("19 - 63 Reserved"));

  pStrReserved = udStrdup(strings.Get("Reserved").AsString("Reserved"));
  pStrColorUserDefinable = udStrdup(strings.Get("ColorUserDefinable").AsString("64 - 255 User definable"));
  pStrUserDefined = udStrdup(strings.Get("UserDefined").AsString("User defined"));
  pStrRename = udStrdup(strings.Get("Rename").AsString("Rename"));
  pStrEdgeHighlighting = udStrdup(strings.Get("EdgeHighlighting").AsString("Enable Edge Highlighting"));
  pStrEdgeWidth = udStrdup(strings.Get("EdgeWidth").AsString("Edge Highlighting Width"));
  pStrEdgeThreshold = udStrdup(strings.Get("EdgeThreshold").AsString("Edge Highlighting Threshold"));
  pStrEdgeColour = udStrdup(strings.Get("EdgeColour").AsString("Edge Highlighting Colour"));
  pStrColourHeight = udStrdup(strings.Get("ColourHeight").AsString("Enable Colour by Height"));
  pStrColourStart = udStrdup(strings.Get("ColourStart").AsString("Colour by Height Start Colour"));
  pStrColourEnd = udStrdup(strings.Get("ColourEnd").AsString("Colour by Height End Colour"));
  pStrColourHeightStart = udStrdup(strings.Get("ColourHeightStart").AsString("Colour by Height Start Height"));
  pStrColourHeightEnd = udStrdup(strings.Get("ColourHeightEnd").AsString("Colour by Height End Height"));
  pStrColourDepth = udStrdup(strings.Get("ColourDepth").AsString("Enable Colour by Depth"));
  pStrDepthColour = udStrdup(strings.Get("DepthColour").AsString("Colour by Depth Colour"));
  pStrColourDepthStart = udStrdup(strings.Get("ColourDepthStart").AsString("Colour by Depth Start Depth"));
  pStrColourDepthEnd = udStrdup(strings.Get("ColourDepthEnd").AsString("Colour by Depth End Depth"));
  pStrEnableContours = udStrdup(strings.Get("EnableContours").AsString("Enable Contours"));
  pStrContoursColour = udStrdup(strings.Get("ContoursColour").AsString("Contours Colour"));
  pStrContoursDistances = udStrdup(strings.Get("ContoursDistances").AsString("Contours Distances"));
  pStrContoursBandHeight = udStrdup(strings.Get("ContoursBandHeight").AsString("Contours Band Height"));

  pStrModelProperties = udStrdup(strings.Get("ModelProperties").AsString("Model Properties"));
  pStrName = udStrdup(strings.Get("Name").AsString("Name"));
  pStrPath = udStrdup(strings.Get("Path").AsString("Path"));
  pStrNoModelInfo = udStrdup(strings.Get("NoModelInfo").AsString("No model information found."));
  pStrClose = udStrdup(strings.Get("Close").AsString("Close"));
  pStrSet = udStrdup(strings.Get("Set").AsString("Set"));

  pStrLoggedOut = udStrdup(strings.Get("LoggedOut").AsString("Logged Out"));
  pStrLogged = udStrdup(strings.Get("Logged").AsString("You were logged out."));
  pStrReleaseShort = udStrdup(strings.Get("ReleaseShort").AsString("ReleaseNotes"));
  pStrCurrentVersion = udStrdup(strings.Get("CurrentVersion").AsString("Current Version"));
  pStrReleaseNotesFail = udStrdup(strings.Get("ReleaseNotesFail").AsString("Unable to load release notes from package."));
  pStrVersion = udStrdup(strings.Get("Version").AsString("Version"));
  pStr3rdPartyLic = udStrdup(strings.Get("3rdPartyLic").AsString("Third Party License Information"));
  pStrUpdateAvailableLong = udStrdup(strings.Get("UpdateAvailableLong").AsString("Update Available to"));
  pStrInYourVaultServer = udStrdup(strings.Get("InYourVaultServer").AsString("in your Vault Server."));
  pStrLicenses = udStrdup(strings.Get("Licenses").AsString("Licenses"));
  pStrNewVersionAvailable = udStrdup(strings.Get("NewVersionAvailable").AsString("New Version Available"));
  pStrNewVersion = udStrdup(strings.Get("NewVersion").AsString("New Version"));
  pStrDownloadPrompt = udStrdup(strings.Get("DownloadPrompt").AsString("Please visit the Vault server in your browser to download the package."));
  pStrImageFormat = udStrdup(strings.Get("ImageFormat").AsString("Image Format"));
  pStrLoadingWait = udStrdup(strings.Get("LoadingWait").AsString("Loading... Please Wait"));
  pStrErrorFetching = udStrdup(strings.Get("ErrorFetching").AsString("Error fetching texture from url"));
  pStrSceneAddUDS = udStrdup(strings.Get("SceneAddUDS").AsString("Add UDS To Scene"));
  pStrPathURL = udStrdup(strings.Get("PathURL").AsString("Path/URL:"));
  pStrLoad = udStrdup(strings.Get("Load").AsString("Load!"));
  pStrCancel = udStrdup(strings.Get("Cancel").AsString("Cancel"));
  pStrNotImplemented = udStrdup(strings.Get("NotImplemented").AsString("Not Implemented"));
  pStrNotAvailable = udStrdup(strings.Get("NotAvailable").AsString("Sorry, this functionality is not yet available."));
  pStrSRID = udStrdup(strings.Get("SRID").AsString("SRID"));
  pStrPresentationUI = udStrdup(strings.Get("PresentationUI").AsString("Presentation UI"));

  pStrHide = udStrdup(strings.Get("Hide").AsString("Hide"));
  pStrResponsive = udStrdup(strings.Get("Responsive").AsString("Responsive"));
  pStrNone = udStrdup(strings.Get("None").AsString("None"));
  pStrOrbit = udStrdup(strings.Get("Orbit").AsString("Orbit"));
  pStrCompass = udStrdup(strings.Get("Compass").AsString("Compass"));
  pStrRectangles = udStrdup(strings.Get("Rectangles").AsString("Rectangles"));
  pStrCubes = udStrdup(strings.Get("Cubes").AsString("Cubes"));
  pStrPoints = udStrdup(strings.Get("Points").AsString("Points"));
  pStrDark = udStrdup(strings.Get("Dark").AsString("Dark"));
  pStrLight = udStrdup(strings.Get("Light").AsString("Light"));

  pStrAppName = udStrdup(strings.Get("AppName").AsString("Euclideon Vault Client"));

  pStrVisualization = udStrdup(strings.Get("Visualization").AsString("Visualization"));
  pStrElevation = udStrdup(strings.Get("Elevation").AsString("Maps & Elevation"));
  pStrDegrees = udStrdup(strings.Get("Degrees").AsString("Degrees"));
  pStrViewport = udStrdup(strings.Get("Viewport").AsString("Viewport"));
  pStrInputControls = udStrdup(strings.Get("InputControls").AsString("Input & Controls"));
  pStrAppearance = udStrdup(strings.Get("Appearance").AsString("Appearance"));

  pStrRemember = udStrdup(strings.Get("Remember").AsString("Remember"));

  pStrDeleteKey = udStrdup(strings.Get("DeleteKey").AsString("Delete"));
  pStrLockAltKey = udStrdup(strings.Get("LockAltKey").AsString("Space"));
  pStrTranslateKey = udStrdup(strings.Get("TranslateKey").AsString("B"));
  pStrRotateKey = udStrdup(strings.Get("RotateKey").AsString("N"));
  pStrScaleKey = udStrdup(strings.Get("ScaleKey").AsString("M"));
  pStrLocalKey = udStrdup(strings.Get("LocalKey").AsString("C"));
  pStrFullscreenKey = udStrdup(strings.Get("FullscreenKey").AsString("F5"));

  // Concatenations
  pStrLoginWaiting = vStringFormat("{0}##LoginWaiting", &pStrLogin, 1);
  pStrRememberServer = vStringFormat("{0}##rememberServerURL", &pStrRemember, 1);
  pStrRememberUser = vStringFormat("{0}##rememberUsername", &pStrRemember, 1);
  pStrSettingsAppearance = vStringFormat("{0}##Settings", &pStrAppearance, 1);
  const char *format[] = { pStrDark, pStrLight };
  pStrThemeOptions = vStringFormat("{0}|\0{1}|\0", format, 2);
  const char *format2[] = { pStrHide, pStrShow, pStrResponsive };
  pStrResponsiveOptions = vStringFormat("{0}|\0{1}|\0{2}|\0", format2, 3);
  const char *format3[] = { pStrNone, pStrOrbit, pStrCompass };
  pStrAnchorOptions = vStringFormat("{0}|\0{1}|\0{2}|\0", format3, 3);
  const char *format4[] = { pStrRectangles, pStrCubes, pStrPoints };
  pStrVoxelOptions = vStringFormat("{0}|\0{1}|\0{2}|\0", format4, 3);
  pStrInputControlsID = vStringFormat("{0}##Settings", &pStrInputControls, 1);
  pStrViewportID = vStringFormat("{0}##Settings", &pStrViewport, 1);
  pStrDegreesFormat = vStringFormat("%.0f {0}", &pStrDegrees, 1);
  pStrElevationFormat = vStringFormat("{0}##Settings", &pStrElevation, 1);
  pStrVisualizationFormat = vStringFormat("{0}##Settings", &pStrVisualization, 1);
  pStrRestoreColorsID = vStringFormat("{0}##RestoreClassificationColors", &pStrMenuRestoreDefaults, 1);
  const char *latLongAlt[] = { pStrLat, pStrLong, pStrAlt };
  pStrLatLongAlt = vStringFormat("{0}: %.7f, {1}: %.7f, {2}: %.2fm", latLongAlt, 3);
  pStrInactiveSlash = vStringFormat("{0} /", &pStrInactive, 1);

  const char *format5[] = { pStrUpdateAvailableLong, pProgramState->packageInfo.Get("package.versionstring").AsString(), pStrInYourVaultServer };
  if (format5[1] == nullptr)
    format5[1] = "";

  pStrPackageUpdate = vStringFormat("{0} {1} {2}", format5, 3);

  strings.Destroy();

  return vE_Success;
}

void vcStrings_FreeStrings()
{
  udFree(pStrLoginConnectionError);
  udFree(pStrLoginAuthError);
  udFree(pStrLoginSyncError);
  udFree(pStrLoginSecurityError);
  udFree(pStrLoginServerError);
  udFree(pStrLoginProxyError);
  udFree(pStrLoginOtherError);

  udFree(pStrGeographicInfo);
  udFree(pStrNotGeolocated);
  udFree(pStrUnsupportedSRID);
  udFree(pStrMousePointInfo);
  udFree(pStrMousePointWGS);

  udFree(pStrOverrideSRID);
  udFree(pStrCameraSettings);
  udFree(pStrLockAltitude);
  udFree(pStrCameraInfo);
  udFree(pStrProjectionInfo);
  udFree(pStrGizmoTranslate);
  udFree(pStrGizmoRotate);
  udFree(pStrGizmoScale);
  udFree(pStrGizmoLocalSpace);
  udFree(pStrFullscreen);

  udFree(pStrCameraPosition);
  udFree(pStrCameraRotation);
  udFree(pStrMoveSpeed);
  udFree(pStrLat);
  udFree(pStrLong);
  udFree(pStrAlt);
  udFree(pStrCameraOutOfBounds);

  udFree(pStrOnScrnControls);
  udFree(pStrControls);
  udFree(pStrMoveCamera);
  udFree(pStrModelWatermark);
  udFree(pStrMapCopyright);
  udFree(pStrMapData);

  udFree(pStrMenuSystem);
  udFree(pStrMenuLogout);
  udFree(pStrMenuRestoreDefaults);
  udFree(pStrMenuAbout);
  udFree(pStrMenuReleaseNotes);
  udFree(pStrMenuQuit);

  udFree(pStrWindows);
  udFree(pStrScene);
  udFree(pStrSceneExplorer);
  udFree(pStrSettings);
  udFree(pStrProjects);
  udFree(pStrNewScene);
  udFree(pStrEndBarFiles);
  udFree(pStrInactive);
  udFree(pStrUpdateAvailable);
  udFree(pStrFPS);
  udFree(pStrLicense);
  udFree(pStrSecs);
  udFree(pStrLicenseExpired);
  udFree(pStrLicenseQueued);

  udFree(pStrRender);
  udFree(pStrConvert);

  udFree(pStrConnectionStatus);
  udFree(pStrPending);
  udFree(pStrLoading);
  udFree(pStrModelOpenFailure);
  udFree(pStrModelLoadFailure);
  udFree(pStrWatermark);
  udFree(pStrChecking);
  udFree(pStrLogin);
  udFree(pStrServerURL);
  udFree(pStrUsername);
  udFree(pStrShow);
  udFree(pStrPassword);
  udFree(pStrCredentials);
  udFree(pStrLoginButton);
  udFree(pStrCapsWarning);
  udFree(pStrAdvancedSettings);
  udFree(pStrProxyAddress);
  udFree(pStrIgnoreCert);
  udFree(pStrIgnoreCertWarning);
  udFree(pStrLoginScreenPopups);
  udFree(pStrReleaseNotes);
  udFree(pStrAbout);
  udFree(pStrAddUDS);
  udFree(pStrAddPOI);
  udFree(pStrAddAOI);
  udFree(pStrAddLines);
  udFree(pStrAddFolder);
  udFree(pStrRemove);

  udFree(pStrAxisFlip);
  udFree(pStrUseProjection);
  udFree(pStrMoveTo);
  udFree(pStrProperties);

  udFree(pStrTheme);
  udFree(pStrShowDiagnostics);
  udFree(pStrAdvancedGIS);
  udFree(pStrLimitFPS);
  udFree(pStrShowCompass);
  udFree(pStrMouseAnchor);
  udFree(pStrVoxelShape);
  udFree(pStrOnScreenControls);
  udFree(pStrTouchUI);
  udFree(pStrInvertX);
  udFree(pStrInvertY);
  udFree(pStrMousePivot);
  udFree(pStrTumble);
  udFree(pStrOrbit);
  udFree(pStrPan);
  udFree(pStrDolly);
  udFree(pStrChangeMoveSpeed);
  udFree(pStrLeft);
  udFree(pStrMiddle);
  udFree(pStrRight);
  udFree(pStrScrollWheel);

  udFree(pStrNearPlane);
  udFree(pStrFarPlane);
  udFree(pStrCameraLense);
  udFree(pStrFOV);
  udFree(pStrMapTiles);
  udFree(pStrMouseLock);
  udFree(pStrTileServer);
  udFree(pStrMapHeight);
  udFree(pStrHybrid);
  udFree(pStrOverlay);
  udFree(pStrUnderlay);
  udFree(pStrBlending);
  udFree(pStrTransparency);
  udFree(pStrSetHeight);
  udFree(pStrColour);
  udFree(pStrIntensity);
  udFree(pStrClassification);
  udFree(pStrDisplayMode);
  udFree(pStrMinIntensity);
  udFree(pStrMaxIntensity);
  udFree(pStrShowColorTable);

  udFree(pStrColorNeverClassified);
  udFree(pStrColorUnclassified);
  udFree(pStrColorGround);
  udFree(pStrColorLowVegetation);
  udFree(pStrColorMediumVegetation);
  udFree(pStrColorHighVegetation);
  udFree(pStrColorBuilding);
  udFree(pStrColorLowPoint);
  udFree(pStrColorKeyPoint);
  udFree(pStrColorWater);
  udFree(pStrColorRail);
  udFree(pStrColorRoadSurface);
  udFree(pStrColorReserved);
  udFree(pStrColorWireGuard);
  udFree(pStrColorWireConductor);
  udFree(pStrColorTransmissionTower);
  udFree(pStrColorWireStructureConnector);
  udFree(pStrColorBridgeDeck);
  udFree(pStrColorHighNoise);
  udFree(pStrColorReservedColors);

  udFree(pStrReserved);
  udFree(pStrColorUserDefinable);
  udFree(pStrUserDefined);
  udFree(pStrRename);
  udFree(pStrEdgeHighlighting);
  udFree(pStrEdgeWidth);
  udFree(pStrEdgeThreshold);
  udFree(pStrEdgeColour);
  udFree(pStrColourHeight);
  udFree(pStrColourStart);
  udFree(pStrColourEnd);
  udFree(pStrColourHeightStart);
  udFree(pStrColourHeightEnd);
  udFree(pStrColourDepth);
  udFree(pStrDepthColour);
  udFree(pStrColourDepthStart);
  udFree(pStrColourDepthEnd);
  udFree(pStrEnableContours);
  udFree(pStrContoursColour);
  udFree(pStrContoursDistances);
  udFree(pStrContoursBandHeight);

  udFree(pStrModelProperties);
  udFree(pStrName);
  udFree(pStrPath);
  udFree(pStrNoModelInfo);
  udFree(pStrClose);
  udFree(pStrSet);

  udFree(pStrLoggedOut);
  udFree(pStrLogged);
  udFree(pStrReleaseShort);
  udFree(pStrCurrentVersion);
  udFree(pStrReleaseNotesFail);
  udFree(pStrVersion);
  udFree(pStr3rdPartyLic);
  udFree(pStrUpdateAvailableLong);
  udFree(pStrInYourVaultServer);
  udFree(pStrLicenses);
  udFree(pStrNewVersionAvailable);
  udFree(pStrNewVersion);
  udFree(pStrDownloadPrompt);
  udFree(pStrImageFormat);
  udFree(pStrLoadingWait);
  udFree(pStrErrorFetching);
  udFree(pStrSceneAddUDS);
  udFree(pStrPathURL);
  udFree(pStrLoad);
  udFree(pStrCancel);
  udFree(pStrNotImplemented);
  udFree(pStrNotAvailable);
  udFree(pStrSRID);
  udFree(pStrPresentationUI);

  udFree(pStrHide);
  udFree(pStrShow);
  udFree(pStrResponsive);
  udFree(pStrNone);
  udFree(pStrCompass);
  udFree(pStrRectangles);
  udFree(pStrCubes);
  udFree(pStrPoints);
  udFree(pStrDark);
  udFree(pStrLight);

  udFree(pStrAppName);

  udFree(pStrVisualization);
  udFree(pStrElevation);
  udFree(pStrDegrees);
  udFree(pStrViewport);
  udFree(pStrInputControls);
  udFree(pStrAppearance);

  udFree(pStrRemember);

  udFree(pStrDeleteKey);
  udFree(pStrLockAltKey);
  udFree(pStrTranslateKey);
  udFree(pStrRotateKey);
  udFree(pStrScaleKey);
  udFree(pStrLocalKey);
  udFree(pStrFullscreenKey);

  udFree(pStrLoginWaiting);
  udFree(pStrRememberServer);
  udFree(pStrRememberUser);
  udFree(pStrSettingsAppearance);
  udFree(pStrThemeOptions);
  udFree(pStrResponsiveOptions);
  udFree(pStrAnchorOptions);
  udFree(pStrVoxelOptions);
  udFree(pStrInputControlsID);
  udFree(pStrViewportID);
  udFree(pStrDegreesFormat);
  udFree(pStrElevationFormat);
  udFree(pStrVisualizationFormat);
  udFree(pStrRestoreColorsID);
  udFree(pStrLatLongAlt);
  udFree(pStrInactiveSlash);

  udFree(pStrPackageUpdate);
}
