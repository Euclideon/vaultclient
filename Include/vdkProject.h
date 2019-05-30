#ifndef vdkProject_h__
#define vdkProject_h__

// Follows the https://tools.ietf.org/html/rfc7946 specifications
// All nodes are a Geometry, a Feature or a collection of features

// TODO:
//   We need to factor in Antimeridian Cutting (Section 3.1.9)
//   Ensure we handle the poles correctly (Section 5.3)

#include "vdkDLLExport.h"
#include "vdkError.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  struct vdkContext;
  struct vdkProject;

  enum vdkProjectNodeType
  {
    // Geometry Types
    vdkPNT_Point, // pCoordinates is a single 3D position
    vdkPNT_MultiPoint, // Array of vdkPNT_Point, pCoordinates is an array of 3D positions
    vdkPNT_LineString, // pCoordinates is an array of 3D positions forming an open line
    vdkPNT_MultiLineString, // Array of vdkPNT_LineString; pCoordinates is NULL and children will be present
    vdkPNT_Polygon, // Much more complex. pCoordinates will be a closed linear ring (the outside), there MAY be children that are interior as pChildren vdkPNT_MultiLineString items, these should be counted as islands of the external ring.
    vdkPNT_MultiPolygon, // pCoordinates is null, array of vdkPNT_Polygon
    vdkPNT_GeometryCollection, // Array of geometries; pCoordinates is NULL and children may be present

    // Non-Geometry Features
    vdkPNT_Feature, // pCoordinates will be null and there may be a child geometry
    vdkPNT_FeatureCollection, // Array of vdkPNT_Feature, pCoordinates will be null and may be children (this is used for folders)

    vdkPNT_Count
  };

  struct vdkProjectNode
  {
    vdkProjectNodeType type;

    char UUID[37]; // Unique identifier for this node "id"
    char itemtype[8]; // "POI", "UDS", etc.
    const char *pName; // Human readable name
    const char *pURI; // Some address or filename for where to find the resource

    int64_t lastUpdate; // The last time this node was updated

    const char *pPropertiesJSON; // This is the JSON metadata- general suggestion is to cache this somehow in the pUserData

    // Bounding Box
    bool hasBoundingBox; // "bbox" element
    double boundingBox[6]; // [West, South, Floor, East, North, Ceiling]

    // Geometry Info
    int geomCount;
    double *pCoordinates;

    // This is the next item in the project (NULL if no further siblings)
    vdkProjectNode *pNextSibling;

    // Some types ("folder" and "transform") have children nodes, NULL if there are no children.
    // Technically these is no limitation on what types have children but how they are handled might not be consistent
    vdkProjectNode *pFirstChild;

    // User Data
    void *pUserData; // This is an application specific storage system
  };

  // Create/Load functions
  VDKDLL_API enum vdkError vdkProject_CreateInServer(struct vdkContext *pContext, struct vdkProject **ppProject, const char *pName, const char *pDescription, const char *pGroupUUID); // Creates an empty project on the server
  VDKDLL_API enum vdkError vdkProject_CreateLocal(struct vdkContext *pContext, struct vdkProject **ppProject, const char *pName, const char *pDescription); // Creates an empty local project
  VDKDLL_API enum vdkError vdkProject_LoadFromServer(struct vdkContext *pContext, struct vdkProject **ppProject, const char *projectUUID); // Loads from the server
  VDKDLL_API enum vdkError vdkProject_LoadFromMemory(struct vdkContext *pContext, struct vdkProject **ppProject, const char *pGeoJSON); // Loads project from GeoJSON

  // Unloader
  VDKDLL_API enum vdkError vdkProject_Release(struct vdkProject **ppProject);

  // Save to disk function
  VDKDLL_API enum vdkError vdkProject_WriteToDisk(struct vdkProject *pProject, const char *pFilename); // Saves the project to disk

  // Project Accessors
  VDKDLL_API enum vdkError vdkProject_GetRootNode(struct vdkProject *pProject, const struct vdkProjectNode **ppRootNode);

  VDKDLL_API enum vdkError vdkProject_AddNodeToEndOfProject(struct vdkProject *pProject, const char *pType, const char *pName, const char *pURI, void *pUserData);

  // Project Node Accessors


#ifdef __cplusplus
}
#endif

#endif // vdkProject_h__
