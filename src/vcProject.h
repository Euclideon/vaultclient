#ifndef vcProject_h__
#define vcProject_h__

#include "udProject.h"
#include "vcGIS.h"

struct vcState;
class vcSceneItem;
class vcFolder;
class udFilename;

struct vcProject
{
  udGeoZone baseZone;
  udProject *pProject;

  // These are the same item pFolder->m_pNode == pRoot && pRoot-pUserdata == pFolder
  udProjectNode *pRoot;
  vcFolder *pFolder;

  const char *pRelativeBase;
};

enum vcProjectStandardZones
{
  vcPSZ_NotGeolocated = 0,
  vcPSZ_StandardGeoJSON = 84,
  vcPSZ_WGS84ECEF = 4978
};

const char* vcProject_ErrorToString(udError error);

bool vcProject_CreateBlankScene(vcState *pProgramState, const char *pName, int srid);
udError vcProject_CreateFileScene(vcState *pProgramState, const char *pFileName, const char *pProjectName, int srid);
udError vcProject_CreateServerScene(vcState *pProgramState, const char *pName, const char *pGroupUUID, int srid);

bool vcProject_LoadFromURI(vcState *pProgramState, const char *pFilename);
bool vcProject_LoadFromServer(vcState *pProgramState, const char *pProjectID);

void vcProject_Deinit(vcState *pProgramData, vcProject *pProject);

void vcProject_AutoCompletedName(udFilename *exportFilename, const char *pProjectName, const char *pFileName);
bool vcProject_Save(vcState *pProgramState);
bool vcProject_SaveAs(vcState *pProgramState, const char *pPath, bool allowOverride);
udError vcProject_SaveAsServer(vcState *pProgramState, const char *pProjectID);

bool vcProject_AbleToChange(vcState *pProgramState);

void vcProject_RemoveItem(vcState *pProgramState, udProjectNode *pParent, udProjectNode *pNode);
void vcProject_RemoveSelected(vcState *pProgramState);

void vcProject_SelectItem(vcState *pProgramState, udProjectNode *pParent, udProjectNode *pNode);
void vcProject_UnselectItem(vcState *pProgramState, udProjectNode *pParent, udProjectNode *pNode);
void vcProject_ClearSelection(vcState *pProgramState, bool clearToolState = true);

bool vcProject_ContainsItem(udProjectNode *pParentNode, udProjectNode *pItem);
bool vcProject_UseProjectionFromItem(vcState *pProgramState, vcSceneItem *pItem);

bool vcProject_UpdateNodeGeometryFromCartesian(vcProject *pProject, udProjectNode *pNode, const udGeoZone &zone, udProjectGeometryType newType, udDouble3 *pPoints, int numPoints);
bool vcProject_UpdateNodeGeometryFromLatLong(vcProject *pProject, udProjectNode *pNode, udProjectGeometryType newType, udDouble3 *pPoints, int numPoints);

bool vcProject_FetchNodeGeometryAsCartesian(vcProject *pProject, udProjectNode *pNode, const udGeoZone &zone, udDouble3 **ppPoints, int *pNumPoints);

void vcProject_ExtractAttributionText(udProjectNode *pFolderNode, const char **ppCurrentText);

void vcProject_RemoveHistoryItem(vcState *pProgramState, size_t itemPosition);

inline udError vcProject_GetNodeMetadata(udProjectNode *pNode, const char *pMetadataKey, bool *pBool, bool defaultValue)
{
  uint32_t boolVal = defaultValue;
  udError ret = udProjectNode_GetMetadataBool(pNode, pMetadataKey, &boolVal, boolVal);
  (*pBool) = (boolVal != 0);
  return ret;
}

void vcProject_UpdateProjectInformationDisplayTextures(vcState *pProgramState);

#endif // vcProject_h__
