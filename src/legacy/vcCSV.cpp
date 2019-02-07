#include "vcCSV.h"

#include "udPlatform/udFile.h"
#include "vcPOI.h"

enum
{
  vcCSV_ExpectedColumnCount = 10,
};

vdkError vcCSV_LoadCSV(vcState *pProgramState, const char *pFilename)
{
  udResult result = udR_Success;
  char *pFile = nullptr;
  char *pPos = nullptr;
  size_t index = 0;
  const char *pName = nullptr;
  const char *pNotes = nullptr;

  UD_ERROR_IF(udFile_Load(pFilename, (void **)&pFile) != udR_Success, udR_OpenFailure);
  pPos = pFile;
  UD_ERROR_IF(pPos == nullptr || *pPos == '\0', udR_ReadFailure);

  // Validate column count and skip header
  {
    size_t count = 0;
    while (*pPos != '\n')
    {
      if (*pPos == ',')
        ++count;
      ++pPos;
    }
    ++pPos;

    // There are N columns, but only N-1 commas
    UD_ERROR_IF(count != (vcCSV_ExpectedColumnCount - 1), udR_ParseError);
  }

  while (*pPos != '\0')
  {
    udDouble3 position;
    int epsgcode;

    int i;
    if (*pPos == '\"')
    {
      ++pPos;
      for (i = 0; i < 32 && *(pPos + i) != '\"'; ++i);
    }
    else
    {
      UD_ERROR_NULL(udStrchr(pPos, ",\n", &index), udR_ParseError);
      UD_ERROR_IF(pPos[index] != ',', udR_ParseError);
      i = (int)index;
    }
    pName = udStrndup(pPos, i);
    if (pPos[i] == '\"')
      pPos += i + 2;
    else
      pPos += i + 1;

    position.x = udStrAtof(pPos, &i);
    pPos += i + 1;
    position.y = udStrAtof(pPos, &i);
    pPos += i + 1;
    position.z = udStrAtof(pPos, &i);
    pPos += i + 1;

    // ID / Parent ID
    UD_ERROR_NULL(udStrchr(pPos, ",\n", &index), udR_ParseError);
    UD_ERROR_IF(pPos[index] != ',', udR_ParseError);
    pPos += index + 1;
    UD_ERROR_NULL(udStrchr(pPos, ",\n", &index), udR_ParseError);
    UD_ERROR_IF(pPos[index] != ',', udR_ParseError);
    pPos += index + 1;

    UD_ERROR_NULL(udStrchr(pPos, ",\n", &index), udR_ParseError);
    UD_ERROR_IF(pPos[index] != ',', udR_ParseError);
    pNotes = udStrndup(pPos, index);
    pPos += index + 1;

    epsgcode = udStrAtoi(pPos, &i);
    pPos += i + 1;

    // POV Direction Yaw / Pitch
    UD_ERROR_NULL(udStrchr(pPos, ",\n", &index), udR_ParseError);
    UD_ERROR_IF(pPos[index] != ',', udR_ParseError);
    pPos += index + 1;
    udStrchr(pPos, "\n", &index); // Could be \n or \0
    pPos += index;
    if (*pPos == '\n')
      ++pPos;

    udStrSkipWhiteSpace(pPos);

    vcPOI_AddToList(pProgramState, pName, 0xFFFFFFFF, 0, position, epsgcode, pNotes);
    udFree(pName);
    udFree(pNotes);
  }

epilogue:
  udFree(pName);
  udFree(pNotes);
  udFree(pFile);

  if (result == udR_Success)
    return vE_Success;
  else if (result == udR_OpenFailure)
    return vE_OpenFailure;
  else if (result == udR_ReadFailure)
    return vE_ReadFailure;
  else if (result == udR_ParseError)
    return vE_ParseError;
  else
    return vE_Failure;
}
