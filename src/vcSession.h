#ifndef vcSession_h__
#define vcSession_h__

#include "udMath.h"
#include "udUUID.h"
#include "udChunkedArray.h"

struct vcState;
struct vcTexture;

enum vcGroupVisibility
{
  vcGroupVisibility_Public,
  vcGroupVisibility_Internal,
  vcGroupVisibility_Private,

  vcGroupVisibility_Unknown
};

enum vcGroupPermissions
{
  vcGroupPermissions_Guest,
  vcGroupPermissions_Contractor,
  vcGroupPermissions_Editor,
  vcGroupPermissions_Manager,
  vcGroupPermissions_Owner
};

struct vcProjectInfo
{
  udUUID projectID;
  const char *pProjectName;
  bool isShared;
  double lastUpdated;

  bool isDeleted;
};

struct vcGroupInfo
{
  udUUID groupID;
  const char *pGroupName;
  const char *pDescription;
  vcGroupVisibility visibility;
  vcGroupPermissions permissionLevel;

  udChunkedArray<vcProjectInfo> projects;
};

struct vcFeaturedProjectInfo
{
  udUUID projectID;
  const char *pProjectName;
  vcTexture *pTexture;
};

void vcSession_Login(void *pProgramStatePtr);
void vcSession_Logout(vcState *pProgramState);

void vcSession_Resume(vcState *pProgramState);

void vcSession_UpdateInfo(void *pProgramStatePtr);

void vcSession_CleanupSession(vcState *pProgramState);

#endif // vcSession_h__
