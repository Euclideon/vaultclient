Euclideon Vault Client Version History

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
