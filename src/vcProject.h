#ifndef vcProject_h__
#define vcProject_h__

#include "vdkProject.h"
#include "vcGIS.h"

struct vcState;
class vcSceneItem;
class vcFolder;

struct vcProject
{
  udGeoZone baseZone;
  vdkProject *pProject;

  // These are the same item pFolder->m_pNode == pRoot && pRoot-pUserdata == pFolder
  vdkProjectNode *pRoot;
  vcFolder *pFolder;

  const char *pRelativeBase;
};

enum vcProjectStandardZones
{
  vcPSZ_NotGeolocated = 0,
  vcPSZ_StandardGeoJSON = 84,
  vcPSZ_WGS84ECEF = 4978
};

bool vcProject_CreateBlankScene(vcState *pProgramState, const char *pName, int srid);
bool vcProject_CreateFileScene(vcState *pProgramState, const char *pPath, const char *pName, int srid);
bool vcProject_CreateServerScene(vcState *pProgramState, const char *pName, const char *pGroupUUID, int srid);

bool vcProject_LoadFromURI(vcState *pProgramState, const char *pFilename);
bool vcProject_LoadFromServer(vcState *pProgramState, const char *pProjectID);

void vcProject_Deinit(vcState *pProgramData, vcProject *pProject);

void vcProject_Save(vcState *pProgramState, const char *pPath, bool allowOverride);

bool vcProject_AbleToChange(vcState *pProgramState);

void vcProject_RemoveItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode);
void vcProject_RemoveSelected(vcState *pProgramState);

void vcProject_SelectItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode);
void vcProject_UnselectItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode);
void vcProject_ClearSelection(vcState *pProgramState, bool clearToolState = true);

bool vcProject_ContainsItem(vdkProjectNode *pParentNode, vdkProjectNode *pItem);
bool vcProject_UseProjectionFromItem(vcState *pProgramState, vcSceneItem *pItem);

bool vcProject_UpdateNodeGeometryFromCartesian(vcProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, vdkProjectGeometryType newType, udDouble3 *pPoints, int numPoints);
bool vcProject_UpdateNodeGeometryFromLatLong(vcProject *pProject, vdkProjectNode *pNode, vdkProjectGeometryType newType, udDouble3 *pPoints, int numPoints);

bool vcProject_FetchNodeGeometryAsCartesian(vcProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, udDouble3 **ppPoints, int *pNumPoints);

void vcProject_ExtractAttributionText(vdkProjectNode *pFolderNode, const char **ppCurrentText);

#endif // vcProject_h__
