# **Euclideon Vault Client User Guide**
## Getting Started
### Unpacking / Installing

#### Windows
1. Download the latest ZIP that has been approved for use from your Euclideon Vault Server provider.
2. Unpack the entire contents of the .zip file
3. If you have a computer capable of running OpenGL run `vaultClient_OpenGL.exe` otherwise for older machines `vaultClient.exe`. The OpenGL version will offer a smoother experience on most machines.

#### macOS
1. Download the latest DMG package that has been approved for use from your Euclideon Vault Server provider.
2. Open the DMG and drag the "Euclideon Vault Client" icon onto the provided "Applications" icon
4. Run the "Euclideon Vault Client" application from "Applications"

#### Debian Linux Distributions (Requires Desktop Environment)
1. Use your package manager to install the following:
    - SDL2 (Minimum version 2.0.5)
    - libCURL (Version 3)
2. Download the latest tar.gz that has been approved for use from your Euclideon Vault Server provider.
3. Unpack the entire contents of the tar.gz
4. Run `vaultClient`

#### iOS / Android
The iOS and Android builds are not currently publicly available.

### Logging In
After starting the application you will see the login screen below.

![Login Screen](images/loginscreen.png)

`ServerURL` will be provided by your vault server provider.

`Username` is the username you were given (or entered while registering). If you weren't provided with a username you may be able to register for one by visiting the server URL in your browser.

`Password` is your account password.

If `Remember` is ticked, the client will store the value entered so you don't have to enter it every time.

There are also additional settings in the "Advanced Connection Settings" dropdown:

- `Proxy Address` is the address for your internet proxy (this is provided by your system administrator). It can additionally include the port number and protocol. Examples include: `192.168.0.1`, `169.123.123.1:80` or `https://10.4.0.1:8081`. Leaving this blank will attempt auto-detection.
  - Many companies & organizations have authenticated proxies. Entering a proxy here is not enough to ensure that the proxy will work in all situations. This box does support authentication using _protocol_://_username_:_password_@_proxyaddress_:_port_ e.g. `https://developer:secretPassword123@10.4.0.255:8081` or `user:p455w0rd@proxy.local`. __PLEASE NOTE THAT THIS SETTING IS STORED IN PLAIN TEXT! AFTER CONFIRMING YOU HAVE AN AUTHENTICATED PROXY- DELETE YOUR INFORMATION FROM THIS BOX!__ A future release will provide a secure way to store proxy information.
- `Ignore Certificate Verification` will disable verification of the PEER and HOST certificate authorities. This setting should *ONLY* be used when instructed by your system administrator and only when errors are occurring during the login process. It will weaken the security between your computer and the Euclideon Vault Server.

> NOTE: `Ignore Certificate Verification` will not be saved due to the security risk associated. You will need to enable this setting each time you open the application.

After you have entered your credentials, click `Login!` and you will see an empty scene in the viewport similar to the image below.

![User Interface](images/afterlogin.png)

---

## User Interface

Shown below is a numbered diagram of a screenshot of the user interface.

![Interface Numbers](images/interfacenumbered.png)

1. [**Scene Viewport**](#Scene-Viewport)
2. [**Menu and Status Bar**](#Menu-and-Status-Bar)
3. [**Scene Explorer**](#Scene-Explorer)
4. [**Settings**](#Settings)
5. [**Scene Info and Controls**](#Scene-Info-and-Controls)
6. [**Copyright and Compass**](#Copyright-and-Compass)
7. [**Watermark**](#Watermark)
8. [**Convert Tab**](#Convert-Tab)

The following numbered sections will explain each of these features in full detail.

### 1. Scene Viewport
The scene viewport displays the current scene. Section [**3. Scene Explorer**](#Scene-Explorer) explains how to add models to your scene.

#### Moving around the viewport
> NOTE: These settings are configurable in [**Input and Controls**](#input-and-controls) (see section [**4. Settings**](#Settings)).

Default Mouse Controls:
- Holding the `left mouse` button and moving the mouse will "tumble" (Turning without moving) the camera.
- Holding the `right mouse` button on a point in the scene (not the skybox) will begin "panning" (moving the camera, but not turning it) the camera. It will keep the originally hovered point _under the mouse cursor_.
- Holding the `middle mouse` button on a point in the scene (not the skybox) will begin "orbiting" (keeping the camera the same distance from the point by turning and moving the camera) the selected point. It will keep the originally hovered point _at the same place on the screen_.
- The `mouse scroll wheel` will "dolly" (move the camera in and out) from the point where the mouse is hovering (will not work with the skybox).
  > TIP: If you prefer scroll wheel to change the move speed like Euclideon Geoverse MDM does, that option is available in the Mouse Pivot bindings in the Settings window as well.

Default Keyboard Controls (with the "Scene" window focused):
- `W` and `S` strafe the camera forward and backward at the current Camera Move Speed.
- `A` and `D` strafe the camera left and right at the current Camera Move Speed.
- `R` and `F` strafe the camera up and down at the current Camera Move Speed.
- `Spacebar` locks altitude, allowing the user to pan and strafe the camera without changing the camera's height (Z-axis lock).

#### Hotkeys
- `F5` Toggles presentation mode, hiding the interface and going fullscreen.

##### Gizmo Hotkeys
- `B` Sets the gizmo to translation mode, allowing user to move objects.
- `N` Sets the gizmo to rotation mode, allowing user to rotate objects.
- `M` Sets the gizmo to scale mode, allowing user to resize objects.
- `C` Toggles the gizmo between local and world space coordinates.

##### Scene Explorer Hotkeys
- `Ctrl+U` Opens a popup for loading UDS files.
- `Delete` Removes the selected item(s) from the scene.

### 2. Menu and Status Bar

![Status Bar](images/statusbar.png)

The menu bar consists of 2 sections, the left section with menu drop downs and the right section with status information.

#### System Menu

![System Menu](images/systemdown.png)

The system menu has a number of system related functions.


- `Logout` terminates your current session with the server. This is a security feature to prevent attackers from hijacking your session. It also takes you back to the login screen in case you want to change users or servers.

![Logged Out](images/loggedout.png)
  > TIP: Logout does not unlock your current licenses for use by other users, they must still time out.
- `Restore Defaults` resets _all_ settings back to how they were when you ran Euclideon Vault Client the first time.
- `About` displays a popup with the version and license information for the current version of Client.
- `Release Notes` displays a popup with information on the current and previous release changes.
- `Quit` logs you out and then closes the program.

#### Windows
![Windows Menu](images/windowsmenu.png)

The windows menu allows you to show or hide various windows in the Euclideon Vault Client application.

- Contains windows: Scene, Scene Explorer, Settings, Convert

#### Projects
![Projects Menu](images/projectdropdown.png)

The projects menu has a new scene button and if additional projects are available to your user, it will show those projects here as well.

- "New Scene" will remove all items from your Scene and create a new empty Scene.

#### Status Bar
The status bar shows a lot of useful information (not all of it will always be available).

- Number of files queued to load "([n] Files Queued),

### 3. Scene Explorer
![Scene Explorer](images/sceneexplorer.png)

The Scene Explorer window shows you the assets currently in your scene, and also allows the user to add UDS models and create new folders, points of interest, areas of interest and lines. Non-UDS models can also be imported if they are legacy files (.udg or .ssf) or they can be converted to UDS format using the Conversion tool (see section [**8. Convert Tab**](#Convert-Tab)).

#### Quick Action Menu
The buttons across the top of the Scene Explorer allow quick access to add or remove from the scene. From left to right:
- `Add UDS` (hotkey `Ctrl + U`) This button opens the dialog box to add a UDS model to the scene. The Path/URL can include files on the local drives, network drives, UNC Paths, HTTP, HTTPS, FTP & FTPS.
   ![AddUDS](images/adduds.png)
- `Add Point of Interest` This button adds a point of interest to the scene.
- `Add Area of Interest` __This button is not enabled in this version of Euclideon Vault Client.__
- `Add Lines` __This button is not enabled in this version of Euclideon Vault Client.__
- `Add Folder` Adds a folder to the scene explorer. This can help with organizing your scene.
- `Delete` (hotkey `Delete`) This deletes all selected items from the scene

> TIP: If you ever want to quickly clear the current scene, the "Projects" menu has a "New Scene" option that will remove everything from the scene.

#### Scene Items

Underneath the quick action menu you will find the contents of your scene.

![Model Information](images/sceneexplorer2.png)

- The checkbox to the left of each item is a visibility checkbox. Disabling this hides the element in the scene.
- The arrow indicator expands the items information. For folders this will show the contents of the folder.

> Note that disabling visibility for a folder will also disable visibility for _all_ items in that folder- including subfolders and their contents.

##### Selecting Items

Left-click on an item in the Scene Explorer to select it. Selected items will appear highlighted.

You can select multiple items in the scene explorer by holding `Ctrl` and then left-clicking them. Doing this on a selected item will deselect it.

Single clicking without `Ctrl` will deselect all items and select just the item you have clicked.

##### Reorganizing the scene

Items or groups of items in the scene explorer can be reordered by holding left-click and dragging them around. A yellow line indicates where the item(s) will be after you release them.

> When dropping onto folders. It is best to open the folder you wish to drag into before starting the click-drag operation.

#### Loading UDS Models
There are a number of ways to add models to the scene.

1. Drag and Drop: On devices with folder exploring you can drag a file from your file explorer and drop it in the Euclideon Vault Client window to add the model to the scene.
2. Direct URI loading: You can type a URL or path in the `Add UDS` popup. The path field at the top of the pane allows URL and network paths to retrieve UDS files.
3. If you have access to projects on the server you will be able to click the "Projects" button in the menu and select a project to load. Projects are loaded from the Vault Server you're connected to.

### 4. Settings
![Settings](images/settings.png)

The settings window has five subheadings which allow the user to customise and control how the vault client looks and operates. The subheading can be opened by left-clicking on the arrows to the left.

>TIP: To restore all default values for any of these settings, simply right-click on the header and then select `Restore Defaults`.

#### Appearance

![Appearance Settings](images/appearance.png)

Here the user can adjust several settings that change the appearance of the client:

  - A visual theme can be chosen to change the overall colour scheme, options are Light or Dark.
  - The user can choose to display diagnostic information, such as the frame-rate and model bounds.
  - The user can choose to display advanced GIS settings in the top-right corner.
  - The user can choose to limit the FPS when the client is running in the background, reducing interference with other open programs.
  - The compass in the bottom-right corner can be toggled on/off.
  - By default the UI is not visible in Presentation or Fullscreen mode (hotkey `F5`) but this setting can be changed.
  - The appearance of the mouse when anchoring to models or map tiles can be changed.
  - The user can set the shape of voxels in the main window to either points, squares or cubes.

#### Input and Controls

![Input Settings](images/inputcontrol.png)

Expanding this subheading allows the user to change settings that affect how they interface with the client:

  - The user can toggle on-screen mouse controls.
  - The user can optimise the client for use with a touchscreen.
  - The X and Y axes can be inverted, which will affect camera movement with the mouse.
  - Mouse-controlled camera movement can be customised by the user. Descriptions of the different mouse pivot bindings can be found in section [**1. Scene Viewport**](#Scene-Viewport).

#### Viewport

![Viewport Settings](images/viewport.png)

These options allow the user to changes the camera's minimum and maximum viewing distance, and field of view of the camera lens.
  > TIP: If experiencing loss of image, try changing the near and far plane settings to make your objects visible again.

#### Maps & Elevation

![Maps & Elevation Settings](images/mapsandelevation.png)

The first checkbox is used to toggle the visibility? of map tiles in the viewport.

The second checkbox allows the mouse to lock to map tiles when moving the camera with the mouse.

Tile Server allows overlay with existing maps??, clicking `Tile Server` prompts the user to enter a https:// address for background tiles, with the option to import .png or .jpg files.??

![Tile Server](images/tileserver.png)

Map height adjusts the height of map tiles in the scene.

Blending allows map tiles to overlay, underlay or feature in hybrid mode with existing objects.

The transparency (opacity) slider adjusts the transparency (opacity) of the map tiles so they don't obscure the visibility of objects in the scene.

The `Set to Camera Height` button can be used to set the height of the map tiles to the camera's current height. This can be used to place map tiles outside the default range of the slider of +/-1000m.?

#### Visualization

![Visualization Settings](images/visualizationsettings.png)

This pane allows users to change between Colour, Intensity and Classification display modes.

In intensity display mode, an extra option appears for the use to specify the min and max intensity of the display? as shown below.

![Visualization Intensity](images/visualizationintensity.png)

In classification display mode, a checkbox appears which, when enabled, allows the user to customise the colours of objects corresponding to their designated classifications.?

![Visualization Classification](images/visualizationclassification.png)

Below Display Mode are four additional visualization options.
  - `Enable Edge Highlighting` ???
  - `Enable Colour by Height` Applies colours along the specified two-colour gradient to all objects in the scene based on their height.?
  - `Enable Colour by Depth` ???
  - `Enable Contours` Displays coloured contours sorted by distance and band height.?

All four display options also allow the user to customise the colours. They are shown expanded below.

![Visualization Settings Expanded](images/visualizationall.png)

> TIP: If you ever change a setting and can't recall what you've changed, you can reset _all_ settings by going to the "System" menu and selecting "Restore Defaults", or you can reset individual groups of settings by right clicking their header and selecting "Restore Defaults".

### 5. Scene Info and Controls

![Camera Information](images/camerabox.png)

This pane contains several useful features, numbered on the screenshot above.
  1. `Lock Altitude` (hotkey `Spacebar`) will keep the camera's height constant (Z-axis lock) when panning with the mouse or strafing with the keyboard (this is the same as enabling "Helicopter" mode in Geoverse MDM).
  2. `Show Camera Information` displays the camera position and rotation coordinates, and the camera move speed slider.
  3. `Show Projection Information` displays SRID information in the top-right corner.
  4. `Gizmo Translate` (hotkey `B`) sets the gizmo to translation mode, allowing the user to move objects.
  5. `Gizmo Rotate` (hotkey `N`) sets the gizmo to rotation mode, allowing the user to rotate objects.
  6. `Gizmo Scale` (hotkey `M`) sets the gizmo to scaling mode, allowing the user to resize objects.
  7. `Gizmo Local Space` (hotkey `C`) toggles the gizmo's operational coordinate system between local coordinates (relative to the object) and world coordinates (relative to the world space).
  8. `Fullscreen` (hotkey `F5`) toggles presentation mode and sets the client to fullscreen. By default this will hide the user interface.

This area also displays the Latitude, Longitude and Altitude of the camera's current position, if the camera is projected in a geospatial zone.

### 6. Copyright and Compass
Copyright data will be displayed in the bottom-right corner of the viewport. Copyright Data can be added to new models in the metadata textbox during conversion.

A compass is also displayed in this corner, indicating the camera's current orientation within the global coordinate space. Compass by default is on, but the Appearance pane contains an on/off checkbox to toggle this setting (see [**Appearance**](#Appearance) in section [**4. Settings**](#Settings)).

### 7. Watermark
Watermarks can be viewed on each UDS file, by clicking on the UDS file in the Scene Explorer and then viewing the [Watermark] identification tag.

### 8. Convert Tab
Converting to UDS file format can be done from within the Euclideon Vault Client as well. Loading a supported pointcloud format will open the "Convert" window if it isn't already open. A quick 'pre-parse' is done to detect the file format, supported attributes etc. By loading the start of the file, this stage also does an estimate of the total number of points the file has.

  - Supported file conversion formats (?????).

> If you aren't able to find the convert tab, it can be displayed from the `Windows > Convert` flag, once selected the convert option will be displayed next to the Scene tab.

![Convert Window](images/convertpane.png)

There are a few additional settings to modify here.

- Output name
  - This is the final name and path of the exported UDS file.
  - Ideally this should be the target location of the file so that you don't need to copy the file at the end. Make sure there is enough space in the target directory to store the entire file that will be output.
- Temp Directory
  - Defaults to beside the Output file but should ideally be on a high speed local drive.
  - A lot of small files will be written here for a short period of time during conversion so make sure you have enough disk space to store the files, a good rule of thumb is that the temporary directory uses about as much space as the uncompressed raw inputs do as the points in the temporary files aren't compressed yet.
- Continue processing after corrupt/incomplete data (where possible) [Defaults to Off]
  - If corruption or incomplete data is detected during a conversion this setting will decide whether the conversion gets cancelled (off) or continues, skipping the corrupted points.
- Point Resolution [Defaults to 0.01 and then source size]
  - This setting determines the size of the points (in meters) in the pointcloud after conversion.
  - It will attempt to find a good size based on the source data and can also be set manually by first checking the "Override" checkbox and then typing in the text field.
- SRID [Defaults to 0]
  - The SRID is the Geotagged ID of the GIS zone for the exported model.
  - Usually the source data will have information about what zone the data is stored in, search for "EPSG code" or "SRID code" in the meta data.
  - If the pre-parse can detect the SRID it will set it here as well and can also be set manually by first checking the "Override" checkbox and then typing in the text field.
  - It assumes that the input is already in the correct zone and does not currently do a conversion.
- Input Files
  - Expanding this will give you the list of files that will be merged together into the new UDS.
  - The estimated number of points in each file will be shown. During conversion the progress for each file will also be shown.

- Meta Data
    - This section allows user to import watermark by drag and dropping, adding metadata related information such as Author, Comments, Copyright and License holders.
  - Author
    - Entry field for name of Authors
  - Comment
    - Entry field for Comments
  - Copyright
    - Entry field for Name/Company of Copyright holders
  License
    - Entry field for Name of License holders (watermark)

You can keep track of progress on the convert tab, go back to working in the scene tab or prepare another conversion that will begin as soon as the first convert completes.

![Conversion Running](images/convertrunning.png)

The "X" button beside the convert job in the "Convert Jobs" section allows you to cancel a running convert (it will cancel at the next 'safe' point to do so and clean up temporary files). Once cancelled the 'Begin Convert' button and the configuration options will reappear enabling you to restart the conversion. After a job has completed the "X" button also allows you to remove it from the list. A successful conversion will also have an "Add to Scene" button that adds the converted UDS to the existing scene.

> We do not recommend running multiple converts at the same time. Converting is a memory and processor intensive process so it's almost always faster to have 1 convert running at a time. Euclideon Vault Client helps with this by allowing you to queue multiple jobs to run one after another.

---

## Technical Information

### Requirements

- VaultClient_OpenGL Requires OpenGL
- Internet Connection
- Euclideon Vault License

### Settings On Disk
Where the settings file is stored depends on your OS.
- Windows: `%AppData%/Euclideon/client/settings.json`
- macOS: `~/Library/Application Support/euclideon/client/settings.json`
- Linux: `~/.local/share/euclideon/client/settings.json`
- iOS: `???`
- Android: `???`

### Third Party Licenses
Euclideon Vault Client uses a number of third party libraries.

- `Dear ImGui` from [GitHub](https://github.com/ocornut/imgui)
- `Dear ImGui - Minimal Docking Extension` from [GitHub](https://github.com/vassvik/imgui_docking_minimal)
- `ImGuizmo` from [GitHub](https://github.com/CedricGuillemet/ImGuizmo)
- `libSDL2` from [libsdl](https://libsdl.org)
- `GLEW` from [SourceForge](http://glew.sourceforge.net/)
- `Nothings/STB` single header libraries from [GitHub](https://github.com/nothings/stb)

Euclideon Vault Development Kit (VDK) uses the following additional libraries.

- `cURL` from [GitHub](https://github.com/curl/curl)
- `Nothings/STB` single header libraries from [GitHub](https://github.com/nothings/stb)
- `libdeflate` from [GitHub](https://github.com/ebiggers/libdeflate)
- `mbedtls` from [GitHub](https://github.com/ARMmbed/mbedtls)
- `miniz` from [GitHub](https://github.com/richgel999/miniz)

## FAQs

Why can't I see my image?
> Try adjusting the viewport settings until the image appears, ensure you have loaded and enabled the layer in the scene viewer pane.

How can I turn the compass off?
> Visualization pane on the right hand side in settings, press the checkbox and you've toggled it off.

How do I lock altitude when moving the camera?
> Press Space bar to toggle, or press the local altitude button in the 'status' window (top left of the Scene window)

How do I load my previous projects?
> Press projects, and view the previous Euclideon vault projects from the dropdown box.

How do I convert into UDS?
> Converting to UDS is as simple as pressing the convert window and naming the output and selecting the destination of the converted file.

A converting error occured, what do I do?
> Due to either corrupt or incomplete data, clicking the 'continue converting' tickbox will complete converting regardless of data integrity. (Visuals may vary)

I want to demonstrate key features of my 3D model, how can I do that?
> Check out the visualization dropdown box in the Settings pane, on the right hand side of the Vault Client.

How do I adjust the mouse controls?
> View Mouse Pivot bindings in the input and controls menu in settings.

I closed the settings window and Scene Explorer how do I display them?
> Open the window box and press the Windows which you want to display.

What is the Name and Light next to my license name mean?
> Status of License, Which license is being used and how recently your license has synced with the Vault. Green = Good, Yellow = >30 second disconnected, Red = >60 seconds disconnected (Services may be hindered)

My 3d object is hidden behind a tile map, how do I see it?
> Changing the Transparency or the Blending in Maps and elevation may make it easier to see your object, using a combination of these for varied affect.

I keep getting the "logged out" screen, how do I fix this issue?
> Check your internet access and license permissions and try again. For offline use, upgrade your license.

I cannot log in?
> Check your internet connection, your license status, and ensure you've used the correct URL, username and password. Make sure your URL has a closed bracket at the end of it if using the default server URL.

I cannot connect to the vault, how do I resolve this issue?
> Check your firewall or proxy settings, if running a proxy ensure the address is proxy.euclideon with the port number 80.

My proxy isn't working, why?
> Authenticated Proxies at the time of this build do not have official proxy support, when using proxies the format `protocol://username:password@domain/URI` should be adopted, network metadata is not transmitted when using proxies and is stored in plain text file, which may assist solving connection issues.

`Could not open a secure channel` Why is this popping up?
> If you're using a proxy, your network may not be sending encrypted data, ticking the "ignore certificate verification" may circumvent this issue. Note: Network Security certificates will not be verified.
