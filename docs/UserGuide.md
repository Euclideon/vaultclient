# Euclideon Vault Client User Guide
## The Basics
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
- The iOS and Android builds are not currently publicly available.

### Logging In
After running the application you should see a login screen as below.

![Login Screen](images/loginscreen.png)

- `ServerURL` will be provided by your vault server provider*
- `Username` is the username you were given (or entered while registering). If you weren't provided with a username you may be able to register for one by visiting the server URL in your browser*
- `Password` is your account password

There are additional settings in the "Advanced Connection Settings" dropdown.

- `Proxy Address` is the address for your internet proxy (this is provided by your system administrator). It can additionally include the port number and protocol. Examples include: `192.168.0.1`, `169.123.123.1:80` or `https://10.4.0.1:8081`. Leaving this blank will attempt auto-detection.
- `Ignore Certificate Verification` will disable verification of the PEER and HOST certificate authorities. This setting should *ONLY* be used when instructed by your system administrator and only when errors are occurring during the login process. It will weaken the security between your computer and the Euclideon Vault Server.

> NOTE: `Ignore Certificate Verification` will not be saved due to the security risk associated. You will need to enable this setting each time you open the application.

*`Remember` will store the value you enter here for future use so you don't have to enter it every time.

After logging in you should see the full user interface:
![User Interface](images/afterlogin.png)

---

## User Interface

![Interface Numbers](images/interfacenumbered.png)

### 1. Scene Viewport
The scene viewport displays your scene (the Scene Explorer (see below) explains how to add things to the scene).

#### Moving around the viewport
> TIP: These settings are configurable in the [Input & Controls](#input-&-controls) settings menu.

Mouse Controls:
- Holding the left mouse button down and moving the mouse will "tumble" (Turning without moving) the camera.
- Holding the right mouse button down on a point in the scene (not the skybox) will begin "panning" (moving the camera, but not turning it) the camera. It will keep the originally hovered point _under the mouse cursor_.
- Holding the middle mouse button down on a point in the scene (not the skybox) will begin "orbiting" (keeping the camera the same distance from the point by turning and moving the camera) the selected point. It will keep the originally hovered point _at the same place on the screen_.
- The scroll wheel will "dolly" (move the camera in and out) from the hovered point (not the skybox).
  > TIP: If you prefer scroll wheel to change the move speed like Euclideon Geoverse MDM does, that option is available in the Mouse Pivot bindings in settings as well.

Default Keyboard Controls (with the "Scene" window focussed):
- `W` and `S` strafe the camera forward and backward at the current Camera Move Speed
- `A` and `D` strafe the camera left and right at the current Camera Move Speed
- `R` and `F` strafe the camera up and down at the current Camera Move Speed
- `Spacebar` locks altitude allowing user to move forward without it directly adjusting camera height (Z-axis lock)

### 2. Menu & Status Bar
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

- "New Scene" will remove all items from your Scene

#### Status Bar
The status bar shows a lot of useful information (not all of it will always be available).

- Number of files queued to load "([n] Files Queued),

### 3. Scene Explorer
![Scene Explorer](images/sceneexplorer0.png)

This pane shows you the assets currently in your scene, allows user to add UDS files and create new folders, Points of interest, Areas of interest & Lines.

> TIP: If you ever want to quickly clear the current scene, the "Projects" menu has a "New Scene" button that will remove everything from the scene.

#### Loading UDS Models
There are a number of ways to add models to the scene.

![Loaded Model](images/sceneexplorer.png)
![Model Information](images/sceneexplorer2.png)

1. Drag and Drop: On devices with folder exploring you can drag a file from your file explorer and drop it in the Euclideon Vault Client window to add the model to the scene.
2. Direct URI loading: You can type a URL or path in the Add UDS popup. The path field at the top of the pane allows URL and network paths to retrieve UDS files.
3. If you have access to projects on the server you will be able to click the "Projects" button in the menu and select a project to load. Projects are loaded from the Vault Server you're connected to.

### 4. Settings
![Settings](images/settings.png)

  - Settings contain both a large number of functional and visual customisations to the Vault Client such as changing the display theme, showing FPS, onscreen compass, displaying diagnostic and advanced GIS settings.

Appearance

![Appearance Settings](images/appearance.png)
  - Allows users to change presentation UI, Mouse Anchor Shape, Default Voxel Shape.


Input and Controls

![Input Settings](images/inputcontrol.png)

  - Allows on screen mouse control, Touchscreen friendly UI, inverse X and Y axis, mouse pivot settings (bindings & mouse icon control).


Viewport

![Viewport Settings](images/viewport.png)

  - Changes minimal viewing distance and maximum viewing distance, allows changing field of view.
  > TIP: If experiencing loss of image, change min and max settings

Maps & Elevation

![Maps & Elevation Settings](images/mapsandelevation.png)

  -  Toggle Map Tiles and allow mouse to lock to Maps.

![Tile Server](images/tileserver.png)

  -  Tile Server allows overlay with existing maps, clicking 'Tile Server' prompts user to enter an https address for background tiles with the option to import png or jpg.
    - Transparency slider adjusts the overall visibility of objects allowing Tiles to be more visible with vault objects in foreground.
    - Map height adjusts the height of tiles in relation to vault objects.
    - Blending with objects allows tile maps to overlay, underlay or feature in hybrid mode with existing vault objects.

Visualization

![Visualization Settings](images/visualizationsettings.png)

  - Allows users to change between Colour, Intensity and Classification Display modes.
  - Enabling Color by Height, Depth and Edges with max / min sliders for each respective property with a color wheel to showcase features.
  - Enabling Contours allows coloured contours sorted by distance and band height with a color wheel.
![Visualization Settings Expanded](images/visualizationall.png)

> TIP: If you ever change a setting and can't recall what you've changed, you can reset _all_ settings by going to the "System" menu and selecting "Restore Defaults", or you can reset individual groups of settings by right clicking their header and selecting "Restore Defaults"

### 5. GIS / Scene Info
![Camera Information](images/camerabox.png)
  - Scene contains several key functions, such as X,Y,Z camera coordinates, camera rotation coordinates and camera movement speed. Toggled by 'Show Camera Button'.
  - Displays Latitude and Longitude, along with current camera altitude.
  - Several clickboxes allow Lock Altitude (spacebar default), Show Camera Information, Show projection information (Displays SRID, MGA zones information).
  - Gismo Translate, Gizmo Rotate, Gizmo Scale and Gizmo local space buttons

### 6. Copyright, Watermark & Compass
  - Adding Copyright Data can be done in the metadata subtab during converting
  - Compass by default is on, Appearance pane contains on/off checkbox
  - Watermarks can be viewed on each UDS file, by clicking on the UDS file in the Scene Explorer and then viewing the [Watermark] identification tag

### 7. Convert Tab
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

The "X" button beside the convert job in the "Convert Jobs" section allows you to cancel a running convert (it will cancel at the next 'safe' point to do so and clean up temporary files). After a job has completed the "X" button also allows you to remove it from the list. A successful conversion will also have an "Add to Scene" button that adds the converted UDS to the existing scene.

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
- macOS: `???`
- Linux: `???`
- iOS: `???`
- Android: `???`

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
> Authenticated Proxies at the time of this build do not have official proxy support, when using proxies the format “protocol://username:password@domain/URI” should be adopted, network metadata is not transmitted when using proxies and is stored in plain text file, which may assist solving connection issues.

`Could not open a secure channel` Why is this popping up?
> If you're using a proxy, your network may not be sending encrypted data, ticking the "ignore certificate verification" may circumvent this issue. Note: Network Security certificates will not be verified.
