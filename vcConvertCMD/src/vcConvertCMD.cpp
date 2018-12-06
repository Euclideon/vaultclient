#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udThread.h"
#include "vdkConvert.h"
#include "vdkContext.h"
#include "udFile.h"

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
  printf("   -resolution <res>   - override the resolution (0.01 = 1cm, 0.001 = 1mm)\n");
  printf("   -srid <sridCode>    - override the srid code for geolocation\n");
  printf("   -pause              - Require enter key to be pressed before exiting\n");
  printf("   -pauseOnError       - If any error occurs, require enter key to be pressed before exiting\n");
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

int main(int argc, const char **ppArgv)
{
  bool cmdlineError = false;
  bool autoOverwrite = false;
  bool pause = false;
  bool pauseOnError = false;

  vdkError result = vE_Success;

  vdkContext *pContext = nullptr;
  const vdkConvertInfo *pInfo = nullptr;
  vdkConvertContext *pModel = nullptr;

  printf("vcConvertCMD %s\n", CONVERTCMD_VERSION);

  if (argc < 4)
  {
    vcConvertCMD_ShowOptions();
    exit(1);
  }

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

  vdkConvertItemInfo itemInfo;

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

  for (int i = 4; i < argc; )
  {
    if (udStrEquali(ppArgv[i], "-resolution"))
    {
      vdkConvert_SetPointResolution(pContext, pModel, true, udStrAtof64(ppArgv[i + 1]));
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-srid"))
    {
      int32_t srid = udStrAtoi(ppArgv[i + 1]);
      if (vdkConvert_SetSRID(pContext, pModel, true, srid) != vE_Success)
      {
        printf("Error setting srid %d\n", srid);
        cmdlineError = true;
        break;
      }
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-watermark"))
    {
      if (vdkConvert_AddWatermark(pContext, pModel, ppArgv[i + 1]) != vE_Success)
        cmdlineError = true;
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-i"))
    {
#if UDPLATFORM_WINDOWS
      WIN32_FIND_DATAA fd;
      HANDLE h = FindFirstFileA(ppArgv[i + 1], &fd);
      if (h != INVALID_HANDLE_VALUE)
      {
        udFilename foundFile(ppArgv[i + 1]);
        do
        {
          foundFile.SetFilenameWithExt(fd.cFileName);
          result = vdkConvert_AddItem(pContext, pModel, foundFile.GetPath());
          if (result != vE_Success)
            printf("Unable to convert %s [Error:%d]:\n", foundFile.GetPath(), result);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
      }
      else
      {
        result = vdkConvert_AddItem(pContext, pModel, ppArgv[i + 1]);
        if (result != vE_Success)
          printf("Unable to convert %s [Error:%d]:\n", ppArgv[i + 1], result);
      }
      i += 2;
#else //Non-Windows
      ++i; // Skip "-i"
      do
      {
        result = vdkConvert_AddItem(pContext, pModel, ppArgv[i++]);
        if (result != vE_Success)
          printf("Unable to convert %s:\n", ppArgv[i]);
      } while (i < argc && ppArgv[i][0] != '-'); // Continue until another option is seen
#endif
    }
    else if (udStrEquali(ppArgv[i], "-pause"))
    {
      pause = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-pauseOnError"))
    {
      pauseOnError = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-o"))
    {
      // -O implies automatic overwrite, -o does a check
      autoOverwrite = udStrEqual(ppArgv[i], "-O");
      vdkConvert_SetOutputFilename(pContext, pModel, ppArgv[i + 1]);

      i += 2;
    }
    else
    {
      printf("Unrecognised option: %s\n", ppArgv[i]);
      cmdlineError = true;
      break;
    }
  }

  if (cmdlineError || pInfo->totalItems == 0)
  {
    vcConvertCMD_ShowOptions();
  }
  else
  {
    if (!autoOverwrite && udFileExists(pInfo->pOutputName) == udR_Success)
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

  if (pause || (pauseOnError && result != vE_Success))
  {
    printf("Press enter...");
    getchar();
  }

  vdkConvert_DestroyContext(pContext, &pModel);
  vdkContext_Disconnect(&pContext);

  return 0;
}
