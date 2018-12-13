Euclideon Vault Client Version History

Version 0.2.0 (In Progress)
  - Added a menu bar for frequently changed settings (EVC-298)
  - Fixed some size issues with some of the modals (EVC-285)
  - Vault Convert CMD is packaged with Vault Client for the Windows and Ubuntu builds (EVC-304)
  - Various tile system optimizations and improvements (EVC-251)
  - Fixed an issue with the wrapping of the camera (EVC-299)
  - Full screen is no longer tracked in the settings to prevent issues with multiple monitor configurations (EVC-9)
  - Significant improvements to how the build in assets are handled internally (EVC-286)
  - Basic Geoverse MDM project (UDP file) support (EVC-287)
  - Proper wizard for changing the tile server and added support for JPG tiles (EVC-290 & EVC-206)
  - Fixed an issue with the mouse position being incorrect on Linux (EVC-27)
  - Better clearing of internal state during logout to prevent information leaking between sessions (EVC-283, EVC-284 & EVC-292)
  - Swapped the names of the DirectX and OpenGL builds on Windows to better clarify that the DirectX version is the more supported one (EVC-281)
  - The compass is no longer affected by FOV as this was causing confusion for some users (EVC-62)
  - Added Gizmos to move items around in the scene (EVC-288, EVC-297 & EVC-294)

Version 0.1.2
  - Seperated the project codes for Vault Server / Vault Development kit (EVA) from the Euclideon Vault Client codes (EVC)
  - Changed the rotation of the camera in the UI to display in degrees (EVC-70)
  - Added proxy support and added option to ignore peer and host certificate verification (EVA-391)
  - Added outlines to the first model in the scene when the diagnostic information is enabled (EVC-243)
  - Fixed an issue where the temp or output directories not being available caused convert to hand indefinately (EVC-115)
  - Fixed some bundling issues that prevented macOS 0.1.1 being released (EVC-153, EVC-88)
  - Fixed an issue with some convert settings being lost after reset (EVC-112)
  - Fixed some issues related to sessions not being kept alive correctly, leaving Client in a mixed state where its partially logged out (EVA-54, EVC-282)

Version 0.1.1
  - Now has UI to show when a new version is available
  - Improved camera panning to move based on hovered point (EVA-289)
  - UDS, UDG, SSF and UDM files are immediately added to scene and then processed in the background
  - Login and Keep alive are handled in the background with better error messages (EVA-388)
  - Watermarks are now only submitted to the GPU when required (speeds up UDS load times)
  - Added a mouse anchor style setting (defaults to orbit) and mouse anchor can also be turned off (EVA-410 and EVA-303)
  - Model load status is now displayed in the UI (pending, loading & failure)
  - Added standard metadata tags from Geoverse Convert (Author, Comment, Copyright and License)
  - Added a clear scene option to project menu (EVA-143 & EVA-384)
  - Added a warning when caps lock is enabled on the login screen (EVA-390)
  - Fixed issue with invert X and invert Y not affecting orbit mode
  - Fixed a number of minor memory leaks
  - Fixed an issue where background workers would occasionally get distracted and stop working
  - Fixed an issue where models with no attribute channels converted using convert were unable to be loaded in Geoverse MDM
  - When loading models you will now move to the location of the first file in the batch
  - Added new settings
    - Point Mode for different voxel modes, currently only supporting Rectangles and Cubes (EVA-169)
    - Allow mouse Interaction with map (when enabled, the mouse ray will intersect with the map; allowing orbit & pan to work on the map)
    - Added scroll wheel binding so users of Geoverse MDM can use the scroll wheel to set move speed
  - Doubled the max far plane to near plane ratio (now 20000) (EVA-396)
  - Added release notes (EVA-405)
  - Various improvements fixed via a VDK update
    - CSV total points are updated at end of processing (EVA-362)
    - Improved error detection during initial connection process
    - UDS files are unlocked in the OS after being removed from the scene (Resolves EVA-382)
  - Changed our version numbering system, now [Major].[Minor].[Revision].[BuildID]

Version 0.1.0
  - Initial Public Release
