# Euclideon Vault Client User Guide

### Table of Contents
- [**Downloading the Euclideon Vault Client software**](#downloading-the-euclideon-vault-client-software)
- [**Unpacking and Installing**](#unpacking-and-installing)
  - [**Windows**](#windows)
  - [**macOS**](#macos)
  - [**Ubuntu Linux Distributions**](#ubuntu-linux-distributions)
  - [**iOS and Android**](#ios-and-android)
- [**Logging In**](#logging-in)
  - [**Login Errors**](#login-errors)
- [**User Interface**](#user-interface)
  - [**1. Menu and Status Bar**](#menu-and-status-bar)
    - [**System Menu**](#system-menu)
    - [**Windows Menu**](#windows-menu)
    - [**Projects Menu**](#projects-menu)
    - [**Status bar**](#status-bar)
    - [**Loading a sample project**](#loading-a-sample-project)
  - [**2. Scene Viewport**](#scene-viewport)
  - [**3. Scene Explorer**](#scene-explorer)
  - [**4. Settings**](#settings)
  - [**5. Scene Info and Controls**](#scene-info-and-controls)
    - [**Gizmos: moving items in the scene**](#gizmos-moving-items-in-the-scene)
  - [**6. Copyright and Compass**](#copyright-and-compass)
  - [**7. Watermark**](#watermark)
  - [**8. Convert**](#convert)
    - [**Converting in Euclideon Vault Client**](#converting-in-euclideon-vault-client)
    - [**Converting From Command Line (CMD)**](#converting-from-command-line-cmd)
- [**Rearranging Windows**](#rearranging-windows)
- [**Hosting a model**](#hosting-a-model)
- [**Technical Information**](#technical-information)
  -  [**Requirements**](#requirements)
  -  [**Settings On Disk**](#settings-on-disk)
  -  [**Third Party Licenses**](#third-party-licenses)
- [**FAQs**](#faqs)
- [**Contact Us**](#contact-us)

\newpage

## Downloading the Euclideon Vault client software

To use Euclideon Vault, you will need to download the Euclideon Vault Client software for your respective operating system.


| Step | Action |
| :--- | :------------------------------ |
| 1. | Click on the link provided in the email that Euclideon sent: [https://earth.vault.euclideon.com/](https://earth.vault.euclideon.com) |
| 2.| If this is your first time downloading the Euclideon Vault Client software, then **Login using the credentials provided in the email from the previous step**. <br />Otherwise, login using your existing credentials. |
| 3. |Change your password from the temporary one Euclideon gave you by clicking on your **Username** in the top right-hand corner of the window and choosing **Change Password**. |
| 4. | The packages screen lists the latest Euclideon Vault Client software version. <br />Click on **Download** for your operating system. <br />![Latest Euclideon Vault Client software for download](./images/packages.png "Latest Euclideon Vault Client software for download") <br />|
| 5. | If you do not see the version that has been approved for use from your Euclideon Vault Server provider, then **Click on Packages** on the top  menu. |
| 6. | Select **Download** for your version and operating system. <br /><br />![List of available Euclideon Vault Client packages](./images/allpackageslist.png "List of available Euclideon Vault Client packages") <br />|  
| 7. | Select the location to download the EuclideonVaultClient\_OS\_version.zip file. |
| 8. | Click **Save** to download to your default download location defined by your browser, or <br />Click the up arrow next to **Save** to select **Save As** to select another download location.  |
| 9. | Change to the directory where you downloaded the file.  |
| 10. |Go to [Unpacking and Installing](#unpacking-and-installing) for specific installation instructions on your operating system. |
 

## Unpacking and Installing

### Windows

1.	If you haven't already, change to the directory where you [downloaded](#downloading-the-vault-client-software) the Euclideon Vault Client software. 
2.	Right-click on the `EuclideonVaultClient\_OS\_version.zip` file and select **Extract Here** to unpack the files to the current directory.
3.	If you have a computer capable of running OpenGL, then run \(double-click\) `vaultClient_OpenGL.exe`.<br /> 
	Otherwise, run \(double-click\) `vaultClient.exe`. 
	

### macOS

1.	Locate the Euclideon Vault Client DMG package that you [downloaded](#downloading-the-vault-client-software) previously. 
2.	Open the DMG and drag the "Euclideon Vault Client" icon onto the provided "Applications" icon.
3.	Run the "Euclideon Vault Client" application from "Applications".

### Ubuntu Linux Distributions

Using this package requires a desktop environment.

1.	Use your package manager to install the following:
    -	SDL2 (Minimum version 2.0.5) 
    -	libCURL (Version 3) 
2.	Locate the Euclideon Vault Client tar.gz package that you [downloaded](#downloading-the-vault-client-software) previously. 
3.	Unpack the entire contents of the tar.gz
4.	Type `./vaultClient` or double-click on `vaultClient` to initiate the Euclideon Vault Client interface.

### iOS and Android
The iOS and Android builds are not currently publicly available.

## Logging In
After starting the application you will see the login screen below.

![Euclideon Vault Client login screen](./images/loginscreen.png "Euclideon Vault Client login screen")

| Parameter | Description |
| :--- | :---------------------------  |
|**ServerURL**| Will be provided by your Euclidon Vault Server provider.|
|**Username** |The username you were given (or entered while registering). If you weren't provided with a username you may be able to register for one by visiting the server URL in your browser.<br /><br /> TIP: If `Remember` is ticked, the client will store the value entered so you don't have to enter it every time.|
|**Password** |Your account password. |

There are also additional settings in the **Advanced Connection Settings** dropdown:

| Parameter | Description |
| :------- | :---------------------------  |
|**Proxy Address** | The address for your internet proxy (this is provided by your system administrator). <br />It can additionally include the port number and protocol.<br /><br />Examples include: `192.168.0.1`, `169.123.123.1:80` or `https://10.4.0.1:8081`.<br />Leaving this blank will attempt auto-detection. |
|**User Agent** | A field included in HTTP request headers. Very few users will need to specify this setting. |
|**Ignore Certificate Verification** | Will disable verification of the PEER and HOST certificate authorities.<br /><br />This setting should **only** be used when instructed by your system administrator and only when errors are occurring during the login process, as it will weaken the security between your computer and the Euclideon Vault Server.<br /><br />NOTE: `Ignore Certificate Verification` will not be saved due to the security risk associated. You will need to enable this setting each time you open the application. |

After you have entered your credentials, click **Login** and you will see an empty scene in the viewport similar to the image below.  
![Euclideon Vault Client interface after login](./images/afterlogin.png "Euclideon Vault Client interface after login")

### Login Errors

| Message | Reason |
| --------- | --------------- |
| **Could not connect to server.** | There are a number of possible causes for this message. The most common is the server address entered into the field is not correct. The system is case- and space-sensitive. Ensure there are no spaces before or after the Server URL. |
| **Username or Password incorrect.** | This is shown in a number of sitations with the most common being that the password is not correct for the given username.<br /><br />The username is incorrect<br /><br />The username may not exist or has been banned.<br /><br />The system is case- and space-sensitive. Ensure there are no spaces before or after the Username or Password. |
| **Your clock doesn't match the remote server clock.** | In order to maintain system security, the client and server must agree on the time to within 5 minutes. Having the server and client both set to syncronize with "universal" NTP time is preferable. This error will occur if the timezone of either the server or the client is not set correctly. |
| **Could not open a secure channel to the server.** | The client was able to connect to the server but an error occurred while verifying the server was the intended target or negotiating an encrypted connection. |
| **Unable to negotiate with the server, please confirm the server address.** | The client was able to connect to the server provided in the server URL field but the server did not respond as expected. This usually occurs if the server is not a Euclideon Vault Server. | 
| **Unable to negotiate with proxy server, please confirm the proxy server address** | This occurs when the proxy information is partially correct. Further details may be required before the connection through the proxy is correct (usually proxy authentication details). | 
| **Unknown error occurred, please try again later.** | This error was not one of the above errors and will require Euclideon Support assistance to resolve. Please contact Euclideon at support@euclideon.com or go to https://euclideon.com to access our online support. | 

> TIP: When an error occurs, press the `Alt` and `Ctrl` keys simultaneously to display an additional error code after the message. Give the error message + code to Euclideon Support so they can help you resolve the problem. You might use this in situations where you get an **Unknown Error Occurred** message.

![Login error message](./images/login-error-code.png "Login error message")



## User Interface

Use the Euclideon Vault Client Interface to view, import, and convert 3D models. Refer to the following sections that correlate to the numbers on the image for more detailed information on how to use that feature.

![Euclideon Vault Client Interface](./images/interface-numbered.png "Euclideon Vault Client Interface")  

[**1. Menu and Status Bar**](#menu-and-status-bar)  
[**2. Scene Viewport**](#scene-viewport)  
[**3. Scene Explorer**](#scene-explorer)  
[**4. Settings**](#settings)  
[**5. Scene Info and Controls**](#scene-info-and-controls)  
[**6. Copyright and Compass**](#copyright-and-compass)  
[**7. Watermark**](#watermark)  
[**8. Convert Tab**](#convert)  


### MENU AND STATUS BAR

The menu bar consists of 2 sections, the left section with menu drop downs and the right section with status information. The status information will change depending on what models are loaded.

![Status bar](./images/menu-status-bar.png "Status bar")

#### System Menu

![System menu](./images/systemdown.png "System menu")

The **System** menu has a number of system related functions:

- `Logout` terminates your current session with the server and takes you back to the login screen. Logging out prevents attackers from potentially hijacking your session and enables you to change users or servers.

![Logged out](./images/loggedout.png "Logged out") 


> TIP: Logout does not unlock your current licenses for use by other users, they must still time out.

- `Restore Defaults` resets all settings back to how they were when you ran Euclideon Vault Client the first time.
- `About` displays a popup with the version and license information for the current version of Client.
- `Release Notes` displays a popup with information on the current and previous release changes.
- `Quit` logs you out and then closes the program.

#### Windows menu

The Windows menu allows you to show (tick) or hide (untick) the `Scene`, `Scene Explorer`, `Settings`, and `Convert` windows in the Euclideon Vault Client application.

![Euclideon Vault Client Windows Menu](./images/windowsmenu.png "Euclideon Vault Client Windows Menu")

>TIP: If you notice missing functionality, be sure to enable the required screen panels.

#### Projects menu
Use the projects menu to manage project viewing.

![Projects menu](./images/projectdropdown.png "Projects menu")

- **New Scene** will remove all items from your Scene and create a new empty Scene.
- **Export**  will export the project information, including links to models and other features, to a GeoJSON (JavaScript Object Notation) file in a user-specified directory. The directory must exist.
- **Import** will enable you to import a project in UDP or GeoJSON (JavaScript Object Notation) format to the Scene.
- **Projects** list of available projects on your Euclideon Vault Server.

>TIP: You can also **drag and drop** a UDP or GeoJSON file onto the Euclideon Vault Client interface to load a project.

#### Loading a sample project

Before we cover the remaining interface parameters, it makes sense to load a project, so you have a scene to work
with while you familiarise yourself with Euclideon Vault Client. 

If you are a subscriber to Euclideon Earth, Euclideon provides sample projects for application evaluation. Otherwise, you will have to load a model of your own. Refer to [**Scene Explorer**](#scene-explorer) for instructions on how to load other models.

| Step | Action |
| :--- | :---------------------------- |
|**1.** |To load a sample project, select **Projects** from the top menu. <br />|
|**2.**	|Click a **Sample Project** from the dropdown list. <br />![Euclideon Vault Client project selection](./images/sample-projects.png "Euclideon Vault Client project selection") <br />|
|**3.**|The sample project should load. <br />![Sample project: Gosse Bluff, Australia](./images/loaded-sample-project.png "Sample project: Gosse Bluff, Australia") |


### SCENE VIEWPORT
Now that you have an Euclideon sample project or one of your own models loaded if you had one available, we can familiarise ourselves with the scene viewport. 

#### Moving around the viewport
These settings are configurable in [**Input & Controls**](#input-and-controls). See section [**Settings**](#settings).

##### Default Mouse Controls:


| Action | Description |
| :--- | :--------------------------- |
| **Tumble** | Hold down the `left-mouse` button and move the mouse left-right-up-down to rotate within the display and change the angle of your view. You will not travel with the tumble action. |
| **Pan** | Holding the `right mouse` button on a point in the scene (not the skybox) will begin "panning" (moving the camera, but not turning the camera. It will keep the originally hovered point _under the mouse cursor_. |
| **Orbit** | Hold down the `mouse scroll wheel` as you move the mouse. The mouse movement will give you the sensation that you are orbiting around the point you selected. This feature is based on where you first click on the screen and will keep the camera the same distance from the clicked point by turning and moving the camera. |
| **Dolly** | The `mouse scroll wheel` will "dolly" (zoom the camera in and out) from the point where the mouse is hovering (will not work with the skybox). |

 
>TIP: If you prefer the Euclideon Geoverse MDM method of using the **Scroll Wheel** to change the mode speed, then that option is available in [**Settings**](#settings) --> [**Input & Controls**](#input-and-controls) --> **Mouse Pivot bindings.**

![Mouse Controls](./images/mousecontrols.png "Mouse Controls")


##### Default Keyboard Controls

You can use specific keys, with the "Scene" window focused, to control the camera movement.  

| Action | Description |
| :--- | :--------------------------- |
|**W** and **S** |manoeuvre the camera forward and backward at the current Camera Move Speed.|
|**A** and **D** |pan the camera left and right at the current Camera Move Speed.|
|**R** and **F** |ascend and descend the camera at the current Camera Move Speed.|
|**Spacebar** |locks altitude, allowing you to pilot the camera without changing the camera's height (Z-axis lock).|

>TIP: Hold the `Ctrl` key while pressing `W`, `S`, `A`, `D`, `R`, or `F` keys to dramatically slow the camera movement.<br />
>Hold the `Shift` key while pressing the `W`, `S`, `A`, `D`, `R`, or `F` keys to speed up the camera movement.

##### Map Mode Controls:
-	Holding down any mouse button (left-wheel-right) and moving in any direction will pan the scene in that direction on a flat plane.
-	Spinning the 'mouse scroll wheel' will zoom in or out
-	Keys `W`, `S`, `A`, `D` will navigate the camera up / down / left / right respectively.
-   Keys `F` and `R` will zoom in and out respectively.
-	`Ctrl+M` will toggle map mode

>TIP: Hold the `Ctrl` key while pressing `W`, `S`, `A`, `D`, `R`, or `F` keys to dramatically slow the camera movement.<br />
>TIP: Hold the `Shift` key while pressing the `W`, `S`, `A`, `D`, `R`, or `F` keys to speed up the camera movement.

Rather than holding down the `Shift` key to increase the camera's navigation speed, you can adjust the `Move Speed` blue bar in the [Scene Info and Controls panel](#scene-info-and-controls). This setting is persistent across sessions. 

![Camera movement speed control](./images/camera-movement-speed-control.png "Camera movement speed control")

>TIP: In addition to mouse/keyboard controls, the camera can also be moved using an Xbox Controller or equivalent gamepad/controller device.

##### Xbox controller movements:

These controls might be mapped differently on other controllers.

- `Left Analog Stick` - Move
- `Right Analog Stick` - Rotate Camera (tumble)
- `Dpad` (directional arrows) - Navigation (or maybe panning?)
- `Left Trigger` - Increase Move Speed (sprint)
- `Right Trigger` - Orbit (with crosshair)
- `A Button` - Toggle Lock Altitude
- `Y Button` - Toggle Map Mode
- `Start Button` - Toggle Fullscreen Mode


##### Hotkeys
Use the following keys anywhere in the active Euclideon Vault Client interface to effect the relevant change:  

- `F5` Toggles presentation mode for the current Scene, hiding all other panels and setting the scene view to fullscreen.
- `Ctrl+U` Opens a popup for loading UDS files. The Scene Explorer window must be opened.
- `Ctrl+M` Changes to / from Map Mode


##### Scene Explorer Hotkeys
- `Delete` Removes the selected item(s) from the scene.


### SCENE EXPLORER
The Scene Explorer window lists the assets currently in your scene. The [Quick Action menu](#quick-action-menu) allows you to:

- Models and metadata
- Quick Action Menu
  - add UDS models
  - add new folders
  - save the current camera view
  - add other (live feed)
  - remove selected items from the scene explorer.

>TIP: If you cannot see the **Scene Explorer** panel, ensure it is enabled under the [Windows](#windows-menu) menu.

#### Models and metadata

Underneath the quick action menu, you will find the contents of your scene, which can be one or multiple UDS files. Click the arrow next to the uds file name to view the metadata.

The checkbox to the left of each item is a visibility checkbox. Disabling this hides the element in the scene.

![Scene contents](./images/sceneexplorer2.png "Scene contents")


> TIP: disabling visibility for a folder will also disable visibility for _all_ items in that folder- including subfolders and their contents.

#### Quick Action Menu

The buttons across the top of the Scene Explorer allow quick access to add or remove from the scene. 

![Scene Explorer](./images/sceneexplorer.png "Scene Explorer")


| Button | Description |
| :----- | :----------------------- |
|**Add UDS**  | ![Add uds](./images/uds-button.png "Add UDS button") This button opens the dialog box to add a UDS model to the scene. <br />The Path/URL can include files on the local drives, network drives, UNC Paths, HTTP, HTTPS, FTP & FTPS. <br />(hotkey `Ctrl + U`) ![](./images/add-uds.png "Add a UDS file") <br />|
|**Add Folder**  | ![](./images/add-folder.png "Add folder") Adds a folder to the scene explorer. This can help with organizing your scene. <br />|
|**Save Current Camera** | ![](./images/save-camera.png "Save Current Camera") Saves the current camera position and rotation as a viewpoint. <br />![](./images/save-current-camera.png "Save current camera")
| **Add Other** | ![](./images/add-other.png "Add Other button") Enables you to add `A Live Feed` |
| **Delete**  | ![](./images/delete-button.png "Delete button") This deletes all selected items from the scene <br />(hotkey `Delete`)  |


> TIP: If you ever want to quickly clear the current scene, the "Projects" menu has a "New Scene" option that will remove everything from the scene.


##### Selecting Items

-	Left-click on an item in the Scene Explorer to select it. Selected items will appear highlighted.

-	You can select multiple items in the scene explorer by holding `Ctrl` and then left-clicking them. Doing this on a selected item will deselect it.

-	Single clicking without `Ctrl` will deselect all items and select just the item you have clicked.

##### Reorganising the scene

Items or groups of items in the scene explorer can be reordered by holding left-click and dragging them around. A yellow line indicates where the item(s) will be after you release them.

> TIP: When dropping onto folders. It is best to open the folder you wish to drag into before starting the click-drag operation.


#### Loading UDS Models
There are a number of ways to add models to the scene.

1. **Drag and Drop:** On devices with folder exploring you can drag a file from your file explorer and drop it in the Euclideon Vault Client window to add the model to the scene.
2. **Direct URL loading:** You can type a URL or path in the `Add UDS` popup. ![Add UDS button](./images/uds-button.png "Add UDS button") The path field at the top of the pane allows URL and network paths to retrieve UDS files.

#### Scene Item Properties

##### UDS Point Cloud

The expanded properties for UDS files show the path the UDS was loaded from and then a treeview of the internal metadata from the file.

![Scene Properties](./images/sceneproperties_uds.png "Scene Properties")

The system displays advanced information like the attributes in the file, their sizes, and how they are blended for level of detail calculations.

##### Points Of Interest (POI)

You can add points of interest (POI) to the current scene, alter the name of the POI, and alter the colour to distinguish each POI.


| Step   | Action |
| :---   | :------------------ |
|**1.** | Right-click on a point of interest in the scene. |
|**2.** | Select Add New Item. |
|**3.** | Select Add POI here. |
|**4.** | Repeat the steps for multiple points of interest.  |


![Adding points of interest](./images/points-of-interest.png "Adding points of interest")  
 
You can also track multiple points within an individual point of interest that extends over a distance, such as a train line.


| Step   | Action |
| :---   | :----------------- |
|**1.**  | Highlight the Point of Interest that appeared in the Scene Explorer after you created the point of interest. |
|**2.**  | In the scene viewport, right-click on another point along the point of interest and select `Add Node Here`.|
|**3.**  | Repeat step 2 for each point you want to add to that `Point of Interest`. |
|**4.**  | We recommend setting the name of the `POI` to be representative of what it is marking as this is the name that will display on the scene. In the example, we know the POI is marking a Powerline.  <br /><br />![](./images/sceneproperties-poi-name.png "Point of Interest properties") <br />|
|**5.**  | The size and colour of the labels in the scene are controlled by `Text Colour`, `Background Colour`, and `Label Size`. |
|**6.**  | In the `Line Settings` there are a number of controls for the look and feel of the line (if there are multiple points in the point of interest): <br />`Show Length` and `Show Area` will show those details on the label in the scene <br />`Close Polygon` causes the line to loop from the last point back to the first one. This setting is also required if you want to see the area. <br />Alter the `line colour`, `line width`, `line style`, and `line orientation` to match your requirements. <br />![](./images/scene-properties-poi-line-settings.png "Point of Interest properties") |
|**7.**  |`Selected Point` allows manipulation of a single point in an individual point of interest. <br />`-1` indicates you're modifying all of the points when using the Gizmo. <br />Slide the blue bar to access individual points. <br />![Point of Interest selected point](./images/scene-properties-poi-selected-points.png "Point of Interest selected point")| 
|**8.**  | The `Fly Camera Through All Points` button will begin a fly-through of all points along the line in order. <br /><br />  The fly-through speed is determined by the Camera Move Speed. If the `Close Polygon` line setting is checked, the fly-through will repeat until cancelled. <br />_If you mark a POI below ground- or object-level, then the camera will fly through the ground or objects in its path._ |
|**9.**  | If you added an incorrect point (node), move the `Selected Point` blue bar until you find the point, then select `Remove Node`. <br />![Remove a point](./images/remove-node.png "Remove a point") <br />|
| **10.**| To easily locate a POI in the Scene, right-click on a `Point of Interest` and select `Move To` to reposition the camera at that point. <br />![Move to POI](./images/move-to-poi.png "Move to POI") <br />|

>TIP: For image POIs, you can change its type (standard, panorama, photosphere) in the `Image Type` drop down.

>![Image type for images loaded on to Scene](./images/image-types.png "Image type for images loaded on to Scene")  

##### Live Feeds (IOT)  

Live Feeds (IOT) is intended for use by advanced users as it contains features that can cause performance loss if not configured correctly.

![Live Feed Settings](./images/sceneproperties_livefeed.png "Live Feed Settings")  

|Option | Description |
| :------ |  :----------- |
|**Update Frequency** | The number of seconds to wait after completing an update before doing another update. |
| **Oldest display time** | Used to hide live feed assets that haven't been updated in a while. |
| **Maximum distance to display** | An override to assist with performance. If feeds are causing performance issues, this should be reduced. |
| **Enable Tweening / Position Smoothing** | Allows the positions and orientations of moving feed items to be calculated. |
| **Update Mode** <br />| Changes how the feeds are updated: <br /><br />`Group Mode` updates all feeds in a group on the server. _THIS IS THE RECOMMENDED MODE_. While in this mode, you can enter the Group ID as provided by the Euclideon Vault Server in the `Group ID` input box. A future version of Euclideon Vault Client will change this box to a drop down with available groups.<br /><br />`Position Mode` updates all feeds around a specified point with the radius specified in `Maximum distance to display`. When this mode is active, you can use the Gizmo to move the centre point to the desired location.<br /><br />`Camera Mode` updates all feeds in the `Maxmimum distance to display`. This mode is unable to run optimisations based on time and position as the camera moves. As such, we do ***not*** recommend using this mode. |

> TIP: Show Diagnostic Information will give a lot more information in the UI about how many items are currently being tracked as well as how long until the next update


### SETTINGS
Use the parameters in the **Settings** window to customise how the Euclideon Vault Client looks and operates.  


![Settings panel](./images/settings.png "Settings panel")


-	Alter the [**Appearance**](#appearance) of the interface  
-   Change the  [**input and mouse control**](#input-and-controls) settings  
-   Change the  [**viewport camera**](#viewport) values   
-   Manipulate the [**maps & elevation**](#maps-and-elevation) settings  
-   Change the [**visualisation**](#visualisation) through colours and contours.  

We recommend familarising yourself with each setting by altering the parameters individually to see how they affect the display and operation.  

>TIP: To restore all default values for any of these settings, simply right-click on the sub-heading and then select `Restore Defaults`.

#### Appearance

![Appearance Settings](./images/appearance.png "Appearance Settings")

The Appearance values enable you to adjust several settings that change the appearance of the Euclideon Vault Client interface.

|Option | Description |
| :------ |  :------------ |
| **Theme** | Choose between a Light or Dark interface colour scheme. |
| **POI Visibility Distance** | Use the blue bar slider to change the maximum distance at which Points of Interest (includes AOI and LOI) are visible.  <br /><br />If this value is low, you won't be able to see the POIs when zoomed out.  <br /><br />Rather than zooming in and traversing the scene with the mouse to search for `Points of Interest`, right-click on the POI in the Scene Explorer and select `Move To` to relocate to that POI.<br /><br />  Labelling POIs to distinguish them is good practice.
|**Show Diagnostic Information** | Check the box to display diagnostic information, such as the frame rate.  <br /><br />The system will display the frame rate in frames per second (FPS) and milliseconds (ms) on the top menu bar next to the license and username information.  <br /><br />Diagnostic information for `Live Feeds` includes `Total Cached Items, Number of items displayed, Next Update`.
| **Show Advanced GIS Settings** | Check to display advanced GIS settings in the top-right corner. |
| **Limit FPS in Background** | Enable this to limit the FPS (frame rate) when the client is running in the background, reducing interference with other open programs. |
| **Show Compass On Screen** | Check to toggle the display of the compass in the bottom-right corner. |
| **Presentation UI** | By default the [Scene Info and Camera Controls](#scene-info-and-controls) are not visible in Presentation or Fullscreen mode (hotkey `F5`). Change this behaviour by ticking this checkbox. |
| **Mouse Anchor Style** | Change the appearance of the mouse when anchoring to models or map tiles:  <br /><br />  Orbit  ![Orbit](./images/orbit.png "Orbit") <br /><br />  Compass  ![Compass](./images/compass.png "Compass") or Nothing  |
| **Voxel Shape** | Set the shape of voxels in the Scene window as either points, rectangles, or cubes. Euclideon recommends rectangles for accuracy and performance. |

>TIP: For settings that are controlled via a slider, you can press Ctrl+click on the slider and manually enter a value, allowing you to enter a more precise value or, in some cases, extend the slider value beyond its min and max range.

#### Input and Controls

![Input Settings](./images/inputcontrol.png "Input Settings")

Expanding this panel allows you to change settings that affect how you interface with the Euclideon Vault Client. Please note, Input & Controls parameters do not work in `Map Mode` (Ctrl+M).:

  - **On Screen Controls:** 
    - You can toggle on-screen mouse controls, which will appear at the bottom-left of the Scene window. 
    - Click and drag the blue bar in the U/D box to move the Scene `up` or `down`.
	- Click inside the `Move Camera` box and drag the mouse out. You will see a line appear from within the box. The camera position and rotation is controlled by how far you draw out and in which direction you drag this line.
	
>TIP: Hold down the Shift-key to speed up the camera movement.
	
![On-screen controls](./images/on-screen-controls.png "On-screen controls")

  - **Touch Friendly UI:** You can enable Euclideon Vault Client to recognise touchscreen devices.
  - **Invert X-axis and Invert Y-axis:** Check to invert the default camera movement when you drag the mouse along the X and Y axes.
  - **Mouse Pivot Bindings:** You can customise the mouse-controlled camera movements. Find the descriptions of the different mouse pivot bindings in the [**Default Mouse Controls**](#default-mouse-controls) section.


#### Viewport

The Viewport allows you to adjust the camera's viewing distance and field of view (fov) of the camera lens. These settings affect your viewing range within the **Scene Window**.

Please note that Viewport parameters do not work in `Map Mode` (Ctrl+M).

![Viewport settings](./images/viewport.png "Viewport settings")

  > TIP: If the model disappears from the scene, yet you can still see the sky, watermark, or a (list of) uds file(s) in the Scene Explorer, your View Distance might be set too low. Adjust the View Distance settings until your models are visible again.


| Parameter  | Default Value| Description  |
| :------ | :------ | :--------------------- |
| **View Distance**     | 10000.000m  | The distance that the camera's view extends. Engage in the following [exercises](#view-distance-exercises) to illustrate how this value works.   |
| **Camera Lens (fov)** | 30mm | The Camera Lens (fov) operates like standard cameras. Use the field of view (fov) to alter the visual angle of the camera lens. <br />Select from the values in the drop down box. The shorter focal length (i.e. 15mm) gives a wider field of view than a larger length (i.e. 70mm).     

##### View Distance Exercises  
Load a model or project and use the following exercises to help familiarise yourself with the `View Distance` and `Camera Lens (fov)` options.

**Exercise 1**

| Step | Action |
| :--- | :--------------------------- |
|**1.**	|Set the angle of the camera so it looks as though you are flying close to ground level. |
|**2.**	|Reduce the value of `View Distance` until only a small part of the scene is visible. |
|**3.**	|Click a point in the distance and zoom in using the mouse scroll button or the `W` key and watch how the scene unfolds: objects in the distance materialise while those in the foreground disapear. |
|**4.** |Repeat these steps while adjusting the `View Distance` values.|

**Exercise 2**

| Step | Action |
| :--- | :--------------------------- |
|**1.**	|Set the `View Distance` to its maximum value.|
|**2.**	|Zoom out until the entire scene is visible.|
|**3.**	|Slowly drag the `View Distance` blue bar towards the minimum setting and watch the scene peel away.|
|**4.**	|Before the scene disappears entirely, zoom in with your `mouse scroll button` or the `W` key and watch the scene materialise.|
|**5.**	|When you can see a large portion of the scene, reduce the `View Distance` blue bar to reduce the camera's viewing distance.|


#### Maps and Elevation

The map feature is useful when you need a geospatial view and are unfamiliar with the area or are looking for a specific reference point. Select **Map &amp; Elevation** within the **Settings** tab. 
 
![Maps and Elevation settings](./images/mapsandelevation.png "Maps and Elevation settings")


 > TIP: Hold down the Ctrl-key and click in any parameter that has a slide bar to manually set a more precise value or set a value outside the parameter boundaries.
 

| Parameter  | Default Value | Description |
| :--- | :--- | :----------------------- |
| **Map Tiles**  | checked | As long as the 3D models are geospatially correct, suitable map tiles will generate automatically. If you uncheck the box, the map tiles (street map) below the model (and all other customisable options in Maps &amp; Elevation) will disappear.   |
|**Mouse can lock to maps** | checked | Check this box to enable the mouse to lock to map tiles when moving the camera. It also allows you to add interest points, measurements, and lines to the map tile, and execute other right-click functionality, such as setting the map height to the current mouse position.|
| **Tile Server** | | Tile Server allows overlay with existing maps, clicking `Tile Server` prompts you to enter a server address for retrieving background map tiles.  <br /><br />![Tile Server](./images/tileserver.png "Tile Server selection")|
| **Map Height**  | 0.000m                | If you adjust Map Height, the map tile will move higher or lower according to your selection. The higher the value, the  more the map will cover the model. Couple with the overlay option to see where map markers cross the terrain image. |
| **Blending**   | Hybrid  | Choose from three options: <br />` Hybrid`  Hybrid-map will move above and below the model as you adjust the map height. <br />`Overlay` Overlay-map will stay above the terrain image as you adjust the map height. <br />`Underlay` Underlay-the map will stay below the terrain image as you adjust the map height. |
| **Opacity**   | 1   | The transparency (opacity) slider adjusts the transparency (opacity) of the map tiles so they don't obscure the visibility of objects in the scene. Move the blue bar to adjust the transparency of the map enabling you to view street markers over the terrain map. <br />![Map overlays the model](./images/map-overlay-model.png "Map overlays the model")|
| **Set to Camera Height**  | N/A  | The `Set to Camera Height` button can be used to set the height of the map tiles to the camera's current height. This can be used to place map tiles outside the default range of the slider of +/-1000m. |


#### Visualisation

The Visualisation panel allows you to change between Colour, Intensity, and Classification display modes. 

**Colour** is the default display mode.

![Visualisation Settings](./images/visualizationsettings.png "Visualisation Settings")

**Intensity** display mode: Adjust the `Min & Max Intensity` values to alter the intensity of the scene.

![Visualisation Intensity settings](./images/visualizationintensity.png "Visualisation Intensity settings")

In **Classification** display mode, a checkbox appears which enables you to customise the colours of objects corresponding to their designated classifications.

![Visualisation Classification](./images/visualizationclassification.png "Visualisation Classification")

Couple the settings below with `Voxel Shape` under the [Appearance](#appearance) option in the [Settings](#settings) window to see how the voxel shape affects these settings.

| Visualisation Setting | Description |
| :----- | :------------ |
| **Enable Edge Highlighting** | Highlights the edges of every voxel in the scene, using the specified width and colour. The threshold determines how to resolve the edges of overlapping voxels. |
|**Enable Colour by Height** | Displays colours along the specified two-colour gradient to all objects in the scene based on their height. |
| **Enable Colour by Depth** | Displays colours along a one-colour gradient to all objects in the scene based on their distance from the camera. |
|**Enable Contours** | Displays elevation contours depicted by`Contour Colour` on all objects.<br />`Contours Band Height` is the width of the contours<br />`Contours Distances` is the vertical space between each contour. |

All four display options also allow you to customise the colours:

![Visualisation Settings Expanded](./images/visualizationall.png "Visualisation Settings Expanded")

> TIP: If you changed a setting and can't recall what you've changed, you can reset all settings by going to the **System** menu and selecting **Restore Defaults**, or you can reset individual groups of settings by right clicking on their header in the **Settings Window** and selecting **Restore Defaults**.

>TIP: All parameters in the Settings Window are persistent across sessions: If you log out and log back in, the system will restore your settings from the previous session.



### SCENE INFO AND CONTROLS

![Camera information and controls](./images/scene-info-and-controls.png "Camera information and controls")

Use the controls on this panel to manipulate the camera and control the scene.


| Label | Name | Hotkey | Description |
| :--- | :------ | :------ | :------------------------- |
|  **1.** |`Lock Altitude` |`Spacebar` |  Will keep the camera's height constant (Z-axis lock) when panning with the mouse or strafing with the keyboard. This function is the same as enabling "Helicopter" mode in Geoverse MDM. |
|  **2.** |`Show Camera Information` | |  2a. camera position <br /> 2b. rotation coordinates <br /> 2c. camera move speed slider. |
| **3.** |`Show Projection Information`| | Displays SRID information in the top-right corner. |
| **4.** |`Gizmo Translate` | `B`| Sets the gizmo to translation mode, allowing you to move objects. |  
| **5.** |`Gizmo Rotate` | `N` | Sets the gizmo to rotation mode, allowing you to rotate objects. |
| **6.** |`Gizmo Scale` | `M`| Sets the gizmo to scaling mode, allowing you to resize objects. |
| **7.** |`Gizmo Local Space` |`C` | toggles the gizmo's operational coordinate system between local coordinates (relative to the object) and world coordinates (relative to the world space). |   
| **8.** |`Fullscreen` | `F5`| Toggles presentation mode and sets the client to fullscreen. Fullscreen mode hides everything except the Scene window&mdash;even the `Advanced GIS Settings` if you have that option checked in [Appearance](#appearance) under [Settings](#settings). |
| **9.** | `Map Mode` |`Ctrl+M` | Toggles map display mode, which displays the scene top down and orthographically. <br />  Entering and exiting map mode will attempt to keep the camera at the same height, so what you see remains consistent. |
| **10.** | `Lat, Lon, Alt` | | This area also displays the Latitude, Longitude and Altitude of the camera's current position, if the camera is projected in a geospatial zone. |


#### GIZMOS: Moving items in the scene

You can move objects in Euclideon Vault Client Scene through the use of a transformation tool called a "gizmo". When active, the gizmo appears at the origin of an item.

Gizmos are disabled by default. To activate the gizmo tool, do the following:  
- Select one or more items in the the Scene Explorer window (see [Selecting Items](#selecting-items)).  
- Select a gizmo tool from the Euclideon Vault Client interface (see [Scene Info And Controls](#scene-info-and-controls))


![Scene Gizmos](./images/gizmos-labelled.png "Scene Gizmos")

In **Local Space Mode**, the axis will align with the local axis of the last selected item. If the model doesn't have a local space axis, then the gizmo will use the global axis. ![](./images/local-space-mode.png "Local Space Mode")
   
- **Red** is the X axis, which in projection space usually corresponds to the _EASTING_.  
- **Green** is the Y axis, which usually corresponds to the _NORTHING_.  
- **Blue** is the Z axis, which usually corresponds to the _ALTITUDE_.  

Looking at the gizmo from different angles will cause one or more of its axes to appear hatched/dashed. This indicates that this axis is pointed in the negative direction. 

![Negative angled gizmo](./images/gizmo-negative-axis.png "Negative angled gizmo")

Use the **Translation Gizmo** to move scene item(s) around. The translation gizmo has 3 components:  

1.	The coloured axis arms of the gizmo will translate only along that axis.
2.  The coloured squares between two axis arms will translate in that plane. For example, the square between the X and Y axes translates only in the XY plane. The colour indicates which axis won't be modified by using that square.  
3.  The white circle at the origin of the gizmo will translate the models in the plane perpendicular to the camera.  


The **Rotation Gizmo** changes the orientation of the item(s). Items will rotate around their pivot point, which is ideally the centre of the mass. For UDS models, a pivot point is selected automatically during conversion. Non-UDS models can designate a different pivot point. The rotation gizmo has 2 components:  

1.	The coloured rings will each rotate around the axis of that colour (e.g. the Z-axis is blue, so the blue ring rotates everything around the Z-axis).  
2.  The white ring rotates everything around the axis parallel to the direction of the camera.  

The **Scale Gizmo** changes the size of the selected item(s). The anchor for scaling is always the centre of the item(s). The scale gizmo has 2 components:  

1.	The coloured axis arms of the gizmo can be used to scale along that axis only.  
2.  The white circle at the origin of the gizmo will scale uniformly in all directions.  

In some situations, you may find parts of the gizmo greyed out. This will happen when the model does not support that action. For instance, UDS files do not support non-uniform scaling. Therefore, that gizmo will be unavailable.

![Scene gizmos greyed out](./images/gizmogreyed.png "Scene gizmos greyed out")

>TIP: If you have moved, rotated, or scaled a uds model incorrectly and cannot recall its original settings, right-click on the item in the `Scene Explorer` and select `Reset Position`.

![Reset gizmo position](./images/reset-gizmo-position.png "Reset gizmo position")



### COPYRIGHT AND COMPASS
Copyright data will be displayed in the bottom-right corner of the viewport. You can add Copyright Data to new models in the metadata textbox during conversion.

A compass is also displayed in this corner, indicating the camera's current orientation within the global coordinate space. Compass by default is on, but the Appearance pane contains an on/off checkbox to toggle this setting (see [**Appearance**](#appearance) in section [**Settings**](#settings)).



### WATERMARK
If a watermark was included in the model, it will appear at the bottom left-hand corner of the Scene window.

![Watermark image on the scene window](./images/watermark-image.png "Watermark image on the scene window")

>TIP: If you have multiple models loaded, all of which have a watermark, if they are separated geographically, then when you `Move To` or use the mouse or keyboard to navigate to the model, its watermark will display. However, if the models overlay each other, then the watermark of the model listed closer to the top in the Scene Explorer window will display.

### CONVERT

#### Converting in Euclideon Vault Client
The Euclideon Vault Client enables users to import 3D models and convert them to Euclideon's Unlimited Detail format (UDS). Euclideon's UDS format enables you to stream and load massive point cloud data sets. Euclideon Vault currently supports the following file types:

-   LAS
-   PTS
-   PTXS
-   TXT/CSV
-   E57
-   XYZ
-   OBJ
-   ASC (Esri)

![Euclideon Vault Convert Window](./images/convert-parameters.png "Euclideon Vault Convert Window")  

>TIP: If you attempt to drag and drop the file to want to convert on to the Convert Window before typing in the `Output Name` path and filename, then Euclideon Vault Client will report that the file type is not supported.


| Step | Action | Comments|
| :--- | :----------- | :------------------------ |
| **1.** | Select the **Convert** tab in the Euclideon Vault Client interface | If the convert table is not visible, select **Convert** from the Windows drop down menu in the top left-hand corner of the Euclideon Vault Client interface. |
| **2.** | Type in the **Output Name**&mdash;path and name&mdash;of the exported UDS file. | Ideally, this parameter should be the final target location of the file so that you don\'t need to copy the file at the end. The system will add the file extension for you. Make sure there is enough space in the target directory to store the entire converted file. |
| **3.** | **Type in the Temp Directory** | Defaults to the same location as the output file in the previous step. <br /><br />Type C:\\Vault\\convert in the Output Name field and the Temp Directory will default to C:\\Vault\\convert_temp. Euclideon recommends setting the Temp Directory location to one that resides on a high-speed local drive with plenty of space (***rule of thumb:*** set aside as much space as the uncompressed raw input, as the points in the temporary files have not as yet been compressed), as the convert process will write many small, temporary files. <br /><br />  The conversion will remove the temp directory after completion. |
| **4.** | Optional Selection: **Continue processing after corrupt/incomplete data (where possible)**. | If corrupt or incomplete data is detected during a conversion, this setting will decide whether the conversion is **cancelled (unticked) or continues (ticked)** resulting in a skip of the corrupted points. NOTE: If you tick this setting, the system will not report that it found corrupt or incomplete data. |
| **5.** | Optional Selection: **Point resolution:0.00000&mdash;Override** | Set to the source size, or it will default to 0.01 if no source size configured. This setting determines the size of the points (in metres) in the point cloud after conversion. It will attempt to find a good size based on the source data but can be set manually by first ticking the \"Override\" checkbox and then typing a value in the text field. |
| **6.** | Optional Selection: **Override Geolocation** | If the source data has been correctly geolocated, when you import that file prior to converting it, the file should already have the **"Spatial Reference Identifier" (SRID)** information filled in this box: search for "EPSG code" or "SRID code" in the metadata. <br />The **SRID** is the Geotagged ID of the GIS zone for the exported model. <br />If the pre-parse can detect the SRID it will be set automatically. If not, and you wish to correctly geolocate your data, then you can manually select the \"Override\" checkbox and enter the correct SRID in the text field. It assumes that the input is already in the correct zone. <br />**Global Point Offset** can be used to add an offset to the x, y and z coordinates of the converted model from that which was specified in the file's metadata, or if not, from the global origin (0, 0, 0). |
| **7.** | **Quick Partial Convert** | Enabling this option will greatly speed-up the conversion process by only processing 1 in every 1000 points of the input data. This is useful for testing and calibrating the conversion settings before attempting to convert a large and time-consuming job. <br />When imported into the scene the converted model will appear disintegrated and will have 1/1000th of the intended resolution. |
| **8.** | **Metadata** | This section allows you to import watermark by drag and dropping, add metadata related information such as Author, Comments, Copyright and License holders. |
| **9.** | **Choose a file to convert** | Drag and drop the file from Windows Explorer onto the **Convert** window. <br />![](./images/drag-to-convert.png) <br />|
| **10.** | Look at the **Input Files** panel. | Once you have selected your file for conversion, a new panel appears listing your selected files. The estimated number of points in each file will be shown. During conversion, the progress for each file will also be shown. Click **Remove** or **Remove All Files** if the wrong file\(s\) was selected. | 
| **11.** | Click **Begin Convert** | Euclideon Vault will read the file then process and write the points. <br />![](./images/conversion-in-progress.png "Conversion in progress") <br />|
| **12.** | **X** to Cancel | The "X" button beside the convert job in the "Convert Jobs" section allows you to cancel a running convert (it will cancel at the next 'safe' point and clean up temporary files). Once cancelled, the 'Begin Convert' button and the configuration options will reappear enabling you to restart the conversion. <br />After a job has completed the "X" button also allows you to remove it from the list. |
| **13.**| Click **Add to Scene** | When the conversion has completed successfully, click the **Add to Scene** button located at the top of the window. Euclideon Vault Client interface will automatically switch to the **Scene Window** to display your 3D model. <br />*NOTE: Requires a Render License.* <br />![](./images/add-to-scene.png "Add to Scene") |
| **14.**| Review your 3D Model | If Euclideon Vault Client interface does not switch automatically to the **Scene Window**, then select **Scene** from the Windows menu at the top of the Euclideon Vault Client interface. <br />*NOTE: Requires a Render License.*|


> TIP: Euclideon does not recommend running multiple converts at the same time. Converting is a memory and processor intensive process so it's almost always faster to have 1 convert running at a time. Euclideon Vault Client helps with this by allowing you to queue multiple jobs to run one after another.


#### Converting from Command Line (CMD)
The Euclideon Vault Client also comes with a separate command line application which allows you to convert files via the command line instead of using the [**Convert Tab**](#converting-in-euclideon-vault-client).


| Command or Option | Action |
| :--- | :-------------- |
|  **vaultConvertCMD.exe** |Open a command prompt and navigate to the folder containing the executable file **vaultConvertCMD.exe** <br />Alternatively, enter the full path of this executable. Remember to put quotes around paths that contain spaces.  |
| **Syntax** | `vaultConvertCMD server username password [options] -i inputFile [-i anotherInputFile] -o outputFile.uds` |
| **server** | The name and location of your Euclideon Vault Server |
| **username** <br /> **password** | Your account credentials for connecting to `server`. <br />These will be the same as those used for the Euclideon Vault Client login screen (See [**Logging In**](#logging-in)). |
| **[options]** | Optional arguments for customising the conversion: <br />**-resolution <res>** Override the resolution (0.01 = 1cm, 0.001 = 1mm) <br />**-srid <sridCode>** Override the srid code for geolocation <br />**-pause** Requires the enter key to be pressed before exiting <br />**-pauseOnError** If an error occurs, require the enter key to be pressed before exiting <br />|
|**-i inputFile**  | The location of the file you wish to convert. If the file's path contains any spaces, you need to put the path in quotes, for example: `"C:/My Data/File to Convert.csv"` <br />Use multiple `-i InputFile` options to specify additional input files. **NOTE:** The conversion process will merge all files into the one output file, so ensure they are compatible first. |
|  **-o outputFile.uds** | The name and location of the new `.uds` file you wish to create. Again, if the file's path contains any spaces you must put the path in quotes, for example: `"C:/Output/Converted File.uds"` |

![Converting models from command line](./images/convert-cmd.png "Converting models from command line")
   

### Rearranging Windows

You can rearrange the windows in the Euclideon Vault Client interface to suit your preferences.

| Step | Action |
| :--- | :--------------------------- |
| **1.**   | Press and hold the left-mouse button on the tab of the window you want to move. |
| **2.**   | As you drag the window, it should pull away from its docked location. |
| **3.**   | Keep dragging and you will notice transparent blue rectangles appear in dockable locations on the interface. |
| **4.**   | Drag your window over the blue rectangle situated where you want your window to sit.  <br /><br />For instance, in the screenshot, we are moving the **Scene Window** to the right of the Convert Window to create a 3-panelled interface display. |
| **5.**   | Once in position, simply release the left-mouse button. |

![Rearrange windows in Vault](./images/rearrange-windows.png "Rearrange windows in Vault")


## Hosting a model

You can store 3D models on:

-   A local computer hard-drive
-   A network drive
-   In a Cloud.
    
>Detailed configuration and implementation instructions on how to host models in a cloud is outside the scope of this document. Please, refer to the Euclideon Server and VDK Guides for more information.

## Technical Information

### Requirements

- **VaultClient_OpenGL** requires OpenGL version 3.2 and up-to-date graphics drivers.
- A reliable **Internet Connection** with adequate bandwidth for Euclideon Earth users.
- An **Euclideon Vault License** for convert and render operations.
- Euclideon Vault Client **Login credentials**

If you have not received your Euclideon Vault Client license or login credentials, then please contact Euclideon at <info@euclideon.com> or go to [Euclideon's website](https://euclideon.com) to access our online support.

### Settings On Disk
We mentioned in [**Visualisation**](#visualisation) that your interface settings are persistent across sessions, and this is because Euclideon Vault Client stores that information in a **settings.json** file, the location of which is dependent on your operating system.

- Windows: `%AppData%/Roaming/Euclideon/client/settings.json`
- macOS: `~/Library/Application Support/euclideon/client/settings.json`
- Linux: `~/.local/share/euclideon/client/settings.json`

### Third Party Licenses
Euclideon Vault Client uses a number of third party libraries. Please view the `About` box from the Login page within the application for full license information.

- `Dear ImGui` from [GitHub](https://github.com/ocornut/imgui)
- `ImGuizmo` from [GitHub](https://github.com/CedricGuillemet/ImGuizmo)
- `libSDL2` from [libsdl](https://libsdl.org)
- `GLEW` from [SourceForge](http://glew.sourceforge.net/)
- `Nothings/STB` single header libraries from [GitHub](https://github.com/nothings/stb)
- `easyexif` available at [Euclideon's GitHub](https://github.com/euclideon/easyexif) forked originally from [GitHub](https://github.com/mayanklahiri/easyexif)
- `Autodesk FBX SDK` from [Autodesk FBX SDK Download Page](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-5)

Euclideon Vault Development Kit (VDK) uses the following additional libraries.

- `cURL` from [GitHub](https://github.com/curl/curl)
- `Nothings/STB` single header libraries from [GitHub](https://github.com/nothings/stb)
- `libdeflate` from [GitHub](https://github.com/ebiggers/libdeflate)
- `mbedtls` from [GitHub](https://github.com/ARMmbed/mbedtls)
- `miniz` from [GitHub](https://github.com/richgel999/miniz)

## FAQs

**Why can't I see my image?**  
	Check that you haven't configured the **View Distance** in viewport settings to a small distance. The default minimum is 200 metres, but you can override this value by holding the `Ctrl` key as you click in the View Distance field.
	Ensure the model is enabled: a tick should appear in the tickbox next to the model name in the Scene Explorer window.

**How can I turn the compass off?**  
	Visualisation pane on the right hand side in settings, press the checkbox and you've toggled it off.

**How do I lock altitude when moving the camera?**  
	Press the `Space bar` to toggle, or press the lock altitude button in the 'status' window (top left of the Scene window). See [**Scene Info and Controls**](#scene-info-and-controls) for more information.

**How do I load my previous projects?**  
	Press Projects, and view the list of hosted Euclideon Vault projects from the dropdown box. You can also load a project that you saved to disk by selecting `Import` from the Project menu and browsing to where you saved your \<project\>.json file.

**How do I convert into UDS?** 
	Select the Convert window and type the destination path and name for the converted file in the `Output Name` field (the sytem will add the .uds extension once you click out of that field if you don't enter it). Drag and drop the file you want to convert on to the Convert window. Fill out other fields as required. Refer to [**Convert**](#convert) in this guide for detailed instructions.

**A converting error occured, what do I do?**  
	Due to either corrupt or incomplete data, clicking the 'Continue processing after corrupt/incomplete data (where possible)' tickbox will let the conversion process know that it should attempt to complete the convert process, ignoring data integrity. Euclideon cannot guarantee that the model will convert correctly or that if it does, it will be a useful rendering. 

**I am dragging my file to convert onto the convert window, but nothing is  happening. What do I do?**  
	Confirm that the file type is supported for conversion by Vault.

**I want to demonstrate key features of my 3D model, how can I do that?**  
	Check out the [Visualisation](#visualisation) dropdown box in the Settings pane, on the right hand side of the viewport in Euclideon Vault Client.

**How do I adjust the mouse controls?**  
	View Mouse Pivot bindings in the [input and controls](#input-and-controls) menu in settings.

**I closed the settings window and Scene Explorer how do I display them?**  
	Select [Window](#windows-menu) from the top menu and confirm that the missing window is unticked; an unticked window means that it won't display. If it is unticked, click it with the left-mouse button. The window should display. If it does not, press `Restore Defaults` from the [System menu](#system-menu) to return the Euclideon Vault Client interface to its default settings.

**What is the Name and Light next to my license name mean?**    
	The **name** is the username you used to log into the Euclideon Vault Client interface.<br /> The **light** represents your connection status.  axis.  
   
- **Green** is good and indicates that you are connected and actively working. 
- **Yellow** indicates that the server hasn't been contacted in over 30 seconds. 
- **Red** indicates that the server hasn't been contacted in more than 60 seconds and has been disconnected. Red can also point to hindered services, such as network connectivity issues.  

  Other useful information includes the license type you are using and how recently your license has synced with the Euclideon Vault Server. 

**My 3d object is hidden behind the map. How do I see it?**  
	Changing the Transparency or the Blending in [Maps and Elevation](#maps-and-elevation) may make it easier to see your object, using a combination of these for varied affect.

**I keep getting the "logged out" screen, how do I fix this issue?** 
	Check your internet access and license permissions and try again. For offline use, upgrade your license.

**I cannot log in?**  
	Check your internet connection, your license status, and ensure you have used the correct URL, username, and password. Make sure your URL has a closed bracket at the end of it if using the default server URL.

**I cannot connect to the Euclideon Vault Server, how do I resolve this issue?**  
	Check your firewall or proxy settings. If running a proxy, check with your IT department that the address is correct and that the correct port is set.

**My proxy isn't working, why?**  
	Adopt the proxy format of `protocol://username:password@domain/URI`. Network metadata is not transmitted when using proxies and is stored in plain text file, which may assist solving connection issues.

**Could not open a secure channel. Why is this popping up?**  
	If you are using a proxy, your network may not be sending encrypted data. Ticking the "ignore certificate verification" may circumvent this issue. Note: Network Security certificates will not be verified.

## Contact Us


To learn more about Euclideon Vault and other Euclideon solutions, please email <sales@euclideon.com> or visit us at [https://www.euclideon.com](https://www.euclideon.com/).