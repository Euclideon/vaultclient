Euclideon Vault Client Version History

Version 0.1.1 (Incoming)
  - Improved camera panning to move based on hovered point (EVA-289)
  - UDS, UDG, SSF and UDM files are immediately added to scene and then processed in the background
  - Login and Keep alive are handled in the background with better error messages (EVA-388)
  - Watermarks are now only submitted to the GPU when required (speeds up UDS load times)
  - Model load status is now displayed in the UI (pending, loading & failure)
  - Added standard metadata tags from Geoverse Convert (Author, Comment, Copyright and License)
  - Added a clear scene option to project menu (EVA-143 & EVA-384)
  - Fixed issue with invert X and invert Y not affecting orbit mode
  - Fixed a number of minor memory leaks
  - Fixed an issue where background workers would occasionally get distracted and stop working
  - Added scroll wheel binding so users of Geoverse MDM can use the scroll wheel to set move speed
  - Doubled the max far plane to near plane ratio (now 20000) (EVA-396)
  - Added release notes (EVA-405)
  - Various improvements fixed via a VDK update
    - CSV total points are updated at end of processing (EVA-362)
    - Improved error detection during initial connection process
    - UDS files are unlocked in the OS after being removed from the scene (Resolves EVA-382)

Version 0.1.0
  - Initial Public Release
