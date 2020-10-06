# udStream 1.3 User Guide

- [Getting Started](#getting-started)
- [Registering](#registering)
- [Downloading the udStream software](#downloading-the-udstream-software)
- [Unpacking and Installing](#unpacking-and-installing)
- [Windows](#windows)
- [macOS](#macos)
- [Ubuntu Linux Distributions](#ubuntu-linux-distributions)
- [Logging into udStream](#_Toc38065739)
- [Login Errors](#_Toc41928042)
- [User Interface](#user-interface-1)
- [Scene Viewport](#scene-viewport)
- [Moving around the viewport](#moving-around-the-viewport)
- [Default Mouse Controls](#default-mouse-controls)
- [Default Keyboard Controls](#default-keyboard-controls-1)
- [Default Gamepad Controls](#default-gamepad-controls)
- [Default Hotkeys](#default-hotkeys)
- [Scene Explorer](#scene-explorer)
- [Scene Action Bar](#scene-action-bar)
- [Scene Items](#scene-items)
- [Selecting Items](#selecting-items)
- [Reorganising the scene](#reorganising-the-scene)
- [Loading UDS Models](#loading-uds-models)
- [Scene Item Properties](#scene-item-properties)
- [UDS Point Cloud](#uds-point-cloud)
- [Comparing Models](#comparing-models)
- [Selection Tool](#selection-tool)
- [Voxel Inspector](#voxel-inspector)
- [Annotation Points](#annotation-points)
- [Line Measurements](#line-measurements)
- [Area Measurements](#area-measurements)
- [Height Measurements](#height-measurements)
- [Show Angles](#show-angles)
- [Image POI](#image-poi)
- [Live Feeds (IOT)](#live-feeds-iot)
- [Viewsheds](#viewsheds)
- [Filters](#filters)
- [Settings](#settings)
- [Appearance](#appearance)
- [Input and Controls](#input-and-controls)
- [Key Bindings](#key-bindings)
- [Maps and Elevation](#maps-and-elevation)
- [Visualisation](#visualisation-1)
- [Tools](#tools)
- [Convert Defaults](#convert-defaults)
- [Screenshot](#screenshot)
- [Connection](#connection)
- [Scene Info & Controls](#scene-info-controls)
- [Gizmos](#gizmos)
- [Copyright and Compass](#copyright-and-compass)
- [Watermark](#watermark)
- [Export](#export)
- [Convert](#convert)
- [Converting from Command Line (CMD)](#converting-from-command-line-cmd)
- [Hosting a model](#hosting-a-model)
- [Technical Information](#technical-information)
- [Requirements](#requirements)
- [udStream Login credentials](#udstream-login-credentials)
- [Settings On Disk](#settings-on-disk)
- [Third Party Licenses](#third-party-licenses)
- [FAQs](#faqs)
- [Contact Us](#contact-us)

## Getting Started

### Registering

Before downloading and installing udStream you will need to go to [https://udstream.euclideon.com](https://udstream.euclideon.com) to register an account. When you arrive at the udStream site, click **Register**.

![](media/image1.png)
![](media/image2.png)                                                                                        Next, fill in your name and email address and click **Register.** An email will be sent to you. When you receive the email, follow the instructions to change your password.

------------------------------------------------------------------------------------------------------------------------------------------------------------------------- ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

You will be brought back the udStream website where you will be asked to set a new password. Once this is complete, you will have successfully registered your account.   ![](media/image3.png)

### Downloading the udStream software

![](media/image4.png)

Once you have registered an account, log in to
<https://udstream.euclideon.com>. Here you can download the correct
version of udStream for your operating system, Windows, macOS or Ubuntu.
Alternatively, if you are using Chrome, you can run udStream right in
the browser without having to download or install anything!

Unpacking and Installing
------------------------

### Windows

Locate and run the downloaded udStreamSetup.exe installer. If a window
pops up asking **Do you want to allow this app to make changes to your
device** click **Yes**.

  ![](media/image5.png)               In the next window, be sure to read the license agreement, then click **I Agree.**
  ------------------------------------------------------------------------------------ ------------------------------------------------------------------------------------
  Choose the items you wish to install and click next. By default, all are selected.   ![](media/image6.png)

  ![](media/image7.png)   Choose a destination to install to and click **Install**.
  ----------------------------------------------------------------------------------- ------------------------------------------------------------------------
  Click **Close** once the installation is complete.                                  ![](media/image8.png)

To run udStream, double click on the desktop icon, or press the windows
key, type 'udStream' and click the app under Best match

![](media/image9.png)

### macOS

1.  Locate the udStream DMG package

2.  Open the DMG and drag the "udStream" icon onto the provided
    "Applications" icon.

3.  Open System Preferences, then click "Security & Privacy".

4.  Under the General, tab locate "Allow apps downloaded from" and
    choose Anywhere.

5.  Run the "udStream" application from "Applications".

### Ubuntu Linux Distributions

1.  Using the GUI interface requires a desktop environment.

2.  Use your package manager to install SDL2 (Minimum version 2.0.5)

3.  Locate the udStream tar.gz package that you
    [downloaded](#_bookmark2) previously.

4.  Unpack the entire contents of the tar.gz

5.  Double-click on udStream to start the udStream application.

[]{#_Toc38065739 .anchor}

Logging into udStream
---------------------

After the application has started, it will present the login screen. If
this does not occur, please contact <support@euclideon.com> for
assistance.

![](media/image10.jpg)

After you have entered your credentials, click Login and you should see
an empty scene in the viewport like the image below (actual opening
scene may vary).

![](media/image11.jpg)

[]{#_Toc41928042 .anchor}

### Login Errors

-   Could not connect to server.

    -   There are several possible causes for this message. The most
        common is the Server URL entered into the field is not correct.
        The system is case- and space-sensitive. Ensure there are no
        spaces before or after the Server URL.

-   Username or Password incorrect

    -   This could mean any of the following:

        -   The username or password is incorrect

        -   The username does not exist

        -   The username has been banned

        -   You need to ensure there are no unintentional spaces before
            or after the Username or Password.

-   Your clock does not match the remote server clock.

    -   To maintain system security, the client and server must agree on
        the time to within 5 minutes. Having the server and client both
        set to synchronize with "universal" NTP time is preferable.
        This error will occur if the time zone of either the server or
        the client is not set correctly.

-   Could not open a secure channel to the server.

    -   The client was able to connect to the server provided in the
        server URL field, but an error occurred while verifying the
        server was the intended target or negotiating an encrypted
        connection.

-   Unable to negotiate with the server, please confirm the server
    address.

    -   The client was able to connect to the server provided in the
        server URL field, but the server did not respond as expected.
        This usually occurs if the server is not a udStream Server.

-   Unable to negotiate with proxy server, please confirm the proxy
    server address

    -   This occurs when the proxy information is partially correct.
        Further details may be required before the connection through
        the proxy is correct (usually proxy authentication details).

-   Unknown error occurred, please try again later.

    -   This error was not one of the above errors and will require
        Euclideon Support assistance to resolve. Please contact
        Euclideon at <support@euclideon.com> or go to
        <https://euclideon.com> to access our online support and
        Knowledgebase.

![Lights On](media/image12.png) When an
error occurs, press the Alt and Ctrl keys simultaneously to display an
additional error code after the message. Email <support@euclideon.com>,
giving the udStream version number, error message and code so they can
help you resolve the problem. You might use this in situations where you
get an Unknown Error Occurred message.

User Interface {#user-interface-1}
==============

Scene Viewport
--------------

### Moving around the viewport

These settings are configurable in [Input &
Controls](#input-and-controls). See section [Settings](#settings).

### Default Mouse Controls:

  Action   Description
  -------- ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Tumble   Hold down the left-mouse button and move the mouse left-right-up-down to rotate within the display and change the angle of your view. You will not travel with the tumble action.
  Pan      Holding the right mouse button on a point in the scene (not the skybox) will begin "panning" (moving the camera, but not turning the camera. It will keep the originally hovered point under the mouse cursor.
  Orbit    Hold down the mouse scroll wheel as you move the mouse. The mouse movement will give you the sensation that you are orbiting around the point you selected. This feature is based on where you first click on the screen and will keep the camera the same distance from the clicked point by turning and moving the camera.
  Dolly    The mouse scroll wheel will "dolly" (zoom the camera in and out) from the point where the mouse is hovering (will not work with the skybox).

![Lights On](media/image12.png) If you
prefer the Euclideon Geoverse MDM method of using the Scroll Wheel to
change the mode speed, then that option is available in Settings >
Input & Controls > Mouse Pivot Bindings.

### Default Keyboard Controls {#default-keyboard-controls-1}

You can use specific keys, with the "Scene" window focused, to control
the camera movement.

  Action     Description
  ---------- -------------------------------------------------------------------------------------------------------
  W and S    manoeuvre the camera forward and backward at the current Camera Move Speed.
  A and D    pan the camera left and right at the current Camera Move Speed.
  R and F    ascend and descend the camera at the current Camera Move Speed.
  Spacebar   locks altitude, allowing you to pilot the camera without changing the camera's height (Z-axis lock).

You can adjust the Move Speed blue bar in the [Scene Info &
Controls](#section) panel. This setting is persistent across sessions.

![](media/image14.jpg)

![Lights On](media/image12.png) In
addition to mouse/keyboard controls, the camera can also be moved using
an Xbox Controller or equivalent gamepad/controller device.

### Default Gamepad Controls

Development testing is done using a Microsoft Xbox 360 controller, these
controls might be mapped differently on other brands or styles of
controllers.

-   Left Analog Stick: Move

-   Right Analog Stick: Rotate Camera (tumble)

-   Right Trigger: Orbit (with crosshair)

-   A Button: Toggle Lock Altitude

-   Y Button: Toggle Map Mode

-   Right Bumper: Toggle Fullscreen Mode

### Default Hotkeys

Use the following keys anywhere in the active udStream interface to
effect the relevant change:

-   F5: Toggles full screen mode on and off.

-   Ctrl+U: Opens a popup for adding additional files.

-   Delete: Removes the selected items from the scene

Scene Explorer
--------------

The Scene Explorer window lists the assets currently in your scene.

![](media/image15.jpg)

### Scene Action Bar

The buttons across the top of the Scene Explorer allow quick access to
add or remove from the scene.

![](media/image16.jpg)

  Button                                                                               Description
  ------------------------------------------------------------------------------------ ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  ![](media/image17.jpg)   This button opens the dialog box to add a UDS model to the scene. The Path/URL can include files on the local drives, network drives, UNC Paths, HTTP, HTTPS, FTP & FTPS.
  ![](media/image18.jpg)   Adds a folder to the scene explorer. This can help with organizing your scene.
  ![](media/image19.jpg)   Saves the current camera position and rotation as a viewpoint.
  ![](media/image20.jpg)   Opens a menu of more advanced scene items that can be added to the scene.
  ![](media/image21.jpg)   This deletes all selected items from the scene.

![Lights On](media/image12.png) If you
ever want to quickly clear the current scene, the "Projects" menu has
a "New Scene" option that will remove everything from the
scene.[]{#_Toc41928052 .anchor}

Scene Items
-----------

The area below the quick menu displays the items in the scene.

![](media/image22.png)

### Selecting Items

Click on an item in the Scene Explorer to select it. Selected items will
appear highlighted.

You can select multiple items in the scene explorer by holding Ctrl and
then left-clicking them individually, or shift-clicking to quickly
select larger groups. Doing this on a selected item will deselect it.

Single clicking without Ctrl will deselect all items and select just the
item you have clicked.

### Reorganising the scene

Items or groups of items in the scene explorer can be reordered by
holding left-click and dragging them around. A blue line indicates where
the item(s) will be after you release them.

### Loading UDS Models

There are two ways to add models to the scene.

1.  Drag and Drop

    a.  On devices with folder exploring you can drag a file from your
        file explorer and drop it in the scene window to add the model
        to the scene.

2.  Direct URL loading

    a.  You can type a URL or path in the Add UDS popup
        ![](media/image17.jpg). The path field at the top of the
        pane allows URL and network paths to retrieve UDS files.

### Scene Item Properties

Items in the Scene explorer (see [Scene Items](#_Toc41928052)) can be
expanded to give additional information (see below). In this information
is settings that can be used to modify that scene item. What information
that is displayed will depend on the scene item type.

### UDS Point Cloud

The expanded properties for UDS files show the path the UDS was loaded
from and then a tree view of the internal metadata from the file.

![](media/image23.png)

The system displays advanced information like the attributes in the
file, their sizes, and how they are blended for level of detail
calculations.

### Comparing Models

You can compare two models currently loaded in the scene that were
scanned at different times. The model you select in the scene explorer
will be the model that will have a distance calculated from each point
to a mesh of the model you will select in the steps below. Please note
that this process will generate a third model, representing the
comparison between the two entered models.

1.  Right-click on a model in the scene explorer

2.  Select the  Compare Models  option

3.  Enter the required information

    a.  Model to compare against. When doing a comparison between models
        from two different dates, this will be the older of the two
        models.

    b.  Ball radius (in metres), this is used to mesh the point cloud.
        This should be the maximum distance between two points that
        could be considered part of the same surface.

    c.  Grid size (in metres), this is used to split the model up into
        smaller pieces for processing. This will be the maximum distance
        a point can be from a point in the old model, points that sit
        outside all the grids will be displayed with the "No Match"
        colour when using the displacement visualisation options.

4.  Click the  Compare Models  button, this will create a convert job
    and open the convert window. See Convert section for more
    information.

### ![](media/image24.png)Selection Tool

This is the default tool when no other tool is active. It allows you to
select items in the scene.

### ![](media/image25.png)Voxel Inspector

This tool allows you to inspect the data associated with individual
voxels of a UDS file.

### ![](media/image26.png)Annotation Points

You can add multiple annotation points to the current scene, and alter
the name and colour of each annotation point. Simply click on the POI
(Point Of Interest) button in the tool bar on the left and click in the
scene.

Clicking on the POI in the Scene Explorer will give you options to
change the size and colours of the displayed text. Right clicking on the
POI name in the Scene Explorer will bring up a context menu, where you
can edit the POI name, move the camera to the POI in the scene, or
remove it. Add a hyperlink in the Hyperlink input, and follow by
clicking Open Hyperlink 

![](media/image27.png)

[]{#_Line_Measurements .anchor}

### ![](media/image28.png)Line Measurements

A Line Measurement is a series of connected points in the scene, useful
to visualise boundaries and measure distances.

To begin a line measurement click on the line measure icon in the tool
panel and click in the scene to place line nodes. A panel will appear in
the top right with a few options, and information about the line
measurement. See below for a description.

![](media/image29.png)

  Option                            Description
  --------------------------------- -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Show Length                       Display the cumulative length of the line segments.
  Show All Lengths                  Display the individual lengths of each of the line segments.
  Show Area                         Display the area encompassed by the line segments. The area is calculated from the projection of each line segment onto the x-y (horizontal) plane. To give a correct area, the shape my be simple, that is cannot contain line that cross over. Note: the Close Polygon option must be selected for an area to be calculated.
  Close Polygon                     Join the first and last points, thereby creating a 'closed' shape.
  Line Width                        Set the width of the line in the scene.
  Line Orientation                  There are three options to choose from: Screen Line, Vertical/Fence and Horizontal/Path. The Screen Line is simply a basic single-colour line drawn on the screen. The Vertical/Fence and Horizontal/Path options with create a vertical and horizontal oriented line object, respectively. These options are helpful when looking at the line from different angles.
  Line Colour                       Set the colours of the line.
  Selected Point                    Move the slider to select a node in the line. Once selected, you can manually change the position of the node by setting the x-y-z coordinates or remove the node from the line. A value of -1 selects indicates all points are selected.
  Text Colour / Background Colour   Set the colours of the text displayed along the line in the scene.
  Hyperlink                         Attach a hyperlink to the line. A button will appear to open the hyperlink once a link is set.
  Description                       Add a description to the line. This will be saved out with the project.

![Lights On](media/image12.png)Right
clicking on the name in the scene explorer will bring up a context menu
with additional options.

  Option                          Description
  ------------------------------- ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Edit Name                       Change the name of the line.
  Move To                         Move the camera such that the line is in view.
  Fly Camera Through All Points   This option will begin a fly-through of all points along the lines. The fly-through speed is determined by the Camera Move Speed. If the Close Polygon line setting is checked, the fly-through will repeat until cancelled.
  Attach Model                    A model can be attached to a line, and will continuously move along the segments of the line. This is useful for vehicle or flight paths. Once attached, more options will become available in the scene explorer under the respective line. These are used to control the speed of the model and set the type of face culling.
  Remove Item                     Deletes the item from the scene.

### ![](media/image30.png)Area Measurements

An Area Measurement is used to measure the horizontal area described by
3 or more points. The function is almost identical to the [Line
Measurement](#line-measurements), the only difference being the
resulting polygon is closed by default.

### ![](media/image31.png)Height Measurements

The height measurement tool is useful when you wish to measure the
height, and horizontal distance between two points. Click the height
measurement tool from the tool bar on the left-hand side. Click on a
point in the scene. Click on a second point to anchor the height
measurement.

![](media/image32.png)

### ![](media/image33.png)Show Angles

This tool will show angles between the lines of Area and Line
measurements.

### Image POI

To add an image at a specific location, simply drag and drop an image
into the scene.

![](media/image34.png)

+--------------------+------------------------------------------------+
| Option             | Description                                    |
+====================+================================================+
| Image Type         | Use Image Type to select how you want the      |
|                    | image to be displayed:                         |
|                    |                                                |
|                    | -   Standard with display the image as a       |
|                    |     billboard, that is, always facing the      |
|                    |     camera                                     |
|                    |                                                |
|                    | -   Orientated Image will allow you to change  |
|                    |     the orientation of the image               |
|                    |                                                |
|                    | -   Panorama will project the image onto a     |
|                    |     panoramic cylinder                         |
|                    |                                                |
|                    | -   Photo Sphere will project the image onto a |
|                    |     sphere                                     |
+--------------------+------------------------------------------------+
| Thumbnail Size     | The size the image will be displayed in the    |
|                    | scene                                          |
+--------------------+------------------------------------------------+
| Reload Time (secs) | Reload Time allows you to control how often    |
|                    | the image in updated in the scene. This is     |
|                    | useful if the image changes over time and      |
|                    | should be updated in the scene.                |
+--------------------+------------------------------------------------+

### Live Feeds (IOT)

Live Feeds (IOT) is intended for use by advanced users as it contains
features that can cause performance loss if not configured correctly.

> ![](media/image35.png)

  Option                               Description
  ------------------------------------ ----------------------------------------------------------------------------------------------------------
  Update Frequency                     The number of seconds to wait after completing an update before doing another update.
  Oldest display time                  Used to hide live feed assets that have not been updated in a while.
  Maximum distance to display          An override to assist with performance. If feeds are causing performance issues, this should be reduced.
  LOD Distance Modifier                How the Level Of Detail (LOD) changes with distance from the camera.
  Enable Tweening/Position Smoothing   Allows the positions and orientations of moving feed items to be calculated.
  Snap to Map                          Positions the live feed assets exactly on the map surface.
  Group ID                             Enter the Group ID as provided by the Euclideon Vault Server in the Group ID input box.

### Viewsheds

A view shed is the set of all points that are visible from a location.
It includes all points that are in direct line of sight to the location
and excludes points that are obstructed by terrain and other features.

To add a viewshed in the scene, right-click a point in the scene, choose
Add New Item  and then Add View Shed . It should look something like
below, where visible points are green and hidden are red. You can change
the colour of visible and hidden points, as well as set the projection
distance in the scene explorer.

![](media/image36.png)

![](media/image37.png)

### Filters

A filter is a 3D object placed in the scene to 'filter' out data. This
can be useful if you want to isolate a feature in your data set to view
or even to export. Currently there are 3 types of filters: box, sphere
and cylinder. To place a filter, right-click a point on your dataset and
choose Add New  then Add Filter .

For example,

  In the scene, we want to isolate the shorter building in the centre...             So we add a cylinder filter
  ---------------------------------------------------------------------------------- ---------------------------------------------------------------------------------
  ![](media/image38.png)   ![](media/image39.png)

In the scene explorer, expand the filter item to modify its shape,
transformation and whether you want to filter out or filter in data. The
filter will also respond to a [gizmo](#gizmos). Simply highlight the
filter in the scene explorer and choose a gizmo.

To export the filtered data, right click on the model you want to export
in the Scene Explorer. Choose Export Pointcloud  then choose the filter
you want to export if you have more than one.

![](media/image40.png)Settings
------------------------------------------------------------------------------------------

Here is where you can customise how the udStream looks and operates.

-   Alter the [Appearance](#appearance) of the interface

-   Change the [Input and mouse control](#_Input_and_Controls) settings

    -   Edit the [Key Bindings](#tools)

-   Manipulate the [Maps & Elevation](#maps-and-elevation) settings

-   Change the [Visualisation](#visualisation-1) of your data through
    colours and contours.

-   Configure the Annotation and measurement
    [Tools](#_Line_Measurements)

-   Set the default settings when you [Convert](#convert) a file

-   Set the [Screenshot](#screenshot) settings

-   Change the [Connection](#connection) settings

![Lights On](media/image12.png)To
restore all default values for any of these settings, simply right-click
on the sub-heading and then select Restore Defaults.

### Appearance

![](media/image41.png)

The Appearance values enable you to adjust several settings that change
the appearance of the udStream interface.

  Option                           Description
  -------------------------------- -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Language                         Choose your language.
  Units of Measurement             The measurement system to use. Currently Metric and US Survey are supported.
  Show Camera Information          Display camera information such as latitude/longitude, heading/pitch and camera speed.
  Show Projection Information      Check to display projection information in the Camera Information panel.
  Show Advanced GIS Settings       Check to display advanced GIS settings in the top-right corner.
  Show Diagnostic Information      Check the box to display diagnostic information, such as the frame rate. The system will display the frame rate in frames per second (FPS) and milliseconds (ms) on the top menu bar next to the license and username information. Diagnostic information for Live Feeds includes Total Cached Items, Number of items displayed, Next Update.
  Show Euclideon Logo              Will display the Euclideon logo at the bottom of the Scene View
  MAX Point Of Interest Distance   Use the blue bar slider to change the maximum distance at which Points of Interest (includes Lines and Areas) are visible. If this value is low, you will not be able to see the POIs when zoomed out.
  Image Rescale Distance           Image POIs will start to shrink and disappear when this distance away from the camera.
  Limit FPS In Background          Enable this to limit the FPS (frame rate) when the client is running in the background, reducing interference with other open programs.
  Show Native Dialogs              Show your operating system's native dialog windows when you save or load a file.
  Skybox Type                      Toggle to change the visualization of the atmosphere. The options range from full atmospheric scattering, to displaying a simple background colour.
  Saturation                       Use the blue bar slider to change the saturation effect applied to the scene.
  Voxel Shape                      Set the shape of voxels in the Scene window as either points, rectangles, or cubes. Euclideon recommends rectangles for accuracy and performance.
  Layout                           Choose which side of the screen you would like the Scene Explorer.

![Lights On](media/image12.png)For
settings that are controlled via a slider, you can press Ctrl+click on
the slider and manually enter a value, allowing you to enter a more
precise value or, in some cases, extend the slider value beyond its min
and max range.

[]{#_Input_and_Controls .anchor}

### Input and Controls

Expanding this panel allows you to change settings that affect how you
interface with the udStream.

![](media/image42.png)

+----------------------------------+----------------------------------+
| Option                           | Description                      |
+==================================+==================================+
| On Screen Controls               | You can toggle on-screen mouse   |
|                                  | controls, which will appear at   |
|                                  | the bottom-left of the Scene     |
|                                  | window. Click and drag the blue  |
|                                  | bar in the U/D box to move the   |
|                                  | Scene up or down. Click inside   |
|                                  | the Move Camera box and drag the |
|                                  | mouse out. You will see a line   |
|                                  | appear from within the box. The  |
|                                  | camera position and rotation is  |
|                                  | controlled by how far you draw   |
|                                  | out and in which direction you   |
|                                  | drag this line.                  |
|                                  |                                  |
|                                  | ![On-screen                      |
|                                  | con                              |
|                                  | trols](media/image43.png "On-scr |
|                                  | een controls")                   |
+----------------------------------+----------------------------------+
| Touch Friendly UI                | You can enable udStream to       |
|                                  | recognise touchscreen devices.   |
+----------------------------------+----------------------------------+
| Invert Mouse X-axis / Invert     | Check to invert the default      |
| Mouse Y-axis                     | camera movement when you drag    |
|                                  | the mouse along the X and Y      |
|                                  | axes.                            |
+----------------------------------+----------------------------------+
| Invert Controller X-axis /       | Check to invert the default      |
| Invert Controller Y-axis         | controller axis.                 |
+----------------------------------+----------------------------------+
| Mouse Snap to Points             | Snap the mouse to points in your |
|                                  | datasets.                        |
+----------------------------------+----------------------------------+
| Mouse Pivot Bindings             | You can customise the            |
|                                  | mouse-controlled camera          |
|                                  | movements. Find the descriptions |
|                                  | of the different mouse pivot     |
|                                  | bindings in the Default Mouse    |
|                                  | Controls section.                |
+----------------------------------+----------------------------------+
| Keep Camera Above Ground         | Checking this will ensure the    |
|                                  | camera will never go below the   |
|                                  | maps surface (if they are        |
|                                  | enabled).                        |
+----------------------------------+----------------------------------+

![Lights On](media/image12.png) To
enable/disable maps, see section "Maps and Elevation".

### Key Bindings

This section allows you to set up your own key bindings. Simply click an
action to want to bind and press the key you want have bound.

> ![](media/image44.png)

### Maps and Elevation

The map feature is useful when you need a geospatial view and are
unfamiliar with the area or are looking for a specific reference point.
Select Map & Elevation within the Settings tab.

![](media/image45.png)

+----------------------------------+----------------------------------+
| Option                           | Description                      |
+==================================+==================================+
| []{#visualisation .anchor}Map    | Toggles the map system. Enabling |
| Tiles                            | the mapping system will increase |
|                                  | network usage.                   |
| Digital Elevation Model          |                                  |
|                                  | Adds height data to the map      |
|                                  | system. Enabling the map height  |
|                                  | data will increase network usage |
+----------------------------------+----------------------------------+
| Base Map                         | Allows the user to choose from a |
|                                  | pre-configured set of map        |
|                                  | servers. ADVANCED: Checking the  |
|                                  | "Custom" option will show        |
|                                  | further options, allowing you to |
|                                  | configure your own tile server   |
|                                  | URL.                             |
+----------------------------------+----------------------------------+
| ECEF Mode                        | Sets the maps to render as a     |
|                                  | globe.                           |
| Map Height                       |                                  |
|                                  | Offset vertically the map tile   |
| Blending                         | positions.                       |
|                                  |                                  |
| Opacity                          | Sets the maps to intersect the   |
|                                  | scene geometry (default), or     |
|                                  | always be under or over the      |
|                                  | scene geometry.                  |
|                                  |                                  |
|                                  | Transparency of the map tiles.   |
+----------------------------------+----------------------------------+

![Lights On](media/image12.png)Hold down
the Ctrl-key and click in any parameter that has a slide bar to manually
set a more precise value or set a value outside the parameter
boundaries.

### Visualisation {#visualisation-1}

The Visualisation panel allows you to change some of the visual aspects
of udStream, as well as visualise your data in different ways.

#### Display Mode

The Display Mode drop down list allows you to view the different types
of data or 'channels' associated with your datasets. This including
Colour, Intensity, Classification and Displacement, GPS Time, Scan
Angle, Point Source ID Return Number and Number of Returns. Note: not
all datasets will contain all these channels.

+-----------------------+---------------------------------------------+
| Channel               | Description                                 |
+=======================+=============================================+
| Colour                | Colour is the default display mode.         |
+-----------------------+---------------------------------------------+
| Intensity             | Intensity refers to the strength of the     |
|                       | laser pulse that generated a point. Use     |
|                       | 'Min Intensity' and 'Max Intensity' to set  |
|                       | the intensity range you want to view.       |
+-----------------------+---------------------------------------------+
| Classification        | In this mode, a checkbox appears which      |
|                       | enables you to customise the colours of     |
|                       | objects corresponding to their designated   |
|                       | classifications.                            |
|                       |                                             |
|                       | ![](media/image46.png)                      |
+-----------------------+---------------------------------------------+
| Displacement Distance | Set the range at which you want to view the |
|                       | displacement between two models. You can    |
|                       | also set the colours corresponding to       |
|                       | maximum and minimum displacement, as well   |
|                       | as set the colour for displacements outside |
|                       | of this range.                              |
+-----------------------+---------------------------------------------+
| GPS Time              | This setting allows you to visualise the    |
|                       | times at which your data was scanned. Time  |
|                       | is typically stored as a single number,     |
|                       | representing the number of seconds since a  |
|                       | specific point in time, or 'epoch'. For     |
|                       | most laser scans, this is typically 'GPS    |
|                       | Time' or 'Adjusted GPS Time'. GPS Time is   |
|                       | the number of seconds since the midnight    |
|                       | the 6^th^ of January 1980, and GPS Adjusted |
|                       | Times is GPS Time minus one billion.        |
|                       |                                             |
|                       | You will need to tell udStream how to       |
|                       | interpret this number, by choosing either   |
|                       | choosing GPS or GPS Adjusted from the       |
|                       | dropdown box. You can also set the range of |
|                       | time you wish to visualise.                 |
+-----------------------+---------------------------------------------+
| Scan Angle            | For data sets created by a laser scanner,   |
|                       | the scan angle is the angle at which the    |
|                       | laser leaves the scanner, from -180° to     |
|                       | 180°, where 0° is directly in front of the  |
|                       | scanner. You can refine the range by        |
|                       | adjusting the Minimum Angle and Maximum     |
|                       | Angle.                                      |
+-----------------------+---------------------------------------------+
| Point Source ID       | Sometimes, each point in the dataset may    |
|                       | have an associated number, or ID to         |
|                       | identify where the data came from. This     |
|                       | could mean any number of things but         |
|                       | commonly is linked to the location of the   |
|                       | dataset. Follow these steps to build up a   |
|                       | list of IDs you want to visualise:          |
|                       |                                             |
|                       | 1.  Set 'Next ID' to the ID you want to     |
|                       |     register                                |
|                       |                                             |
|                       | 2.  Set the colour associated with this ID  |
|                       |                                             |
|                       | 3.  Press 'Add' to add the ID to the list   |
|                       |     of currently registered IDs.            |
|                       |                                             |
|                       | Any IDs that are not registered will use    |
|                       | the 'Default Colour.' You can remove an ID  |
|                       | by pressing the                             |
|                       | ![](media/image47.png)} next to the         |
|                       | list item. Click Remove All  to clear the   |
|                       | list.                                       |
+-----------------------+---------------------------------------------+
| Return Number         | A laser pulse can return up to 6 times when |
|                       | scanning a feature. It is sometimes useful  |
|                       | to know which *return number* is associated |
|                       | with each point in the dataset. In this     |
|                       | display mode, you can associate a different |
|                       | colour with each return number.             |
+-----------------------+---------------------------------------------+
| Number of Returns     | This signifies how many times the laser     |
|                       | pulse was returned when scanning a feature. |
|                       | There is a maximum of 6. The colour of each |
|                       | can be set in this display mode.            |
+-----------------------+---------------------------------------------+

#### Other Visualisation Options

+----------------------------------+----------------------------------+
| Option                           | Description                      |
+==================================+==================================+
| Camera Lens (fov)                | Field of View. This controls the |
|                                  | horizontal extents of the scene  |
|                                  | displayed on the screen.         |
+----------------------------------+----------------------------------+
| Skybox Type                      | This changes the visualization   |
|                                  | of the sky and surrounding       |
|                                  | atmosphere. The options include: |
|                                  |                                  |
|                                  | -   None: The sky will be        |
|                                  |     displayed black              |
|                                  |                                  |
|                                  | -   Colour: Set your own sky     |
|                                  |     colour                       |
|                                  |                                  |
|                                  | -   Simple: A simple texture of  |
|                                  |     a blue sky with clouds       |
|                                  |                                  |
|                                  | -   Realistic atmospheric        |
|                                  |     colouring, taking into       |
|                                  |     account the time of day. You |
|                                  |     can adjust the time of day,  |
|                                  |     time of year and the         |
|                                  |     brightness of the sun.       |
+----------------------------------+----------------------------------+
| Saturation                       | Used to control the colour       |
|                                  | saturation of the scene.         |
+----------------------------------+----------------------------------+
| Voxel Shape                      | Set the shape of voxels in the   |
|                                  | Scene window as either points,   |
|                                  | rectangles, or cubes. Euclideon  |
|                                  | recommends rectangles for        |
|                                  | accuracy and performance.        |
+----------------------------------+----------------------------------+
| Enable Selected Objects          | Selected objects will now be     |
| Highlighting                     | highlighted in the scene, using  |
|                                  | the specified Highlight Colour   |
|                                  | and Highlight Thickness          |
+----------------------------------+----------------------------------+
| Enable Edge Highlighting         | Highlights the edges of every    |
|                                  | voxel in the scene, using the    |
|                                  | specified width and colour. The  |
|                                  | threshold determines how to      |
|                                  | resolve the edges of overlapping |
|                                  | voxels.                          |
+----------------------------------+----------------------------------+
| Enable Colour by Height          | Displays colours along the       |
|                                  | specified two-colour gradient to |
|                                  | all objects in the scene based   |
|                                  | on their height.                 |
+----------------------------------+----------------------------------+
| Enable Colour by Distance To     | Displays colours along a         |
| Camera                           | one-colour gradient to all       |
|                                  | objects in the scene based on    |
|                                  | their distance from the camera.  |
+----------------------------------+----------------------------------+
| Enable Contours                  | Displays elevation contours      |
|                                  | depicted by Contour Colour on    |
|                                  | all objects. Contours Band       |
|                                  | Height is the width of the       |
|                                  | Contour Distances in the         |
|                                  | vertical space between each      |
|                                  | contour.                         |
+----------------------------------+----------------------------------+

The last five display options also allow you to customise the colours.

![](media/image48.png)

![Lights On](media/image12.png) If you
changed a setting and can't recall what you've changed, you can reset
all settings by going to the System menu and selecting Restore Defaults,
or you can reset individual groups of settings by right clicking on
their header in the Settings Window and selecting Restore Defaults.

![Lights On](media/image12.png) All
parameters in the Settings Window are persistent across sessions: If you
log out and log back in, the system will restore your settings from the
previous session.

### Tools

Here you can set the default settings for the udStream tools.

![](media/image49.png)

  Option              Description
  ------------------- -----------------------------------------------------------------------------------------------------
  Line Width          Set the line width for all new line measurements you add to the scene.
  Line Orientation    Set the line orientation for all new line measurements you add to the scene.
  Line Style          Choose whether you want new measurements to be placed as screen lines, fences, or horizontal paths.
  Line Colour         Set the line colour for all new line measurements.
  Text Colour         Set the text colour for all new line measurements.
  Background Colour   Set the text background colour for all new line measurements.

### Convert Defaults

Converting is a process where a point cloud or 3D model is converted
into Euclideon's own file format, UDS. In this section you can set some
default settings when you convert your files.

![](media/image50.png)

  Option                                   Description
  ---------------------------------------- ----------------------------------------------------------------------------------------------------------
  Temp Directory                           The process of conversion requires temporary files to be written to disk. You can set the location here.
  Change Watermark Image                   Embed a watermark image of your company logo into your uds files.
  Author, Comment, Copyright and License   These are optional fields you can add as metadata to your dataset when you convert.

### Screenshot

Pressing the PrtScn Key will take a screenshot of the current Scene
Viewport and place it into a folder of your choosing.

> ![](media/image51.png)

  Option            Description
  ----------------- ------------------------------------------------------------------------
  Resolution        Set the resolution of your screenshots
  Filename          Choose the path you wish to save your screenshots
  View Once Taken   Display the image in a separate window each time you take a screenshot

### Connection

Here we can set various connection settings.

> ![](media/image52.png)

  Option                            Description
  --------------------------------- -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Proxy Address                     The address for your internet proxy (this is provided by your system administrator). It can additionally include the port number and protocol. Examples include: 192.168.0.1, 169.123.123.1:80 or https://10.4.0.1:8081.Leaving this blank will attempt auto-detection.
  User Agent                        A field included in HTTP request headers. Very few users will need to specify this setting.
  Ignore Certificate Verification   Will disable verification of the PEER and HOST certificate authorities. This setting should only be used when instructed by your system administrator and only when errors are occurring during the login process, as it will weaken the security between your computer and the Euclideon Vault Server. NOTE: Ignore Certificate Verification will not be saved due to the security risk associated. You will need to enable this setting each time you open the application.
  Require Proxy Authentication      If you require Proxy Authentication, Set the username and password here.

Scene Info & Controls
=====================

Use the controls on this panel to manipulate the camera and control the
scene.

![](media/image53.png)

  Label   Name                        Hotkey     Description
  ------- --------------------------- ---------- ------------------------------------------------------------------------------------------------------------------------------------------------------------
  1       Scene Profile Menu                     Access your profile, change password, export project, sample projects, or to log out.
  2       Scene Explorer Menu         F4         Show/Hide Information controls for the models and assets within the Scene window.
  3       Selection Tool              F5         Set the mouse to select objects.
  4       Voxel Inspector             F9         Set the mouse to display voxel attributes when hovering over a model. For example, RGB/ Intensity/Classifications/Latitude & longitude.
  5       *Annotated Point*           F10        Adds an annotated point to scene.
  6       *Line Measurement Tool*     F7         Adds an editable measurement line.
  7       Area Measurement Tool       F8         Adds an editable area of measurement.
  8       *Height Measurement Tool*   h          Allows you to measure heights and horizontal distances.
  9       *Angle Measurement Tool*               Allow you to measure angles.
  10      *Begin New Project*                    Allows to create a new project, either geolocated or non-geolocated, or choose a sample project.
  11      *Import Project*                       Opens a project (.json).
  12      *Save project*                         Saves the project to a chosen destination.
  13      *Share project*
  14      *Convert*                              Opens a new window to begin a convert project.
  15      *Settings*                             Opens the settings window.
  16      *Help*                                 Opens a new tab in your browser, taking you to the udStream help page.
  17      *Information panel*                    Displays information about the currently selected object in the scene.
  18      *Translate Gizmo*           b          Sets the gizmo to translation mode, allowing you to move objects.
  19      Rotate Gizmo                n          Sets the gizmo to rotation mode, allowing you to rotate objects.
  20      Local Space Gizmo           c          Toggles the gizmo's operational coordinate system between local coordinates (relative to the object) and world coordinates (relative to the world space).
  21      Scale Gizmo                 m          Sets the gizmo to scaling mode, allowing you to resize objects.
  22      *Compass*                              The red element of the compass will always point True North. Clicking the compass will return the camera to face north.
  23      *Lock Altitude*             spacebar   Will keep the camera's height constant (Z-axis lock) when panning with the mouse or strafing with the keyboard.
  24      Map Settings Button                    Change Base map tiles and ECEF mode.
  25      *Visualisation*                        Opens a panel to change some of the visual aspects of udStream.
  26      Full Screen                 F5         Open the Scene Window in Full Screen mode

### Gizmos

A "gizmo" is used to move objects around in the scene. When active, the
gizmo appears at the origin of an item. Gizmos are disabled by default.
To activate the gizmo tool, do the following:

1.  Select an item in the Scene Explorer window (see [Selecting
    Items](#selecting-items)).

2.  Select a gizmo tool from the udStream interface (see [Scene Info And
    Controls](#section))

![](media/image54.jpg)

In Local Space Mode, the axis will align with the local axis of the last
selected item. If the model does not have a local space axis, then the
gizmo will use the global axis ![](media/image55.jpg). Red is the X axis, which in projection space usually
corresponds to the EASTING. Green is the Y axis, which usually
corresponds to the NORTHING. Blue is the Z axis, which usually
corresponds to the ALTITUDE. Looking at the gizmo from different angles
will cause one or more of its axes to appear hatched/dashed. This
indicates that this axis is pointed in the negative direction.

![Negative angled
gizmo](media/image56.png "Negative angled gizmo")

Use the Translation Gizmo to move scene item(s) around. The translation
gizmo has 3 components:

1.  The coloured axis arms of the gizmo will translate only along that
    axis.

2.  The coloured squares between two axis arms will translate in that
    plane. For example, the square between the X and Y axes translates
    only in the XY plane. The colour indicates which axis will not be
    modified by using that square.

3.  The white circle at the origin of the gizmo will translate the
    models in the plane perpendicular to the camera.

The Rotation Gizmo changes the orientation of the item(s). Items will
rotate around their pivot point, which is ideally the centre of the
mass. For UDS models, a pivot point is selected automatically during
conversion. Non-UDS models can designate a different pivot point. The
rotation gizmo has 2 components:

1.  The coloured rings will each rotate around the axis of that colour
    (e.g. the Z-axis is blue, so the blue ring rotates everything around
    the Z-axis).

2.  The white ring rotates everything around the axis parallel to the
    direction of the camera.

The Scale Gizmo changes the size of the selected item(s). The anchor for
scaling is always the centre of the item(s). The scale gizmo has 2
components:

1.  The coloured axis arms of the gizmo can be used to scale along that
    axis only.

2.  The white circle at the origin of the gizmo will scale uniformly in
    all directions.

In some situations, you may find parts of the gizmo greyed out. This
will happen when the model does not support that action. For instance,
UDS files do not support non-uniform scaling. Therefore, that gizmo will
be unavailable.

![Scene gizmos greyed out](media/image57.png "Scene gizmos greyed out")

![Lights On](media/image12.png) You can
undo a rotation, translation or scaling by right clicking the model in
the Scene Explorer and selecting the transformation you wish to undo.

![](media/image58.jpg)

### Copyright and Compass

Copyright data will be displayed in the bottom-right corner of the
viewport. You can add Copyright Data to new models in the metadata
textbox during conversion.

### Watermark

If a watermark was included in the model, it will appear at the bottom
left-hand corner of the Scene window.

![](media/image59.jpg)

Export
======

You can export the entire scene as a project file. Simply navigate to
Scene Profile Menu > Export (see [Scene Info & Controls](#section)).

Alternatively, you can export all or part of a point cloud model. Go to
the model in the scene explorer and select Export Pointcloud. This will
save the model as a .uds file at your chosen destination. If you have an
active filter associated with the model, you will also have a choice to
export just what is contained in the filter.

![](media/image60.png)

Convert
=======

To access the Convert Options, go to Scene Profile Menu > Convert (see
[Scene Info & Controls](#section)).

The udStream enables users to import 3D models and convert them to
Euclideon's Unlimited Detail format (UDS). Euclideon's UDS format
enables you to stream and load massive point cloud data sets. Euclideon
Vault currently supports the following file types:

UDS, UDG, LAS, LAZ, SSF, FBX, PTS, PTX, TXT, CSV, XYZ, E57, OBJ, DXF,
DAE and ASC (Esri)

![](media/image61.png)

![Lights On](media/image12.png) If you
attempt to drag and drop the file to want to convert on to the Convert
Window before typing in the Output Name path and filename, then udStream
will report that the file type is not supported.

  Step      Action                                                                                   Comments
  --------- ---------------------------------------------------------------------------------------- -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  1, 2.     Create a Job and add a file                                                              Begin by creating a new job, or simply by adding a new file. Drag and drop a file will automatically start a new job. You can add multiple files to a job, and convert will collate them into a single file.
  3.       Type in the Output Name---path and name---of the exported UDS file.                       Ideally, this parameter should be the final target location of the file so that you do not need to copy the file at the end. The system will add the file extension for you. Make sure there is enough space in the target directory to store the entire converted file.
  4.       Type in the Temp Directory                                                                Defaults to the same location as the output file in the previous step. Type C:\\Vault\\convert in the Output Name field and the Temp Directory will default to C:\\Vault\\convert\_temp. Euclideon recommends setting the Temp Directory location to one that resides on a high-speed local drive with plenty of space (rule of thumb: set aside as much space as the uncompressed raw input, as the points in the temporary files have not as yet been compressed), as the convert process will write many small, temporary files. The conversion will remove the temp directory after completion.
  5.       Optional Selection: Continue processing after corrupt/incomplete data (where possible).   If corrupt or incomplete data is detected during a conversion, this setting will decide whether the conversion is cancelled (unticked) or continues (ticked) resulting in a skip of the corrupted points. NOTE: If you tick this setting, the system will not report that it found corrupt or incomplete data.
  6.       Optional Selection: Point resolution:0.00000---Override                                   Set to the source size, or it will default to 0.01 if no source size configured. This setting determines the size of the points (in meters) in the point cloud after conversion. It will attempt to find a good size based on the source data but can be set manually by first ticking the "Override Resolution" checkbox and then typing a value in the text field.
  7.       Retain Primitives, (texture, polygons, lines etc)                                         This option sets the convert context up to retain rasterised primitives such as lines/triangles to be rendered at finer resolutions at runtime
  8.       Optional Selection: Override Geolocation                                                  If the source data has been correctly geolocated, when you import that file prior to converting it, the file should already have the "Spatial Reference Identifier" (SRID) information filled in this box: search for "EPSG code" or "SRID code" in the metadata. The SRID is the Geotagged ID of the GIS zone for the exported model. If the pre-parse can detect the SRID it will be set automatically. If not, and you wish to correctly geolocate your data, then you can manually select the "Override" checkbox and enter the correct SRID in the text field. It assumes that the input is already in the correct zone. Global Point Offset can be used to add an offset to the x, y and z coordinates of the converted model from that which was specified in the file's metadata, or if not, from the global origin (0, 0, 0).
  9.       Quick Partial Convert                                                                     Enabling this option will greatly speed-up the conversion process by only processing 1 in every 1000 points of the input data. This is useful for testing and calibrating the conversion settings before attempting to convert a large and time-consuming job. When imported into the scene the converted model will appear disintegrated and will have 1/1000th of the intended resolution.
  10.      Metadata                                                                                  This section allows you to add related information to the metadata of the output file, such as Author, Comments, Copyright and License holders.
  11.      Add a watermark                                                                           The watermark will be stored in the metadata of the file and will be displayed when you view the dataset.
  12, 13.   Set the source x and y                                                                   The source x and y values can mean a variety of things, for example latitude / longitude, or simply cartesian coordinates. Set the source x, y values for all files with 13, or individual files with 14.
  14.      Look at the Input Files panel.                                                            The estimated number of points in each file will be shown. During conversion, the progress for each file will also be shown. Here you can also remove any unwanted files.
  15.      Click Begin Conversion                                                                    udStream will read the file then process and write the points.
                                                                                                     The "X" button beside the convert job in the "Convert Jobs" section allows you to cancel a running convert (it will cancel at the next 'safe' point and clean up temporary files). Once cancelled, the 'Begin Convert' button and the configuration options will reappear enabling you to restart the conversion. After a job has completed the "X" button also allows you to remove it from the list.
                                                                                                     When the conversion has completed successfully, click the Add to Scene button located at the top of the window. udStream interface will automatically switch to the Scene Window to display your 3D model.

![Lights On](media/image12.png)
Euclideon does not recommend running multiple converts at the same time.
Converting is a memory and processor intensive process so it is almost
always faster to have 1 convert running at a time. udStream helps with
this by allowing you to queue multiple jobs to run one after another.

### Converting from Command Line (CMD)

udStream also comes with a separate command line application which
allows you to convert files via the command line instead of using the
Convert Tab.

  Command or Option        Action
  ------------------------ ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  udStreamConvertCMD.exe   Open a command prompt and navigate to the folder containing the executable file vaultConvertCMD.exe Alternatively, enter the full path of this executable. Remember to put quotes around paths that contain spaces.
  Syntax                   udStreamConvertCMD server username password \[options\] -i inputFile \[-i anotherInputFile\] -o outputFile.uds
  server                   The name and location of your udStream Server
  username password        Your account credentials for connecting to server.
  -i inputFile             The location of the file you wish to convert. If the file's path contains any spaces, you need to put the path in quotes, for example: "C:/My Data/File to Convert.csv" Use multiple -i InputFile options to specify additional input files. NOTE: The conversion process will merge all files into the one output file, so ensure they are compatible first.
  -o outputFile.uds        The name and location of the new .uds file you wish to create. Again, if the file's path contains any spaces you must put the path in quotes, for example: "C:/Output/Converted File.uds"
  -resolution              override the resolution (0.01 = 1cm, 0.001 = 1mm)
  -srid                    override the srid code for geolocation
  -globalOffset            add an offset to all points, no spaces allowed in the x,y,z value
  -pause                   Require enter key to be pressed before exiting
  -pauseOnError            If any error occurs, require enter key to be pressed before exiting
  -proxyURL                Set the proxy URL
  -proxyUsername           Set the username to use with the proxy
  -proxyPassword           Set the password to use with the proxy

Here is an example of what you might see when doing a simple convert of
a file 'POINT\_CLOUD.csv' to 'myDataset.uds'.

D:\\EuclideonConvert>udStreamConvertCMD.exe
https://udStream.euclideon.com username password -i
"./POINT\_CLOUD.csv" -o "myDataset.uds"

udStreamConvertCMD 0.2.0

Converting\--

Output: myDataset.uds

Temp: myDataset\_temp/

\# Inputs: 1

\[1/1\] ./POINT\_CLOUD.csv: 163,114/163,114

\--

Conversion Ended!

\--

./POINT\_CLOUD.csv: 163,114/163,114 points read

Output Filesize: 137,921

Peak disk usage: 137,921

Temp files: 0 (0)

### Hosting a model

You can store 3D models on:

1.  A local computer hard-drive

2.  A network drive

3.  In the Cloud.

Detailed configuration and implementation instructions on how to host
models in a cloud is outside the scope of this document. Please, refer
to the Euclideon Server and udSDK Guides for more information.

12dxml support
==============

udStream now has limited support for 12dxml files. Drag and drop your
file into udStream to load the file.

A 12dxml file can have any Unicode encoding. There are many different
encodings although only the following encodings are supported:

-   ANSI

-   UTF-8

-   UTF-8 BOM

If a file uses an unsupported encoding you will be notified with a
pop-up window.

![Lights On](media/image12.png) If the
file encoding is not supported, you can just open the file in notepad,
click Save As and down the bottom under Encoding, select UTF-8.
Alternatively, in the software you created the file, export the project
again as UTF-8.

[]{#technical-information .anchor}Currently only a part of the 12dxml
spec is supported. More features may be supported in future releases.
Check back, or if you have a specific request for a 12dxml feature, let
us know at <support@euclideon.com>. Otherwise, udStream will try to
convert a 12dxml file in the follow manner:

-   All Models will be extracted and loaded as folders in the scene
    explorer

-   Super strings from within models will be extracted and stored in the
    model folder. The following super string elements are supported:

    -   3D points and point lists will load as Annotations and Lines of
        Interest respectively

        -   Both the data\_3d and data\_2d tag are supported

    -   Names

    -   Colours

    -   The null and colour command is supported and can be set once per
        string

Technical Information {#technical-information-1}
=====================

### Requirements

udStream OpenGL: Requires OpenGL version 3.2 and up-to-date graphics
drivers.

udStream DirectX11: Requires DirectX 11 and up-to-date graphics drivers.

A reliable Internet Connection with adequate bandwidth.

A udStream License for convert and render operations.

### udStream Login credentials

If you have not received your udStream license or login credentials,
then please contact Euclideon at <info@euclideon.com> or go to
[Euclideon's website](https://euclideon.com) to access our online
support.

### Settings On Disk

We mentioned in [Visualisation](#visualisation) that your interface
settings are persistent across sessions, and this is because udStream
stores that information in a settings.json file, the location of which
is dependent on your operating system.

Windows: %AppData%/Roaming/Euclideon/client/settings.json

macOS: \~/Library/Application Support/euclideon/client/settings.json

Linux: \~/.local/share/euclideon/client/settings.json

Third Party Licenses
====================

udStream uses a number of third party libraries. Goto Scene Profile Menu
> Settings > About for full license information (see [Scene Info &
Controls](#section)).

Dear ImGui from [GitHub](https://github.com/ocornut/imgui)

ImGuizmo from [GitHub](https://github.com/CedricGuillemet/ImGuizmo)

libSDL2 from [libsdl](https://libsdl.org)

GLEW from [SourceForge](http://glew.sourceforge.net/)

Nothings/STB single header libraries from
[GitHub](https://github.com/nothings/stb)

ImGuizmo from [GitHub](https://github.com/CedricGuillemet/ImGuizmo)

easyexif available at [Euclideon's
GitHub](https://github.com/euclideon/easyexif) forked originally from
[GitHub](https://github.com/mayanklahiri/easyexif)

Autodesk FBX SDK from [Autodesk FBX SDK Download
Page](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-5)

Atmosphere from
[Evasion](http://www-evasion.imag.fr/Membres/Eric.Bruneton/)

Poly2tri from [Github](https://github.com/jhasse/poly2tri)

Euclideon Vault Development Kit (VDK) uses the following additional
libraries.

cURL from [GitHub](https://github.com/curl/curl)

libdeflate from [GitHub](https://github.com/ebiggers/libdeflate)

mbedtls from [GitHub](https://github.com/ARMmbed/mbedtls)

miniz from [GitHub](https://github.com/richgel999/miniz)

FAQs
====

[]{#_Toc41928085 .anchor}Why can't I see my model?
Ensure the model is enabled: a tick should appear in the tickbox next to
the model name in the Scene Explorer window.

[]{#_Toc41928087 .anchor}How do I lock altitude when moving the camera?
Press the Space bar to toggle or press the lock altitude button in the
'status' window (bottom right of the Scene window). See [Scene Info
and Controls](#scene-info-and-controls) for more information.

[]{#_Toc41928088 .anchor}How do I load my previous projects?
You can load a project that you saved to disk by clicking the Import
button at the top of the window and browsing to where you saved your
<project>.json file.

### How do I convert into UDS?

Select the Convert window and type the destination path and name for the
converted file in the Output Name field (the system will add the .uds
extension once you click out of that field if you don't enter it). Drag
and drop the file you want to convert on to the Convert window. Fill out
other fields as required. Refer to Convert in this guide for detailed
instructions.

[]{#_Toc41928090 .anchor}A converting error occurred, what do I do?
Due to either corrupt or incomplete data, clicking the 'Continue
processing after corrupt/incomplete data (where possible)' tickbox will
let the conversion process know that it should attempt to complete the
convert process, ignoring data integrity. Euclideon cannot guarantee
that the model will convert correctly or that if it does, it will be a
useful rendering.

[]{#_Toc41928091 .anchor}I am dragging my file to convert onto the
convert window, but nothing is happening. What do I do?
Confirm that the file type is supported for conversion by Vault.

[]{#_Toc41928092 .anchor}I want to demonstrate key features of my 3D
model, how can I do that?
Check out the [Visualisation](#visualisation) category under Settings.

[]{#_Toc41928093 .anchor}How do I adjust the mouse controls?
View Mouse Pivot bindings in the [input and
controls](#input-and-controls) menu in settings.

[]{#_Toc41928094 .anchor}My 3d object is hidden behind the map. How do I
see it?
Changing the Transparency or the Blending in [Maps and
Elevation](#maps-and-elevation) may make it easier to see your object,
using a combination of these for varied affect.

[]{#_Toc41928095 .anchor}I keep getting the "logged out" screen, how
do I fix this issue?
Check your internet access and try again.

[]{#_Toc41928096 .anchor}I cannot log in
Check your internet connection, and ensure you have used the correct
URL, username, and password. Make sure your URL has a closed bracket at
the end of it if using the default server URL.

[]{#_Toc41928097 .anchor}I cannot connect to the udStream Server, how do
I resolve this issue?
Check your firewall or proxy settings. If running a proxy, check with
your IT department that the address is correct and that the correct port
is set.

[]{#_Toc41928098 .anchor}My proxy is not working, why?
Adopt the proxy format of protocol://username:password\@domain/URI.
Network metadata is not transmitted when using proxies and is stored in
plain text file, which may assist solving connection issues.

[]{#_Toc41928099 .anchor}Could not open a secure channel. Why is this
popping up?
If you are using a proxy, your network may not be sending encrypted
data. Ticking the "ignore certificate verification" may circumvent
this issue. Note: Network Security certificates will not be verified.

Contact Us
==========

To learn more about udStream and other Euclideon solutions, please email
<sales@euclideon.com> or visit us at
[https://www.euclideon.com](https://www.euclideon.com/).
