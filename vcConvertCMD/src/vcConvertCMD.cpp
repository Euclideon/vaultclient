#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udThread.h"
#include "udStringUtil.h"
#include "udFile.h"

#include "vdkConvert.h"
#include "vdkContext.h"
#include "vdkConfig.h"

#include <stdio.h>
#include <inttypes.h>

#ifdef GIT_BUILD
#  define CONVERTCMD_VERSION "0.2.0." UDSTRINGIFY(GIT_BUILD)
#else
#  define CONVERTCMD_VERSION "0.2.0.DEVELOPER BUILD / DO NOT DISTRIBUTE"
#endif

void vcConvertCMD_ShowOptions()
{
  printf("Usage: vcConvertCMD [vault server] [username] [password] [options] -i inputfile [-i anotherInputFile] -o outputfile.uds\n");
  printf("   -resolution <res>           - override the resolution (0.01 = 1cm, 0.001 = 1mm)\n");
  printf("   -srid <sridCode>            - override the srid code for geolocation\n");
  printf("   -globalOffset <x,y,z>       - add an offset to all points, no spaces allowed in the x,y,z value\n");
  printf("   -pause                      - Require enter key to be pressed before exiting\n");
  printf("   -pauseOnError               - If any error occurs, require enter key to be pressed before exiting\n");
  printf("   -proxyURL <url>             - Set the proxy URL\n");
  printf("   -proxyUsername <username>   - Set the username to use with the proxy\n");
  printf("   -proxyPassword <password>   - Set the password to use with the proxy\n");
}

struct vcConvertData
{
  vdkContext *pContext;
  vdkConvertContext *pConvertContext;

  bool ended;
  vdkError response;
};

uint32_t vcConvertCMD_DoConvert(void *pDataPtr)
{
  vcConvertData *pConvData = (vcConvertData*)pDataPtr;

  pConvData->response = vdkConvert_DoConvert(pConvData->pContext, pConvData->pConvertContext);
  pConvData->ended = true;

  return 0;
}

struct vcConvertCMDSettings
{
  double resolution;
  const char *pWatermark;
  const char *pOutputFilename;
  const char *pProxyURL;
  const char *pProxyUsername;
  const char *pProxyPassword;
  const char **ppInputFiles;
  int32_t inputFileCount;
  int32_t srid;
  double globalOffset[3];
  bool pause;
  bool pauseOnError;
  bool autoOverwrite;
};

bool vcConvertCMD_ProcessCommandLine(int argc, const char **ppArgv, vcConvertCMDSettings *pSettings)
{
  bool cmdlineError = false;
  int32_t currentInputFile = 0;

  for (int i = 4; i < argc; )
  {
    if (udStrEquali(ppArgv[i], "-resolution"))
    {
      pSettings->resolution = udStrAtof64(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-srid"))
    {
      pSettings->srid = udStrAtoi(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-globalOffset"))
    {
      const char *pStr = ppArgv[i + 1];
      for (int j = 0; j < 3; ++j)
      {
        int charCount;
        pSettings->globalOffset[j] = udStrAtof64(pStr, &charCount);
        if (charCount == 0 || (j < 2 && pStr[charCount] != ','))
        {
          printf("Error parsing component %d of option -globalOffset %s", j, ppArgv[i + 1]);
          cmdlineError = true;
          break;
        }
        pStr += charCount + 1; // +1 for , separator
      }
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-watermark"))
    {
      pSettings->pWatermark = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-i"))
    {
      // First time encountering input files, count them
      if (pSettings->inputFileCount == 0)
      {
        for (int j = i; j < argc; j++)
        {
          if (udStrEquali(ppArgv[j], "-i"))
            ++pSettings->inputFileCount;
        }
        pSettings->ppInputFiles = udAllocType(const char *, pSettings->inputFileCount, udAF_None);
      }
      pSettings->ppInputFiles[currentInputFile] = ppArgv[i + 1];
      ++currentInputFile;

      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-pause"))
    {
      pSettings->pause = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-pauseOnError"))
    {
      pSettings->pauseOnError = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-o"))
    {
      // -O implies automatic overwrite, -o does a check
      pSettings->autoOverwrite = udStrEqual(ppArgv[i], "-O");
      pSettings->pOutputFilename = ppArgv[i + 1];

      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyURL"))
    {
      pSettings->pProxyURL = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyUsername"))
    {
      pSettings->pProxyUsername = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyPassword"))
    {
      pSettings->pProxyPassword = ppArgv[i + 1];
      i += 2;
    }
    else
    {
      printf("Unrecognised option: %s\n", ppArgv[i]);
      cmdlineError = true;
      break;
    }
  }

  return cmdlineError;
}

int main(int argc, const char **ppArgv)
{
  vdkError result = vE_Success;

  vdkContext *pContext = nullptr;
  const vdkConvertInfo *pInfo = nullptr;
  vdkConvertContext *pModel = nullptr;
  vcConvertCMDSettings settings = {};

  printf("vcConvertCMD %s\n", CONVERTCMD_VERSION);

  if (argc < 4)
  {
    vcConvertCMD_ShowOptions();
    exit(1);
  }

  bool cmdlineError = vcConvertCMD_ProcessCommandLine(argc, ppArgv, &settings);

  if (settings.pProxyURL)
    vdkConfig_ForceProxy(settings.pProxyURL);

  if (settings.pProxyUsername)
    vdkConfig_SetProxyAuth(settings.pProxyUsername, settings.pProxyPassword);

  result = vdkContext_Connect(&pContext, ppArgv[1], "vcConvertCMD", ppArgv[2], ppArgv[3]);
  if (result == vE_ConnectionFailure)
    printf("Could not connect to server.");
  else if (result == vE_NotAllowed)
    printf("Username or Password incorrect.");
  else if (result == vE_OutOfSync)
    printf("Your clock doesn't match the remote server clock.");
  else if (result == vE_SecurityFailure)
    printf("Could not open a secure channel to the server.");
  else if (result == vE_ServerFailure)
    printf("Unable to negotiate with server, please confirm the server address");
  else if (result != vE_Success)
    printf("Unknown error occurred (Error=%d), please try again later.", result);

  if (result != vE_Success)
    exit(2);

  result = vdkConvert_CreateContext(pContext, &pModel);
  if (result != vE_Success)
  {
    printf("Could not created render context");
    exit(3);
  }

  result = vdkContext_RequestLicense(pContext, vdkLT_Convert);
  if (result != vE_Success)
  {
    printf("No licenses available");
    exit(4);
  }

  vdkConvert_GetInfo(pContext, pModel, &pInfo);

  // Process settings
  if (settings.resolution != 0)
    vdkConvert_SetPointResolution(pContext, pModel, true, settings.resolution);

  if (settings.srid != 0)
  {
    if (vdkConvert_SetSRID(pContext, pModel, true, settings.srid) != vE_Success)
    {
      printf("Error setting srid %d\n", settings.srid);
      cmdlineError = true;
    }
  }

  if (settings.globalOffset[0] != 0.0 || settings.globalOffset[1] != 0.0 || settings.globalOffset[2] != 0.0)
  {
    if (vdkConvert_SetGlobalOffset(pContext, pModel, settings.globalOffset) != vE_Success)
    {
      printf("Error setting global offset %1.1f,%1.1f,%1.1f\n", settings.globalOffset[0], settings.globalOffset[1], settings.globalOffset[2]);
      cmdlineError = true;
    }
  }

  if (settings.pWatermark)
  {
    if (vdkConvert_AddWatermark(pContext, pModel, settings.pWatermark) != vE_Success)
      cmdlineError = true;
  }

  for (int i = 0; i < settings.inputFileCount; ++i)
  {
    udFindDir *pFindDir = nullptr;
    udResult res = udOpenDir(&pFindDir, settings.ppInputFiles[i]);
    if (res == udR_Success)
    {
      do
      {
        if (pFindDir->isDirectory)
          continue;

        udFilename foundFile(pFindDir->pFilename);
        foundFile.SetFolder(settings.ppInputFiles[i]);
        result = vdkConvert_AddItem(pContext, pModel, foundFile.GetPath());
        if (result != vE_Success)
          printf("Unable to convert %s [Error:%d]:\n", foundFile.GetPath(), result);
      } while (udReadDir(pFindDir) == udR_Success);
      res = udCloseDir(&pFindDir);
    }
    else
    {
      result = vdkConvert_AddItem(pContext, pModel, settings.ppInputFiles[i]);
      if (result != vE_Success)
        printf("Unable to convert %s [Error:%d]:\n", settings.ppInputFiles[i], result);
    }
  }

  if (settings.pOutputFilename)
    vdkConvert_SetOutputFilename(pContext, pModel, settings.pOutputFilename);

  if (cmdlineError || pInfo->totalItems == 0)
  {
    vcConvertCMD_ShowOptions();
  }
  else
  {
    if (!settings.autoOverwrite && udFileExists(pInfo->pOutputName) == udR_Success)
    {
      printf("Output file %s exists, overwrite [Y/n]?", pInfo->pOutputName);
      int answer = getchar();
      if (answer != 'y' && answer != 'Y' && answer != '\n')
        exit(printf("Exiting\n"));
    }

#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_AWAYMODE_REQUIRED | ES_CONTINUOUS);
#endif

    vcConvertData convdata = {};
    convdata.pContext = pContext;
    convdata.pConvertContext = pModel;

    printf("Converting--\n");
    printf("\tOutput: %s\n", pInfo->pOutputName);
    printf("\tTemp: %s\n", pInfo->pTempFilesPrefix);
    printf("\t# Inputs: %" PRIu64 "\n\n", pInfo->totalItems);

    udThread_Create(nullptr, vcConvertCMD_DoConvert, &convdata);

    vdkConvertItemInfo itemInfo = {};

    while (!convdata.ended)
    {
      uint64_t currentItem = pInfo->currentInputItem; // Just copying this for thread safety

      if (currentItem < pInfo->totalItems)
      {
        vdkConvert_GetItemInfo(pContext, pModel, currentItem, &itemInfo);
        printf("[%" PRIu64 "/%" PRIu64 "] %s: %s/%s    \r", currentItem +1, pInfo->totalItems, itemInfo.pFilename, udTempStr_CommaInt(itemInfo.pointsRead), udTempStr_CommaInt(itemInfo.pointsCount));
      }
      else
      {
        printf("sourcePoints: %s, uniquePoints: %s, discardedPoints: %s, outputPoints: %s\r", udCommaInt(pInfo->sourcePointCount), udCommaInt(pInfo->uniquePointCount), udCommaInt(pInfo->discardedPointCount), udCommaInt(pInfo->outputPointCount));
      }

      udSleep(100);
    }

    printf("\n--\nConversion Ended!\n--\n");
    result = convdata.response;

    for (size_t inputFilesRead = 0; inputFilesRead < pInfo->currentInputItem; ++inputFilesRead)
    {
      vdkConvert_GetItemInfo(pContext, pModel, inputFilesRead, &itemInfo);
      printf("%s: %s/%s points read         \n", itemInfo.pFilename, udCommaInt(itemInfo.pointsRead), udCommaInt(itemInfo.pointsCount));
    }

    printf("\nOutput Filesize: %s\nPeak disk usage: %s\nTemp files: %s (%d)\n", udCommaInt(pInfo->outputFileSize), udCommaInt(pInfo->peakDiskUsage), udCommaInt(pInfo->peakTempFileUsage), pInfo->peakTempFileCount);

    if (pInfo->pTempFilesPrefix)
      udRemoveDir(pInfo->pTempFilesPrefix);

#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
  }

  if (settings.pause || (settings.pauseOnError && result != vE_Success))
  {
    printf("Press enter...");
    getchar();
  }

  vdkConvert_DestroyContext(pContext, &pModel);
  vdkContext_Disconnect(&pContext);

  return 0;
}
