#include "vcCSV.h"

#include "udPlatform/udFile.h"
#include "vcPOI.h"

vdkError vcCSV_LoadCSV(vcState *pProgramState, const char *pFilename)
{
  // Improperly formatted input has undefined behavior
  char *pFile;
  if (udFile_Load(pFilename, (void **)&pFile) != udR_Success)
    return vE_OpenFailure;

  char *pPos = pFile;

  size_t index;
  if (*pPos == '\0')
    return vE_ReadFailure;

  udStrchr(pPos, "\n", &index);
  pPos += index + 1;

  while (*pPos != '\0')
  {
    udDouble3 position;
    const char *pName;
    int epsgcode;
    const char *pNotes;

    int i;
    ++pPos;

    for (i = 0; i < 32 && *(pPos + i) != '\"'; ++i);
    pName = udStrndup(pPos, i);
    pPos += i + 2;

    position.x = udStrAtof(pPos, &i);
    pPos += i + 1;
    position.y = udStrAtof(pPos, &i);
    pPos += i + 1;
    position.z = udStrAtof(pPos, &i);
    pPos += i + 1;

    // ID / Parent ID
    udStrchr(pPos, ",", &index);
    pPos += index + 1;
    udStrchr(pPos, ",", &index);
    pPos += index + 1;

    udStrchr(pPos, ",", &index);
    pNotes = udStrndup(pPos, index);
    pPos += index + 1;

    epsgcode = udStrAtoi(pPos, &i);
    pPos += i + 1;

    // POV Direction Yaw / Pitch
    udStrchr(pPos, ",", &index);
    pPos += index + 1;
    udStrchr(pPos, "\n", &index);
    pPos += index + 1;

    udStrSkipWhiteSpace(pPos);

    vcPOI_AddToList(pProgramState, pName, 0, 0, position, epsgcode, pNotes);
    udFree(pName);
    udFree(pNotes);
  }
  udFree(pFile);

  return vE_Success;
}
