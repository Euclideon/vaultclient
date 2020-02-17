# **Euclideon Vault Client Translation Guide**

## Changelist
- Version 0.5.0
  - Added:
    - Bindings Interface
      - `bindingsTitle`, `bindingsSettingsSave`, `bindingsColumnName`, `bindingsColumnKeyCombination`, `bindingsColumnDescriptions`, `bindingsErrorUnbound`, `bindingsClear`
    - Bindings Key Descriptions
      - `bindingsLockAltitude`, `bindingsGizmoTranslate`, `bindingsGizmoRotate`, `bindingsGizmoScale`, `bindingsGizmoLocalSpace`, `bindingsFullscreen`, `bindingsAddUDS`, `bindingsRemove`, `bindingsCameraForward`, `bindingsCameraBackward`, `bindingsCameraLeft`, `bindingsCameraRight`, `bindingsCameraUp`, `bindingsCameraDown`, `bindingsBindingsInterface`, `bindingsCancel`, `bindingsRenameSceneItem`, `bindingsSave`, `bindingsLoad`, `bindingsUndo`
    - Convert
      - `convertRestart`
    - Menus
      - `menuBindings`, `settingsAppearanceSaturation`, `menuProjectNone`, `menuProfileTitle`, `menuUserNameLabel`, `menuRealNameLabel`, `menuEmailLabel`
    - Polygon Meshes
      - `polyModelCullFace`, `polyModelCullFaceBack`, `polyModelCullFaceFront`, `polyModelCullFaceNone`, `polyModelIgnoreTint`
    - Scene
      - `sceneCameraAttachmentWarning`, `sceneCameraAttachmentDetach`
    - Settings
      - `settingsVisModeDisplacement`, `settingsVisDisplacementRange`, `settingsVisObjectHighlight`
    - Errors
      - `errorUnknown`, `errorCorruptData`, `errorUnsupported`, `errorOpenFailure`, `errorReadFailure`
  - Changed:
    - 
  - Removed:
    - Legacy Key Bindings
      - `sceneLockAltitudeKey`, `sceneGizmoTranslateKey`, `sceneGizmoRotateKey`, `sceneGizmoScaleKey`, `sceneGizmoLocalSpaceKey`, `sceneFullscreenKey`, `sceneMapModeKey`, `sceneExplorerRemoveKey`, `sceneExplorerAddUDSKey`
    - Other
      - `polyModelInvertFaces`, `menuBarConnectionStatus`

- Version 0.4.0
  - Added:
    - Settings
      - `settingsVisContoursRainbowRepeatRate`, `settingsVisContoursRainbowIntensity`, `settingsAppearanceLoginRenderLicense`, `settingsControlsMouseInvertX`, `settingsControlsMouseInvertY`, `settingsControlsControllerInvertX`,`settingsControlsControllerInvertY`, `settingsAppearanceShowEuclideonLogo`
    - Scene Explorer
      - `sceneExplorerProjectChangeFailedWrite`, `sceneExplorerProjectChangeFailedRead`, `sceneExplorerProjectChangeFailedParse`, `sceneExplorerSetButton`, `scenePOILabelImageTypeOriented`, `scenePOIAttachModel`, `scenePOIAttachModelURI`, `scenePOIAttachModelFailed`, `scenePOIAttachmentSpeed`, `sceneExplorerCompareModels`
    - Viewport (New Node Types)
      - View Sheds
        - `sceneAddViewShed`, `sceneExplorerViewShedDefaultName`, `viewShedDistance`, `viewShedVisibleColour`, `viewShedHiddenColour`
      - Polygon Meshes
        - `polyModelInvertFaces`, `polyModelMesh`, `polyModelTexture`, `polyModelMatColour`
      - Filters
        - `sceneAddFilter`, `sceneAddFilterBox`, `sceneAddFilterSphere`, `sceneAddFilterCylinder`, `sceneExplorerFilterBoxDefaultName`, `sceneExplorerFilterSphereDefaultName`, `sceneExplorerFilterCylinderDefaultName`, `sceneFilterPosition`, `sceneFilterRotation`, `sceneFilterExtents`, `sceneFilterShape`, `sceneFilterShapeBox`, `sceneFilterShapeSphere`, `sceneFilterShapeCylinder`, `sceneFilterInverted`
    - Convert
      - `convertAllSpaceLabel`, `convertSpaceECEF`, `convertSpaceDetected`
    - Misc
      - `loginEnterURL`
  - Changed:
    - Viewport
      - `sceneAddAOI` (improved clarity), `sceneAddLine` (improved clarity)
  - Removed:
    - `sceneExplorerProjectChangeFailedTitle`, `sceneExplorerProjectChangeSucceededTitle`, `settingsControlsInvertX`, `settingsControlsInvertY`

- Version 0.3.1
  - Added:
    - Settings
      - `settingsAppearanceShowSkybox`, `settingsAppearanceSkyboxColour`
    - Convert
      - `convertSetTempDirectoryTitle`
    - Viewport
      - `settingsViewportCameraLensCustom`, `settingsViewportCameraLens15mm`, `settingsViewportCameraLens24mm`, `settingsViewportCameraLens30mm`, `settingsViewportCameraLens50mm`, `settingsViewportCameraLens70mm`, `settingsViewportCameraLens100mm`, `menuBarWaitingForRenderLicense`
    - Scene Explorer
      - `scenePOIThumbnailSize`, `scenePOIThumbnailSizeNative`, `scenePOIThumbnailSizeSmall`, `scenePOIThumbnailSizeLarge`, `scenePOIReloadTime`
  - Changed:
    - Settings
      - `settingsVisDepth`, `settingsVisDepthColour`, `settingsVisDepthStart`, `settingsVisDepthEnd`
    - Scene Explorer
      - `scenePOILabelImageTypeStandard` (capitalised value), `scenePOILabelImageTypePanorama` (capitalised value), `scenePOILabelImageTypePhotosphere` (capitalised value)

- Version 0.3.0
  - Added:
    - Login Screen
      - `loginUserAgent`, `loginSelectUserAgent`, `loginRestoreDefaults`
    - Convert
      - `convertWriteFailed`, `convertParseError`, `convertImageParseError`, `convertNoFile`, `convertAddFileTitle`, `convertSetOutputTitle`, `convertAddNewJob`, `convertAddFile`, `convertSetOutput`, `convertWatermark`, `convertBeginPendingFiles`, `convertPendingProcessInProgress`, `convertPendingProcessing`, `convertNoJobSelected`, `convertChangeDefaultWatermark`
    - Menus & Modals
      - `menuProjectExport`, `menuProjectExportTitle`, `menuProjectImport`, `menuProjectImportTitle`, `menuBarFilesFailed`, `menuBarFilesFailedTooltip`, `menuLanguage`, `menuExperimentalFeatures`
    - Scene Explorer
      - `sceneExplorerRemoveItem`, `sceneExplorerUnknownCustomNode`, `sceneExplorerProjectChangeFailedTitle`, `sceneExplorerProjectChangeFailedMessage`, `sceneExplorerProjectReadOnlyTitle`, `sceneExplorerProjectReadOnlyMessage`, `sceneExplorerUnsupportedFilesTitle`, `sceneExplorerUnsupportedFilesMessage`, `sceneExplorerCloseButton`, `sceneViewpointSetCamera`, `sceneViewpointPosition`, `sceneViewpointRotation`, `sceneExplorerAddViewpoint`, `viewpointDefaultName`, `liveFeedDefaultName`, `sceneExplorerExportButton`, `sceneExplorerProjectChangeSucceededTitle`, `sceneExplorerProjectChangeSucceededMessage`, `sceneResetRotation`, `sceneExplorerClearAllButton`, `scenePOILineShowAllLengths`, `scenePOILineStyleDiagonal`, `sceneModelPosition`, `sceneModelRotation`, `sceneModelScale`
    - Settings
      - `settingsAppearanceTranslationDifferentVersion`, `settingsConvert`, `settingsConvertRestoreDefaults`,  `settingsVisHighlightColour`, `settingsVisHighlightThickness`
  - Removed:
    - Menu strings
      - `menuRestoreDefaults`
    - Project Menu & Modals
      - `menuImport`, `menuImportUDP`, `menuImportUDPTitle`
    - Convert
      - `convertRemove`
    - Scene Explorer
      - `sceneExplorerNotImplementedTitle`, `sceneExplorerNotImplementedMessage`, `sceneExplorerNotImplementedCloseButton`, `scenePOIBookmarkMode`, `scenePOISetBookmarkCamera`, `scenePOICameraPosition`, `scenePOICameraRotation`
    - Live Feeds
      - `liveFeedMode`, `liveFeedModeGroups`, `liveFeedModePosition`, `liveFeedModeCamera`, `liveFeedPosition`
    - Error Strings
      - `errorTitle`, `errorUnknown`, `errorOpening`, `errorReading`, `errorWriting`, `errorCloseButton`
    - Settings
      - `settingsViewportDegrees`

- Version 0.2.3
  - Added:
    - Login Screen
      - `loginErrorProxyAuthPending`, `loginErrorProxyAuthFailed`, `loginProxyAutodetect`, `loginProxyTest`, `loginSupportIssueTracker`, `loginSupportDirectEmail`, `loginTranslationBy`
    - Control Settings
      - `settingsControlsForward`
    - Proxy Modal
      - `modalProxyInfo`, `modalProxyBadCreds`, `modalProxyUsername`, `modalProxyPassword`, `modalProxyAuthContinueButton`, `modalProxyAuthCancelButton`
    - Scene Explorer
      - `scenePOIRemovePoint`, `scenePOILabelImageType`, `scenePOILabelImageTypeStandard`, `scenePOILabelImageTypePanorama`, `scenePOILabelImageTypePhotosphere`, `sceneAddMenu`, `sceneAddAOI`, `sceneAddLine`, `scenePOIAreaDefaultName`, `scenePOILineDefaultName`, `sceneExplorerAddLine`, `scenePOICancelFlyThrough`, `scenePOIPerformFlyThrough`, `scenePOIBookmarkMode`, `scenePOISetBookmarkCamera`, `scenePOICameraPosition`, `scenePOICameraRotation`
    - Viewport
      - `settingsViewportViewDistance`
    - Convert
      - `convertGeneratingPreview`, `convertAddPreviewToScene`, `convertPreviewName`, `convertSpaceLabel`, `convertSpaceCartesian`, `convertSpaceLatLong`, `convertSpaceLongLat`
  - Removed:
    - Titles
      - `loginTitle`
    - Scene Explorer
      - `sceneExplorerAddLines`
    - Viewport
      - `settingsViewportFarPlane`, `settingsViewportNearPlane`
    - Menu strings
      - `menuImportCSV`
      - `menuImportCSVTitle`
- Version 0.2.2
  - Added:
    - Appearance Setting
      - `settingsAppearancePOIDistance`
    - Convert Status codes:
      - `convertJobs`, `convertAwaiting`, `convertQueued`, `convertAwaitingLicense`, `convertRunning`, `convertCompleted`, `convertCancelled`, `convertFailed`
  - Removed:
    - Unused convert strings:
      - `convertOverride`, `convertEstimate`, `convertRead`, `convertPoints`

## Getting Started with a New Translation

Translation files live in `/assets/lang/` and are named using the ISO standards [ISO 639-1](https://wikipedia.org/wiki/ISO_639-1) and [ISO 3166-1 alpha-2](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2). The Australian English translation is therefore `en-AU.json`.

It is recommended to start a new translation by copying the `en-AU.json` file as this is the official translation.

The top of the file needs info on the translation:
- `langkey`: This is the ISO 639-1 key for the language (or `null` if not applicable)
- `countrykey`: This is the ISO 3166-1 key for the country (or `null` if not applicable)
- `localname`: Is the name of the translation in the language (so users of that language can see it in the list)
- `englishname`: Is the name of the translation in English so the developers at Euclideon know what it is.
- `translationby`: This is your name so you can be credited for your work.
- `contactemail`: This is your email so you can be credited for your work.
- `targetVersion`: Which version of Euclideon Client that this translation targets. If the version number doens't match the user will be alerted that the translation may be incomplete

The `languages.json` file contains the list of languages that Euclideon Vault Client will offer to the end user. This is checked after boot (not necessarily immediately at startup) to include new language packs. This also gets cached in the users settings directory.

Some strings support optional parameters that will be generated by the application. These are accessed using `{n}` where n is the index for the parameter starting from 0. The string that concatenates Latitude, Longitude and Altitude, `gisInfoLatLongAlt` has 3 parameters, `{0}` for latitude, `{1}` for longitude and `{2}` for altitude so the localized string is `Lat: {0}, Lon: {1}, Alt {2}`. There are no restrictions on the order of these parameters in the localized string, nor a restriction on the count. `{1}, {0}, {1}` were the parameters are `[Apple, Orange]` would output `Orange, Apple, Orange`.

The json file should be saved in UTF-8 with no byte order mark. Code page 65001 is recommended.

### Submiting back to Euclideon for inclusion
Email support@euclideon.com to get your language included in the official bundle.

## String List

Strings are camelCase, with the module they are from and then the specific string key. For example, if the convert module has an error message that is displayed when an unsupported filename is found, this would be `convertErrorUnsupported`

Modules are currently:
- [Login](#login-strings)
- [Settings](#settings-strings)
- [Convert](#convert-strings)
- [Menu](#menu-strings)
- [Scene](#scene-strings)
  - [Camera](#camera)
  - [Scene Explorer](#scene-explorer)
  - [Point of Interest](#point-of-interest)
  - [Live Feeds](#live-feeds)
- [Error](#error-strings)

### Login Strings

- `loginButton`: Used on the button that logs in the user
- `loginRestoreDefaults`: Used on the button that restores most vault client settings to their default values
- `loginAbout`: Used on the button that opens the About window from the login screen
- `loginReleaseNotes`: Used on the button that opens the Release Notes window from the login screen
- `loginServerURL`: Vault Server address
- `loginRememberServer`: Used on the button that remembers the server address
- `loginUsername`: Label for the username field
- `loginRememberUser`: Used on the button that remembers the username
- `loginShowPassword`: Label for the button that toggles showing the user's password
- `loginPassword`: Label for the password field
- `loginCapsWarning`: Warning displayed when caps lock is enabled while logging in
- `loginAdvancedSettings`: Subheading for expandable section containing advanced settings
- `loginProxyAddress`: Label for the proxy address field
- `loginProxyAutodetect`: Label for the checkbox that attempts to find a proxy automatically
- `loginProxyTest`: Used on the button that tests the proxy details are correct
- `loginUserAgent`: Used on the label for the 'fake' user agent provided to servers when communicating
- `loginSelectUserAgent`: Label on the drop down for embedded fake user agents
- `loginSupportIssueTracker`: Displayed on the login page to direct the user to the Euclideon public issue tracker (is a hyperlink to the webpage)
  - {0} is the issue tracker web address
  - {1} is the issue tracker host name
- `loginSupportDirectEmail`: Displayed on the login page to direct the user to Euclideon support (opens an email to support when clicked)
  - {0} is the support email address
- `loginTranslationBy`: Displays credits for the translation on the login page
  - {0} is the language `localname` provided in `_LanguageInfo`
  - {1} is the `translationby` provided in `_LanguageInfo`
  - {2} is the `contactemail` provided in `_LanguageInfo`
- `loginIgnoreCert`: Label for a setting that disables many of the security checks when logging in
- `loginIgnoreCertWarning`: This displays a warning to the user that disabling the security certificates is _very_ dangerous
- `loginPending`: String used internally to set the login status to "pending"?
- `loginMessageCredentials`: Prompts the user to enter their login information
- `loginEnterURL`: Prompts the user to enter a server URL
- `loginMessageChecking`: Tells the user to wait while their login information is confirmed
- `loginErrorConnection`: Displays when the vault client fails to connect to the specified vault server
- `loginErrorAuth`: Displays when the server rejects the specified username or password
- `loginErrorTimeSync`: Displays when the local clock and server clock are not synchronised
- `loginErrorSecurity`: Displays when the server fails a security check during a login attempt
- `loginErrorNegotiate`: Displays when the server entered responds but not in a way expected of a vault server
- `loginErrorProxy`: Displays when the user was unable to login due to a proxy interfering
- `loginErrorProxyAuthPending`: Displays in the background when the proxy credential modal is visible
- `loginErrorProxyAuthFailed`: Displays if the user cancels the proxy modal without submitting valid details
- `loginErrorOther`: Displays when an unknown error occurs during login, tells the user to try again

### Settings Strings

- `settingsTitle`: Title of the Settings tab

#### Appearance
- `settingsAppearance`: Title of the Appearance subheading in Settings
- `settingsAppearanceRestoreDefaults`: Used to restore default values for all appearance settings
- `settingsAppearanceTheme`: Label next to dropdown list to select appearance theme
- `settingsAppearanceDark`: Dark option for appearance theme and login screen dropdown lists
- `settingsAppearanceLight`: Light option for appearance theme and login screen  dropdown lists
- `settingsAppearancePOIDistance`: Label next to slider for setting the maximum distance at which POIs are visible
- `settingsAppearanceShowDiagnostics`: Label next to the checkbox that toggles the display of diagnostic information in the menu bar
- `settingsAppearanceShowEuclideonLogo`: Label next to the checkbox that toggles the display of the Euclideon logo over the scene
- `settingsAppearanceAdvancedGIS`: Label next to the checkbox that toggles the display of advanced GIS settings in the viewport
- `settingsAppearanceLimitFPS`: Label next to checkbox for limiting the frame-rate while vault client window is inactive or unselected
- `settingsAppearanceShowCompass`: Label next to checkbox that toggles the appearance of a compass in the viewport
- `settingsAppearanceLoginRenderLicense`: Label next to checkbox that toggles acquiring a render license automatically on login
- `settingsAppearanceShowNativeDialogs`: Label next to checkbox that toggles displaying native dialogs
- `settingsAppearanceShowSkybox`: Label next to checkbox that toggles if the skybox is displayed
- `settingsAppearanceSkyboxColour`: Label next to colour picker to decide the colour of the background if the skybox is disabled
- `settingsAppearanceSaturation`: Slider that applies saturation to the scene
- `settingsAppearanceMouseAnchor`: Label next to dropdown list to select the appearance of the mouse cursor when hovering over objects in the scene
- `settingsAppearanceNone`: "None" option for mouse cursor appearance. If selected, mouse cursor retains its default appearance
- `settingsAppearanceCompass`: Compass option for mouse cursor appearance
- `settingsAppearanceOrbit`: Orbit option for mouse cursor appearance
- `settingsAppearanceVoxelShape`: Label next to dropdown list to select the shape of the voxels that make up the models in the scene
- `settingsAppearanceRectangles`: Rectangles option for voxel shape dropdown list
- `settingsAppearanceCubes`: Cubes option for voxel shape dropdown list
- `settingsAppearancePoints`: Points option for voxel shape dropdown list
- `settingsAppearanceWindowLayout`: Label next to combo list to select the layout
- `settingsAppearanceWindowLayoutScSx`: Combo option for when the user wants the Scene on left and scene explorer on right
- `settingsAppearanceWindowLayoutSxSc`: Combo option for when the user wants the Scene explorer on left and scene on right
- `settingsAppearancePresentationUI`: Label for dropdown list to select whether the user interface (UI) is visible in presentation/fullscreen mode
- `settingsAppearanceHide`: Hide option for presentation UI dropdown list, in fullscreen mode the UI will not be visible
- `settingsAppearanceShow`: Show option for presentation UI dropdown list, in fullscreen mode the UI will be visible
- `settingsAppearanceResponsive`: Responsive option for presentation UI dropdown list, in fullscreen mode the UI will only be visible if the user does something
- `settingsAppearanceTranslationDifferentVersion`: Is an error tooltip that is displayed when the selected translation is for a different version of the software (may indicate that some strings may not be available)

#### Input & Controls
- `settingsControls`: Title of the Input & Controls subheading in Settings
- `settingsControlsRestoreDefaults`: Used to restore default values for all controls settings
- `settingsControlsOSC`: Label next to checkbox that toggles the appearance of on-screen camera controls in the viewport
- `settingsControlsTouchUI`: Label next to checkbox that toggles UI optimisation for a touchscreen
- `settingsControlsMouseInvertX`: Label next to checkbox that toggles the positive/negative X-axis for mouse inputs
- `settingsControlsMouseInvertY`: Label next to checkbox that toggles the positive/negative Y-axis for mouse inputs
- `settingsControlsControllerInvertX`: Label next to checkbox that toggles the positive/negative X-axis for controller inputs
- `settingsControlsControllerInvertY`: Label next to checkbox that toggles the positive/negative Y-axis for controller inputs
- `settingsControlsMousePivot`: Subheading for the four dropdown lists used to assign bindings for mouse input
- `settingsControlsForward`: Option for Left, Middle and Right mouse button dropdown lists, moves the camera forwards (same as W key) and also allows tumbling
- `settingsControlsTumble`: Option for Left, Middle and Right mouse button dropdown lists, rotates the camera without moving it
- `settingsControlsOrbit`: Option for Left, Middle and Right mouse button dropdown lists, moves the camera while maintaining distance to the selected point
- `settingsControlsPan`: Option for Left, Middle and Right mouse button dropdown lists, moves the camera perpendicular to its forward direction
- `settingsControlsDolly`: Option for scrollwheel dropdown list, zooms camera in and out
- `settingsControlsChangeMoveSpeed`: Option for scrollwheel dropdown list, changes the camera movement speed
- `settingsControlsLeft`: Label next to dropdown list for selecting what the left mouse button does
- `settingsControlsMiddle`: Label next to dropdown list for selecting what the middle mouse button does
- `settingsControlsRight`: Label next to dropdown list for selecting what the right mouse button does
- `settingsControlsScrollWheel`: Label next to dropdown list for selecting what the scrollwheel does

#### Viewport
- `settingsViewport`: Title of the Viewport subheading in Settings
- `settingsViewportRestoreDefaults`: Used to restore default values for all viewport settings
- `settingsViewportViewDistance`: Label next to slider for setting the viewable distance
- `settingsViewportCameraLens`: Label next to dropdown list for selecting the camera's field of view
- `settingsViewportCameraLensCustom`: Label for the custom lens option,
- `settingsViewportCameraLens15mm`: Label for the 15mm lens option,
- `settingsViewportCameraLens24mm`: Label for the 24mm lens option,
- `settingsViewportCameraLens30mm`: Label for the 30mm lens option,
- `settingsViewportCameraLens50mm`: Label for the 50mm lens option,
- `settingsViewportCameraLens70mm`: Label for the 70mm lens option,
- `settingsViewportCameraLens100mm`: Label for the 100mm lens option,
- `settingsViewportFOV`: Label next to slider for setting a custom field of view
- `settingsMaps`: Title of the Maps & Elevation subheading in Settings
- `settingsMapsRestoreDefaults`: Used to restore default values for all maps settings
- `settingsMapsMapTiles`: Label next to checkbox for toggling the visibility of map tiles in the scene
- `settingsMapsMouseLock`: Label next to checkbox for toggling whether or not the user can click on map tiles within the scene
- `settingsMapsTileServerTitle`: Title used on tile server window
- `settingsMapsTileServerButton`: Label used on button for opening the tile server window
- `settingsMapsTileServer`: Label used next to textbox where user can enter the address of the tile server they wish to use
- `settingsMapsTileServerLoading`: Message displayed in tile server window while vault client tries contacting the specified server
- `settingsMapsTileServerErrorFetching`: Message displayed in tile server window when vault client fails to contact the specified server
- `settingsMapsTileServerImageFormat`: Label next to dropdown list for selecting the file format of images obtained from the tile server
- `settingsMapsTileServerCloseButton`: Label used on button for closing tile server window
- `settingsMapsMapHeight`: Label next to slider for setting the height of map tiles in the scene
- `settingsMapsHybrid`: Option in map blending dropdown list, will display map tiles only over scene objects which are below the specified map height
- `settingsMapsOverlay`: Option in map blending dropdown list, will display map tiles over all scene objects
- `settingsMapsUnderlay`: Option in map blending dropdown list, will display all scene objects over map tiles
- `settingsMapsBlending`: Label next to dropdown list for selecting the style of map tile blending in the scene
- `settingsMapsOpacity`: Label next to slider for setting the opacity of map tiles (opacity is the opposite of transparency)
- `settingsMapsSetHeight`: Label used on button that sets the height of map tiles to the current camera height

#### Visualization
- `settingsVis`: Title of the Visualisation subheading in Settings
- `settingsVisObjectHighlight`: Label next to checkbox for enabling highlighting selected objects
- `settingsVisHighlightColour`: Label next to the colour picker for highlighting
- `settingsVisHighlightThickness`: Label next to slider to select highlight edge thickness
- `settingsVisDisplayMode`: Label next to dropdown list for selecting how the scene is displayed
- `settingsVisModeClassification`: Option in display mode dropdown list where the colour of scene objects is determined by the objects classification
- `settingsVisModeColour`: Option in display mode dropdown list where scene objects are shown in their true colour
- `settingsVisModeDisplacement`: Option in display mode dropdown list where scene objects are shown with colour to indicate displacement range
- `settingsVisModeIntensity`: ?
- `settingsVisMinIntensity`: ?
- `settingsVisMaxIntensity`: ?
- `settingsVisEdge`: Label next to checkbox for enabling voxel outlines
- `settingsVisEdgeWidth`: Label next to slider for setting the width of voxel outlines
- `settingsVisEdgeThreshold`: Label next to slider for setting the size limit for edges to consist of multiple voxels
- `settingsVisEdgeColour`: Label next to colour selector for setting the colour of voxel outlines
- `settingsVisHeight`: Label next to checkbox for enabling colouration of scene objects based on their height
- `settingsVisHeightStartColour`: Label next to colour selector for setting the colour of objects towards the minimum height
- `settingsVisHeightEndColour`: Label next to colour selector for setting the colour of objects towards the maximum height
- `settingsVisHeightStart`: Label next to slider for setting the minimum height for colouration
- `settingsVisHeightEnd`: Label next to slider for setting the maximum height for colouration
- `settingsVisDepth`: Label next to checkbox for enabling colouration of scene objects based on their distance from the camera (camera depth)
- `settingsVisDepthColour`: Label next to colour selector for setting the colour of objects towards the maximum depth
- `settingsVisDepthStart`: Label next to slider for setting the minimum distance for colouration
- `settingsVisDepthEnd`: Label next to slider for setting the maximum distance for colouration
- `settingsVisContours`: Label next to checkbox for enabling an overlay of contours indicating the height of scene objects
- `settingsVisContoursColour`: Label next to colour selector for setting the colour of contours
- `settingsVisContoursDistances`: Label next to slider for setting the vertical distance between each contour
- `settingsVisContoursBandHeight`: Label next to slider for setting the vertical size of each contour
- `settingsVisContoursRainbowRepeatRate`: Label next to the slider for how quickly the rainbow mode for contours repeats
- `settingsVisContoursRainbowIntensity`: Label next to the slider that sets how strongly the rainbow appears
- `settingsVisDisplacementRange`: Label next to the input fields to specify displacement range values
- `settingsVisRestoreDefaults`: Used to restore default values for all visualisation settings

- `settingsVisClassRestoreDefaults`: Label on a button that restores default values for all classifications and colours
- `settingsVisClassShowColourTable`: Label next to checkbox that opens table for defining colours for each classification
- `settingsVisClassRename`: Label for rename button beside each user definable classification
- `settingsVisClassSet`: Label for button that appears beside a textbox when renaming a user definable classification, used to confirm the new name

##### Classifications
- `settingsVisClassNeverClassified`: Label for classification #0
- `settingsVisClassUnclassified`: Label for classification #1
- `settingsVisClassGround`: Label for classification #2
- `settingsVisClassLowVegetation`: Label for classification #3
- `settingsVisClassMediumVegetation`: Label for classification #4
- `settingsVisClassHighVegetation`: Label for classification #5
- `settingsVisClassBuilding`: Label for classification #6
- `settingsVisClassLowPoint`: Label for classification #7
- `settingsVisClassKeyPoint`: Label for classification #8
- `settingsVisClassWater`: Label for classification #9
- `settingsVisClassRail`: Label for classification #10
- `settingsVisClassRoadSurface`: Label for classification #11
- `settingsVisClassReserved`: Label for classification #12
- `settingsVisClassWireGuard`: Label for classification #13
- `settingsVisClassWireConductor`: Label for classification #14
- `settingsVisClassTransmissionTower`: Label for classification #15
- `settingsVisClassWireStructureConnector`: Label for classification #16
- `settingsVisClassBridgeDeck`: Label for classification #17
- `settingsVisClassHighNoise`: Label for classification #18
- `settingsVisClassReservedColours`: Title for expandable subheading containing classifications #19 to #63
- `settingsVisClassReservedLabels`: Label used for classifications #19 to #63
- `settingsVisClassUserDefinable`: Title for expandable subheading containing classifications #64 to #255
- `settingsVisClassUserDefined`: Default label used for unnamed classifications #64 to #255

### Convert Default Settings Strings

- `settingsConvert`: Label used on the section heading for convert default settings
- `settingsConvertRestoreDefaults`: Label used on the context menu to restore convert settings from the default settings file
- `convertChangeDefaultWatermark`: Label used for button that opens the "change default watermark" window

### Convert Strings

- `convertTitle`: Title of the convert tab
- `convertReadingFile`: Progress message displayed when reading a file during a convert job
  - {0} the current file being read
  - {1} the total number of files
- `convertWritingPoints`: Progress message displayed when writing to the new file during a convert job
  - {0} the current number of points written
  - {1} the total points expected to be read
- `convertAddToScene`: Label for button that appears following a successful convert job that adds the new object to the scene
- `convertSettings`: Subheading for section where user can customise settings for the convert job
- `convertOutputName`: Label next to text box where user specifies the path and name of the output file generation from the convert job
- `convertTempDirectory`: Label next to text box where user specifies the path of the folder used to store temporary files during the convert job
- `convertContinueOnCorrupt`: Label next to checkbox to set whether or not convert job should try to continue upon encountering an error
- `convertPointResolution`: Label showing the current point resolution of the convert job
- `convertInputDataRanges`: Label showing the min and max point resolution values of the convert job
- `convertOverrideResolution`: Label next to checkbox that is used to set a custom point resolution
- `convertOverrideGeolocation`: Label next to checkboxes that are used to set a custom point resolution or SRID
- `convertQuickTest`: label on checkbox to select quick/partial convert mode that assists with testing the settings are correct
- `convertGlobalOffset` label beside the XYZ override position
- `convertMetadata`: Subheading for section where user can define metadata for the file that results from the convert job
- `convertNoWatermark`: This message displays when no watermark image has been chosen for the current convert job
- `convertLicense`: Label next to text box where user can enter license information to be assigned to the output file
- `convertCopyright`: Label next to text box where user can enter copyright information to be assigned to the output file
- `convertComment`: Label next to text box where user can set the value of the comment field in the output file
- `convertAuthor`: Label next to text box where user can set the value of the author field in the output file
- `convertReset`: Label used for button that resets a completed (or cancelled?) convert job
- `convertBeginConvert`: Label used for button that commences the currently selected convert job(s)
- `convertRestart`: Label used for button that commences the previously cancelled or failed convert job(s)
- `convertBeginPendingFiles`: Label used for convert button when files are still being processed
  - {0} is how many files are left to be processed
- `convertPendingEstimate`: label used when the item is pending and has estimated point count
  - {0} is points estimated
  - {1} is `0` (no points read because its pending)
- `convertPendingNoEstimate`: label used when the point estimated isn't available
  - {0} is `-1` (cannot guess number of points)
  - {1} is `0` (no points read because its pending)
- `convertPendingProcessInProgress`: label used when the added file is currently being preprocessed
- `convertPendingProcessing`: label used when the added file has yet to be preprocessed
- `convertReadingEstimate`: label used when reading and there is an estimated number of points
  - {0} is points estimated
  - {1} is number of points read
- `convertReadingNoEstimate`: label used when reading points and there is no estimated number of points
  - {0} is `-1` (cannot guess number of points)
  - {1} is number of points read
- `convertReadComplete`: label used when reading points has completed
  - {0} is points total
  - {1} is points read
- `convertLoadWatermark`: Label used for button that opens the "load watermark" window
- `convertRemoveWatermark`: Label used for button that appears once a watermark has been loaded and is used to remove the loaded watermark
- `convertRemoveAll`: Label used for button that removes all queued convert jobs
- `convertInputFiles`: Subheading for section showing all currently queued convert jobs, will not appear if no jobs have been added
  - {0} is the number of files
- `convertSRID`: Label showing the current SRID of the convert job
- `convertPathURL`: Label next to text box in Load Watermark window containing the path of the watermark image file
- `convertLoadButton`: Label used for button in Load Watermark window that loads the currently selected file and closes the window
- `convertCancelButton`: Label used for button beside in Load Watermark window that closes the window
- `convertFileExistsTitle`: Title of the window that appears when the specified convert output file already exists
- `convertFileExistsMessage`: Message displayed in the window that appears when the specified convert output file already exists, {0} is the full path and name of the file
- `convertJobs`: Label used for the list of conversion jobs at the top of the convert page
- `convertAwaiting`: Message displayed when no conversion jobs have been added
- `convertQueued`: Message displayed next to a job that is queued to start but has not started yet
- `convertAwaitingLicense`: Message displayed next to a job that is waiting for a convert license and has not started yet
- `convertRunning`: Message displayed next to a job that is currently being converted
- `convertCompleted`: Message displayed next to a job that has finished being converted
- `convertCancelled`: Message displayed next to a job that was cancelled by the user
- `convertWriteFailed`: Message displayed next to a job that failed due to being unable to write a file to disk or create a directory
- `convertParseError`: Message displayed next to a job that failed due to an issue interpreting the input file
- `convertImageParseError`: Message displayed next to a job that failed due to an error processing an image file
- `convertFailed`: Message displayed next to a job that failed to finish converting for a reason not listed above
- `convertNoFile`: Message displayed to indicate that no file has been selected for conversion
- `convertGeneratingPreview`: Displayed on the button while a preview is being generated
- `convertAddPreviewToScene`: Displayed on the button that generates a preview model and adds it to the scene
- `convertPreviewName`: The default name for a preview pointcloud when added to the scene
- `convertAllSpaceLabel`: The label beside the dropdown to select the projection space for all inputs
- `convertSpaceLabel`: The label beside the dropdown to select the projection space for the input
- `convertSpaceDetected`: The label on the combobox item to reset the projection space to the originally detected value
- `convertSpaceCartesian`: The label on the combobox item for a cartesian projected conversion input
- `convertSpaceLatLong`: The label on the combobox item for a latitude, longitude ordered conversion input
- `convertSpaceLongLat`: The label on the combobox item for a longitude, latitude ordered conversion input
- `convertSpaceECEF`: The label on the combobox item for earth centered, earth fixed conversion inputs
- `convertAddFileTitle`: Title of the file dialog used for adding files to the convert queue
- `convertSetOutputTitle`: Title of the file dialog used for setting the convert output path
- `convertSetTempDirectoryTitle`: Title of the file dialog used for setting the convert temporary directory
- `convertAddNewJob`: Label for button used in convert window to create a blank new job in the convert job list
- `convertAddFile`: Label for button used in convert window to open a file dialog and add a file to be converted
- `convertSetOutput`: Label for button used in convert window to open a file dialog and set the destination filepath for the converted file output.
- `convertWatermark`: Label when displaying watermark or watermark path
- `convertNoJobSelected`: Label when no job is selected and one will need to be created or selected

### Menu Strings

- `menuSystem`: Header for System menu
- `menuQuit`: Option in System menu, exits the program
- `menuLanguage`: Submenu heading in System Menu with language options
- `menuExperimentalFeatures`: Submenu in System Menu for experimental features (not that experimental feature names are not localised as they are guaranteed to change in future versions)
- `menuWindows`: Header for Windows menu
- `menuScene`: Option in Windows menu to enable the Scene tab
- `menuConvert`: Option in Windows menu to enable the Convert tab
- `menuSceneExplorer`: Option in Windows menu to enable the Scene Explorer tab
- `menuSettings`: Option in Windows menu to enable the Settings tab
- `menuProjects`: Header for Projects menu
- `menuNewScene`: Option in Projects menu to remove all objects in the current scene and creating a new, empty scene
- `menuBindings`: Submenu in System Menu for the hotkey binding interface
- `menuProjectExport`: Menu button to open the export project modal
- `menuProjectExportTitle`: Title on the modal when exporting project files
- `menuProjectImport`: Menu button to open the import project modal
- `menuProjectImportTitle`: Title on modal when importing a project file (UDP or JSON files)
- `menuProjectNone`: Label displayed when no server projects are available
- `menuLogout`: Logout option in System menu, logs the user out and opens the logout window
- `menuLogoutTitle`: Title of logout window
- `menuLogoutMessage`: Message displayed in logout window, informs the user that they have been logged out
- `menuLogoutCloseButton`: Label for button that closes the logout window
- `menuProfileTitle`: Title of the profile modal
- `menuUserNameLabel`: Label of user name in the profile modal
- `menuRealNameLabel`: Label of real name in the profile modal
- `menuEmailLabel`: Label of email in the profile modal
- `menuReleaseNotes`: Option in System menu, opens the Release Notes window
- `menuReleaseNotesTitle`: Title of Release Notes window
- `menuReleaseNotesFail`: Displayed when client fails to load text from the Release Notes file
- `menuReleaseNotesShort`: String used internally as ID for displaying the Release Notes window?
- `menuReleaseNotesCloseButton`: Label used on the close button of the Release Notes window
- `menuAbout`: Option in System menu, opens the About window
- `menuAboutTitle`: Title for the About window
- `menuAboutVersion`: Used in the About window to show the current version of vault client
- `menuAboutLicenseInfo`: Used in the About window to indicate that it contains third party license information
- `menuAboutCloseButton`: Label used on the close button of the About window
- `menuAboutPackageUpdate`: Used to show information about the most recent package update?
- `menuImport`: Option in Projects menu, opens the Import sub-menu
- `menuImportUDP`: Option in Import sub-menu, opens the Import UDP Window
- `menuImportUDPTitle`: Title of the Import UDP Window
- `menuCurrentVersion`: Used to display the current version of vault client in the Release Notes and "New Version Available" windows
- `menuNewVersionAvailableTitle`: Title of "New Version Available" window
- `menuNewVersion`: Used to display the new version of vault client that is available
- `menuNewVersionDownloadPrompt`: Tells the user how to download the new version of vault client
- `menuNewVersionCloseButton`: Label used for the close button of the "New Version Available" window
- `menuBarUpdateAvailable`: Message shown in the menu bar when there is a new version of vault client available
- `menuBarFilesQueued`: Used to show in the menu bar how many files have been queued for importing into the scene
  - {0} = number of files queued
- `menuBarFilesFailed`: Used to show in the menu bar how many of the queued files failed to be processed
  - {0} = number of files that failed
- `menuBarFilesFailedTooltip` Tooltip that appears to let the user know that they can click to open the failed file list
- `menuBarConvert`: Used to display the status of the convert license
- `menuBarLicense`: Used to display that the menu bar is showing the status of the convert or render licenses
- `menuBarRender`: Used to display the status of the render license
- `menuBarFPS`: Displays vault client's current framerate in both frames per second (fps) and milliseconds (ms)
  - {0} = framerate in frames/s
  - {1} = framerate in ms/frame
- `menuBarSecondsAbbreviation`: Abbreviation used to display the number of seconds remaining before license expiration
- `menuBarLicenseExpired`: Used to indicate when the render or convert license has expired
- `menuBarLicenseQueued`: Used to indicate the position of a file being imported in the import queue?
- `menuBarWaitingForRenderLicense`: Message to let the user know the system is waiting for a render license
- `menuBarInactive`: Used to indicate that vault client is not the currently selected window and is running in the background

### Proxy Login Modal

- `modalProxyInfo`: Used at the top of the proxy login modal to let the user know why they are logging in
- `modalProxyBadCreds`: Message displayed at the top of the proxy modal after the supplied username and password fail to authenticate with the proxy
- `modalProxyUsername`: Label for the text input for the proxy username
- `modalProxyPassword`: Label for the text input for the proxy password
- `modalProxyAuthContinueButton`: Button used for continuing with the supplied username and password
- `modalProxyAuthCancelButton`: Button used to cancel the proxy authentication modal without submitting details

### Scene Strings

- `sceneTitle`: Title of the Scene tab
- `sceneImageViewerTitle`: Title of the Image Viewer window
- `sceneImageViewerCloseButton`: Label used for the close button of the Image Viewer window
- `sceneGeographicInfo`: Used to display the geographic information in the corner of the viewport
- `sceneNotGeolocated`: Displays when the current location of the camera is not in a known geographic area
- `sceneUnsupportedSRID`: Displays when the current location has an SRID that is invalid
- `sceneMousePointInfo`: Used to display the position of the mouse in the corner of the viewport, in global space coordinates, when in a geolocated zone
- `sceneMousePointWGS`: Used to display the position of the mouse in the corner of the viewport, in WGS84 coordinates, when in a geolocated zone
- `sceneSRID`: Used to display the SRID code of the geolocated position of the camera
- `sceneOverrideSRID`: Label used for the interface shown when Advanced GIS Settings is enabled in Appearance Settings, allows the user to change the current SRID
- `sceneMapCopyright`: String used internally as the ID for the map copyright data window?
- `sceneMapData`: Message displayed in the map copyright data window in the corner of the viewport, only shows when the camera is in a valid SRID zone
- `sceneSetMapHeight`: Menu option when right-clicking within the scene, sets map height to the current mouse location's height
- `sceneAddPOI`: Menu option when right-clicking within the scene, creates a POI at the current mouse location
- `sceneMoveTo`: Menu option when right-clicking within the scene, moves the camera to the current mouse location
- `sceneAddMenu`: Sub-menu menu option when right-clicking within the scene, contains AddPOI, AddAOI and AddLine
- `sceneAddAOI`: Menu option when right-clicking within the scene, creates an AOI at the current mouse location
- `sceneAddLine`: Menu option when right-clicking within the scene, creates a Line at the current mouse location
- `sceneAddViewShed`: Menu option when right-clicking within the scene, creates a View Shed at the current mouse location
- `sceneAddFilter`: Context menu option when right clicking in the scene to open another submenu with filter types
- `sceneAddFilterBox`: Context menu option when right clicking in the scene to add a new Box filter
- `sceneAddFilterSphere`: Context menu option when right clicking in the scene to add a new Sphere filter
- `sceneAddFilterCylinder`: Context menu option when right clicking in the scene to add a new Cylinder filter
- `sceneResetRotation`: Menu option when right-clicking within the scene, rotates the camera to default orientation

- `sceneCameraAttachmentWarning` Warning text at the top of the window when the camera is attached to a scene item
  - {0} the name of the scene item the camera is attached to
- `sceneCameraAttachmentDetach` Button at the top of the screen to detach the camera when it is attached to a scene item

- `sceneLockAltitude`: Tooltip displayed when mouse is hovered over the Lock Altitude button in the viewport controls window
- `sceneLockAltitudeKey`: Used in Lock Altitude tooltip, shown in brackets
- `sceneCameraInfo`: Tooltip displayed when mouse is hovered over the Show Camera Information button in the viewport controls window
- `sceneProjectionInfo`: Tooltip displayed when mouse is hovered over the Show Projection Information button in the viewport controls window
- `sceneGizmoTranslate`: Tooltip displayed when mouse is hovered over the Gizmo Translate button in the viewport controls window
- `sceneGizmoTranslateKey`: Used in Gizmo Translate tooltip, shown in brackets
- `sceneGizmoRotate`: Tooltip displayed when mouse is hovered over the Gizmo Rotate button in the viewport controls window
- `sceneGizmoRotateKey`: Used in Gizmo Rotate tooltip, shown in brackets
- `sceneGizmoScale`: Tooltip displayed when mouse is hovered over the Gizmo Scale button in the viewport controls window
- `sceneGizmoScaleKey`: Used in Gizmo Scale tooltip, shown in brackets
- `sceneGizmoLocalSpace`: Tooltip displayed when mouse is hovered over the Gizmo Local Space button in the viewport controls window
- `sceneGizmoLocalSpaceKey`: Used in Gizmo Local Space tooltip, shown in brackets
- `sceneFullscreen`: Tooltip displayed when mouse is hovered over the Fullscreen button in the viewport controls window
- `sceneFullscreenKey`: Used in Fullscreen button tooltip, shown in brackets
- `sceneMapMode`: Tooltip displayed when mouse is hovered over the Map Mode button in the viewport controls window
- `sceneMapModeKey`: Used in Map Mode button tooltip, shown in brackets

#### Camera
- `sceneCameraSettings`: String used internally as ID for the viewport controls/camera information window?
- `sceneCameraLatLongAlt`: Used to display the latitude, longitude and altitude information where applicable
  - {0} = Latitude (in degrees)
  - {1} = Longitude (in degrees)
  - {2} = Altitude (in SRID space)
- `sceneCameraPosition`: Is used in the camera info box so the user can change the position of the camera (in the current SRID)
- `sceneCameraRotation`: Is used in the camera info box so the user can change the orientation of the camera
- `sceneCameraMoveSpeed`: Is used on the slider in the camera info box to let the user change the speed of the camera
- `sceneCameraOutOfBounds`: Is displayed in the camera info box when the camera is outside of the requested geozone. This is intended to alert the user that the space is distorted
- `sceneCameraOSCMove`: This text is displayed on the forward, back and strage button of the on screen controls (note that this setting is deprecated to be removed)
- `sceneCameraOSCUpDown`: This is the text in the up/down section of the on screen controls (note that this setting is deprecated to be removed)

#### Scene Explorer
- `sceneExplorerTitle`: Title of the Scene Explorer tab
- `sceneExplorerRemove`: Tooltip displayed when mouse is hovered over the Remove button in the scene explorer tab
- `sceneExplorerRemoveKey`: Used in Remove button tooltip, shown in brackets
- `sceneExplorerRemoveItem`: Menu option when right-clicking on an item in the Scene Explorer, removes the clicked item
- `sceneExplorerAddUDS`: Tooltip displayed when mouse is hovered over the Add UDS button in the scene explorer tab
- `sceneExplorerAddUDSKey`: Used in Add UDS button tooltip, shown in brackets
- `sceneExplorerAddUDSTitle`: Title of the Add UDS window
- `sceneExplorerAddOther`: Tooltip displayed when mouse is hovered over the Add Other button in the scene explorer tab
- `sceneExplorerAddFeed`: Menu option in the Add Other menu which is displayed when the Add Other button is clicked, adds a Live Feed item to the scene
- `sceneExplorerAddFolder`: Tooltip displayed when mouse is hovered over the Add Folder button in the scene explorer tab
- `sceneExplorerViewShedDefaultName`: Default name of new View Sheds when they are added to the scene
- `sceneExplorerFilterBoxDefaultName`: The default name when a new box filter is added
- `sceneExplorerFilterSphereDefaultName`: The default name when a new sphere filter is added
- `sceneExplorerFilterCylinderDefaultName`: The default name when a new cylinder filter is added
- `sceneExplorerEditName`: Menu option when right-clicking on an item in the Scene Explorer, allows user to change the name of the item
- `sceneExplorerUseProjection`: Menu option when right-clicking on an item in the Scene Explorer, changes the current SRID to that of the selected scene item
- `sceneExplorerMoveTo`: Menu option when right-clicking on an item in the Scene Explorer, moves the camera to the location of the selected item
- `sceneExplorerResetPosition`: Menu option when right-clicking on an item in the Scene Explorer that has an original geolocation, will return the object to that position if it has been moved
- `sceneExplorerCompareModels`: Menu option when right-clicking on an item in the Scene Explorer that can be compared with another item, a model with displacement values will be generated on disk.
- `sceneExplorerPathURL`: Label next to the text box containing the path of the currently selected file
- `sceneExplorerLoadButton`: Label used for the Load button in the Add UDS and Import Project windows which loads the currently selected file and closes the window
- `sceneExplorerExportButton`: Label used for the Export button in the Export Project window which saves the content of the current scene to the specified location and closes the window
- `sceneExplorerSetButton`: Label used for the Set button in the Convert Output and Temporary Directory selection window
- `sceneExplorerCancelButton`: Label used for the Close button of the Add UDS window
- `sceneExplorerUnknownCustomNode`: Label used when unsupported nodes are loaded from a project

- `sceneExplorerProjectChangeFailedTitle`: Title of modal when a change to the project fails
- `sceneExplorerProjectChangeFailedMessage`: Body of modal that appears when a change to a project fails
- `sceneExplorerProjectChangeSucceededTitle`: Title of modal when a change to the project succeeds
- `sceneExplorerProjectChangeSucceededMessage`: Body of modal that appears when a change to a project succeeds
- `sceneExplorerProjectReadOnlyTitle`: Title of modal when a change to the project fails because its read only
- `sceneExplorerProjectReadOnlyMessage`: Body of modal that appears when a change to a project fails becaues its read only
- `sceneExplorerUnsupportedFilesTitle`: Title of modal that appears when the user added unsupported files
- `sceneExplorerUnsupportedFilesMessage`: Body of modal that appears when the user added unsupported files

- `sceneExplorerClearAllButton`: Label on the button that clears the list of items in a modal
- `sceneExplorerCloseButton`: Label on button used when closing a modal

- `sceneExplorerErrorOpen`: Tooltip displayed when mouse is hovered over the warning next to a scene item in the Scene Explorer that vault client was unable to open or access
- `sceneExplorerErrorLoad`: Tooltip displayed when mouse is hovered over the warning next to a scene item in the Scene Explorer that vault client was unable to fully load
- `sceneExplorerPending`: Tooltip displayed when mouse is hovered over the alert next to a scene item in the Scene Explorer that vault client has not opened yet
- `sceneExplorerLoading`: Tooltip displayed when mouse is hovered over the alert next to a scene item in the Scene Explorer that vault client has not fully loaded yet
- `sceneExplorerFolderDefaultName`: Default name of new folders when they are added to the Scene Explorer

#### Model
- `sceneModelPosition`: Label used for the model position
- `sceneModelRotation`: Label used for the model rotation
- `sceneModelScale`: Label used for the model scale
- `sceneModelMetreScale`: Label for metadata on metre scale for this model (how many units per metric metre)
  - {0} the actual scale amount
- `sceneModelLODLayers`: Label for metadata on number of LOD layers for this model
  - {0} the number of LOD layers in this model
- `sceneModelResolution`: Label for metadata on resolution at time of conversion for this model
  - {0} the resolution of the model
- `sceneModelAttributes`: Label for metadata on number of attributes for this model
  - {0} number of attributes

#### Point of Interest
- `scenePOIDefaultName`: Default name of new POI objects when they are added to the Scene Explorer
- `scenePOIAreaDefaultName`: Default name of new AOI objects when they are added to the Scene Explorer
- `scenePOIAreaMeasureDefaultName`: Default name of new Area measurements when they are added to the Scene Explorer
- `scenePOILineDefaultName`: Default name of new Line objects when they are added to the Scene Explorer
- `scenePOILineMeasureDefaultName`: Default name of new Line measurements when they are added to the Scene Explorer
- `scenePOIAddPoint`: Used in the context menu for adding a new point at the current mouse position
- `scenePOIRemovePoint`: Used in the button to remove the selected point from this POI.
- `scenePOIPointPosition`: Label for the UI to override the position of the selected point
- `scenePOISelectedPoint`: Used on the slider to select a point from the points in this POI
- `scenePOILineLength`: Used in the label to show the length of the POI
- `scenePOIArea`: Used in the label to show the length of the POI
- `scenePOILabelColour`: The colour of the text on the label in the scene
- `scenePOILabelBackgroundColour`: The background colour of the label in the scene
- `scenePOILabelHyperlink`: Label used to show if there is a hyperlink attached
- `scenePOILabelOpenHyperlink`: Button used to open the hyperlink if its possible to open it
- `scenePOILabelSize`: Label for the drop down to select the label size
- `scenePOILabelSizeSmall`: text on the drop down to select the smaller than default text size
- `scenePOILabelSizeNormal`: text on the drop down to select the default text size
- `scenePOILabelSizeLarge`: text on the drop down to select the larger than default text size
- `scenePOIThumbnailSize`: Text on the label for the combo box to select the size of the thumbnail
- `scenePOIThumbnailSizeNative`: Text on the combo option to select the size of the thumbnail that displays the image at the native size
- `scenePOIThumbnailSizeSmall`: Text on the combo option to select the smallest size for the thumbnail
- `scenePOIThumbnailSizeLarge`: Text on the combo option to select the largest size for the thumbnail
- `scenePOILabelImageType`: Label for POI image type shown in the scene explorer
- `scenePOILabelImageTypeStandard`: One of the image types shown in the scene explorer
- `scenePOILabelImageTypeOriented`: One of the image types shown in the scene explorer
- `scenePOILabelImageTypePanorama`: One of the image types shown in the scene explorer
- `scenePOILabelImageTypePhotosphere`: One of the image types shown in the scene explorer
- `scenePOIReloadTime`: Label on the reload time for the image on a media node (in seconds)
- `scenePOIAttachModel`: Drop down menu in context menu and also button in submenu to attach a model to the POI
- `scenePOIAttachModelURI`: The label beside the URL box when attaching a model
- `scenePOIAttachModelFailed`: Error message that appears when the model fails to load
- `scenePOIAttachmentSpeed`: Label beside slider that sets the speed for attached model
- `scenePOIAttachCameraToAttachment`: The label on the context menu to link the camera to the attached model

- `scenePOILineSettings`: The header for the expansion for the other line settings
- `scenePOILineShowLength`: Label on the checkbox for when the user wants the length displayed on the label in the scene
- `scenePOILineShowAllLengths`: Label on the checkbox that will show each individual segment line length
- `scenePOILineShowArea`: Label on the checkbox for when the user wants the area displayed on the label in the scene
- `scenePOILineClosed`: Label on the checkbox for when the user wants the lines to form a closed polygon
- `scenePOILineColour1`: Label for the colour picker for the first colour for the lines
- `scenePOILineColour2`: Label for the colour picker for the second colour for the lines
- `scenePOILineWidth`: Label for how wide (in SRID units) the line is
- `scenePOILineStyle`: Label for the drop down to select a line style
- `scenePOILineStyleArrow`: Drop down item for the "Arrow" line style
- `scenePOILineStyleGlow`: Drop down item for the "Glow" line style
- `scenePOILineStyleSolid`: Drop down item for the "Solid" line style
- `scenePOILineStyleDiagonal`: Drop down item for the "Diagonal" line style
- `scenePOILineOrientation`: Label for the drop down to select a line orientation
- `scenePOILineOrientationVert`: Text on the drop down item for the "Vertical" / "Fence" orientation
- `scenePOILineOrientationHorz`: Text on the drop down item for the "Horizontal / Path" orientation
- `scenePOICancelFlyThrough`: Label for button that appears during fly-through, allows user to cancel the fly-through
- `scenePOIPerformFlyThrough`: Label for button that appears when POI has multiple nodes, will trigger a fly-through

- `sceneFilterPosition`: Label beside inputs to set the (center) position of the filter
- `sceneFilterRotation`: Label beside YPR inputs for filter rotation
- `sceneFilterExtents`: Label beside inputs to specify the half size of the filter
- `sceneFilterShape`: Label beside drop down 
- `sceneFilterShapeBox`: Label on drop down item to specify a box filter
- `sceneFilterShapeSphere`: Label on drop down item to specify a sphere filter
- `sceneFilterShapeCylinder`: Label on drop down item to specify a cylinder filter
- `sceneFilterInverted`: Label on checkbox to invert the result of the filter

- `sceneViewpointSetCamera`: Label for button that bookmarks the current camera position and orientation
- `sceneViewpointPosition`: Label for displaying the current stored camera position
- `sceneViewpointRotation`: Label for displaying the current stored camera orientation
- `sceneExplorerAddViewpoint`: Label used on tooltip when adding a viewpoint node
- `viewpointDefaultName`: Default name for stored camera positions added to the scene explorer

#### Live Feeds
- `liveFeedDefaultName`: Default name for live feed nodes added to the scene explorer
- `liveFeedUpdateFrequency`: Used on label for how many seconds delay between updates
- `liveFeedMaxDisplayTime`: Used on the label for how many seconds to display an item before hiding it
- `liveFeedDisplayDistance`: Label on slider for Maximum distance to display an item before it gets hidden
- `liveFeedTween`: Label on the checkbox that enables tweening
- `liveFeedDiagInfo`: Diagnostic info about the feed (shown only with "Show Diagnostic Info" enabled)
  - {0} total number of items
  - {1} total number of visible items
  - {2} number of second until the next update (or negative for seconds since starting the last update)
- `liveFeedGroupID`: When in `liveFeedModeGroups` mode, this is the input for the Group ID. [Will eventually be a drop down to select by group name]
- `liveFeedLODModifier`: Label on slider that modifies the distance at which to show particular levels of detail on live feed item labels

#### Polygon Models
- `polyModelCullFace`: Label on combo box to select which face culling mode
- `polyModelCullFaceBack`: Combo box item for back-face culling (default setting)
- `polyModelCullFaceFront`: Combo box item for front-face culling (inverted faces)
- `polyModelCullFaceNone`: Combo box item for no-culling (not recommended)
- `polyModelMesh`: Used on the label next to mesh information in the scene explorer with "Show Diagnostic Information" enabled 
- `polyModelTexture`: Used on the label next to mesh texture information in the scene explorer with "Show Diagnostic Information" enabled 
- `polyModelMatColour`: Used on the label next to mesh material colour information in the scene explorer with "Show Diagnostic Information" enabled 
- `polyModelIgnoreTint`: Checkbox to ignoring any tint material colour

#### View Sheds

- `viewShedDistance`: Label on slider to select view shed distance of effect
- `viewShedVisibleColour`: Label on colour picker to select colour to highlight regions visible by the view shed
- `viewShedHiddenColour`: Label on colour picker to select colour to highlight regions hidden from the view shed

### Attributes
- `attributeType`: Label when showing the attribute type
  - {0} one of the attributeTypeX strings below
- `attributeTypeUint8`: label when the attribute is a 8-bit unsigned integer
- `attributeTypeUint16`: label when the attribute is a 16-bit unsigned integer
- `attributeTypeUint32`: label when the attribute is a 32-bit unsigned integer
- `attributeTypeUint64`: label when the attribute is a 64-bit unsigned integer
- `attributeTypeInt8`: label when the attribute is a 8-bit signed integer
- `attributeTypeInt16`: label when the attribute is a 16-bit signed integer
- `attributeTypeInt32`: label when the attribute is a 32-bit signed integer
- `attributeTypeInt64`: label when the attribute is a 64-bit signed integer
- `attributeTypeFloat32`: label when the attribute is a 32-bit floating point value
- `attributeTypeFloat64`: label when the attribute is a 64-bit floating point value
- `attributeTypeColour`: label when the attribute is the special colour format
- `attributeTypeNormal`: label when the attribute is a packed normal
- `attributeTypeVec3F32`: label when the attribute is a 3x 32-bit floating point values (3D Vector)
- `attributeTypeUnknown`: label when the attribute is an unknown format

- `attributeBlending`: Label when showing the attribute blending mode
  - {0} one of the attributeBlendingX strings below
- `attributeBlendingMean`: Label when the value is averaged from the next highest resolution LOD layer
- `attributeBlendingSingle`: Label when the value is one of the points in the next highest resolution LOD layer
- `attributeBlendingUnknown`: Label when the blending is an unknown format

#### Bindings Interface

- `bindingsTitle`: Title for key bindings window
- `bindingsSettingsSave`: Label for save button on key bindings window
- `bindingsColumnName`: Header for key binding name column
- `bindingsColumnKeyCombination`: Header for key binding key combination column
- `bindingsColumnDescription`: Header for key binding description column
- `bindingsErrorBound`: Text warning when user attempts to bind an already bound key combination
  - {0} the key that it conflicts with
- `bindingsClear`: Text that displays instead of key combination when key binding isn't set
  
##### Bindings Descriptions

- `bindingsMapMode`: Key binding description displayed next to Map Mode binding
- `bindingsLockAltitude`: Key binding description displayed next to Lock Altitude binding
- `bindingsGizmoTranslate`: Key binding description displayed next to Gizmo Translate binding
- `bindingsGizmoRotate`: Key binding description displayed next to Gizmo Rotate binding
- `bindingsGizmoScale`: Key binding description displayed next to Gizmo Scale binding
- `bindingsGizmoLocalSpace`: Key binding description displayed next to Gizmo Local Space binding
- `bindingsFullscreen`: Key binding description displayed next to Presentation Mode (Fullscreen) binding
- `bindingsAddUDS`: Key binding description displayed next to Add UDS binding
- `bindingsRemove`: Key binding description displayed next to Remove binding
- `bindingsForward`: Key binding description displayed next to Forward binding
- `bindingsBackward`: Key binding description displayed next to Backward binding
- `bindingsLeft`: Key binding description displayed next to Left binding
- `bindingsRight`: Key binding description displayed next to Right binding
- `bindingsUp`: Key binding description displayed next to Up binding
- `bindingsDown`: Key binding description displayed next to Down binding
- `bindingsBindingsInterface`: Key binding description displayed next to Bindings Interface binding
- `bindingsCancel`: Key binding description displayed next to Cancel binding
- `bindingsRenameSceneItem`: Key binding description displayed next to Rename Scene Item binding
- `bindingsSave`: Key binding description displayed next to Save binding
- `bindingsLoad`: Key binding description displayed next to Load binding
- `bindingsUndo`: Key binding description displayed next to Undo binding

##### Errors
- `errorUnknown`: An unexpected error occurred.
- `errorCorruptData`: Corrupt data error shown in "Unsupported Files window" when converting files
- `errorUnsupported`: Unsupported file formats error shown in "Unsupported Files window" when converting files
- `errorOpenFailure`: Failed to open files error shown in "Unsupported Files window" when converting files
- `errorReadFailure`: Failed to read files error shown in "Unsupported Files window" when converting files
