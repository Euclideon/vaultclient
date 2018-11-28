#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udThread.h"
#include "vdkConvert.h"
#include "vdkContext.h"
#include "udFile.h"
#include "stdio.h"

#define CONVERTCMD_VERSION "0.2.0"

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2018
int main(int argc, const char **ppArgv)
{
  udMemoryDebugTrackingInit();
  bool cmdlineError = false;
  bool autoOverwrite = false;
  bool pause = false;
  bool pauseOnError = false;
  vdkError convResult = vE_Success;

  vdkContext *pContext = nullptr;
  const vdkConvertInfo *pInfo = nullptr;
  vdkConvertContext *pModel = nullptr;

  vdkContext_Connect(&pContext, ppArgv[1], "vdkConvertCMD", ppArgv[2], ppArgv[3]);

  vdkConvertItemInfo itemInfo;

  vdkConvert_CreateContext(pContext, &pModel);

  vdkConvert_GetInfo(pContext, pModel, &pInfo);

  vdkConvert_SetOutputFilename(pContext, pModel, "Converted");
  vdkConvert_SetTempDirectory(pContext, pModel, "TempData");

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
    /*else if (udStrEquali(ppArgv[i], "-licence"))
    {
      if (conv.SetLicence(ppArgv[i + 1]) != udR_Success)
        cmdlineError = true;
      i += 2;
    }*/
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
          convResult = vdkConvert_AddItem(pContext, pModel, foundFile.GetPath());
          if (convResult != vE_Success)
            printf("Unable to convert %s:\n", foundFile.GetPath());
        } while (FindNextFileA(h, &fd));
        FindClose(h);
      }
      else
      {
        vdkConvert_AddItem(pContext, pModel, ppArgv[i + 1]);
      }
      i += 2;
#else //Non-Windows
      ++i; // Skip "-i"
      do
      {
        convResult = vdkConvert_AddItem(pContext, pModel, ppArgv[i++]);
        if (convResult != vE_Success)
        {
          printf("Unable to convert %s:\n", ppArgv[i]);
        }
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
      const char *pExpireDateStr = nullptr;
      const int maxDays = 60;
      int days = udDaysUntilExpired(maxDays, &pExpireDateStr);
      if (days <= 0)
        exit(printf("Build expired on %s\n", pExpireDateStr));
      if (days <= 7)
        printf("WARNING: build will expire in %d days on %s\n", days, pExpireDateStr);

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
    printf("vcConvertCMD %s\n", CONVERTCMD_VERSION);
    printf("Usage: vcConvertCMD [options] -i inputfile [-i anotherInputFile] -o outputfile.uds\n");
    printf("   input file types supported: .las, .uds\n");
    printf("   -resolution <res>   - override the resolution (0.01 = 1cm, 0.001 = 1mm)\n");
    printf("   -srid <sridCode>    - override the srid code for geolocation\n");
    printf("   -nopalette          - Store the color attribute as lossless 24-bit rather than compressing to a palette per block\n");
    printf("   -uniquelod          - disables level of detail blending (matching Geoverse Convert result - not recommended)\n");
    printf("   -fastReconvert      - A fast mode when reconverting a single color-only UDS\n");
    printf("   -pause              - Require enter key to be pressed before exiting\n");
    printf("   -pauseOnError       - If any error occurs, require enter key to be pressed before exiting\n");
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

    for (size_t i = 0; i < pInfo->totalItems; ++i)
    {
      vdkConvert_GetItemInfo(pContext, pModel, i, &itemInfo);

      udFilename fn(itemInfo.pFilename);
    }

    convResult = vdkConvert_DoConvert(pContext, pModel);

    if (pInfo->pTempFilesPrefix)
      udCreateDir(pInfo->pTempFilesPrefix);
    printf("Converting: srid=%d attributes=", pInfo->srid);
    for (size_t i = 0; i < pInfo->totalItems; ++i)
    {
      vdkConvert_GetItemInfo(pContext, pModel, i, &itemInfo);
      printf("%s%s", i > 0 ? "," : "", itemInfo.pFilename);
    }
    printf("\n");
    size_t inputFilesRead = 0;
    for (; inputFilesRead < pInfo->currentInputItem; ++inputFilesRead)
    {
      vdkConvert_GetItemInfo(pContext, pModel, inputFilesRead, &itemInfo);
      printf("%s: %s/%s points read         \n", itemInfo.pFilename, udCommaInt(itemInfo.pointsRead), udCommaInt(itemInfo.pointsCount));
    }
    if (pInfo->currentInputItem < pInfo->totalItems)
    {
      printf("%s: %s/%s    \r", itemInfo.pFilename, udTempStr_CommaInt(itemInfo.pointsRead), udTempStr_CommaInt(itemInfo.pointsCount));
    }
    else
      printf("sourcePoints: %s, uniquePoints: %s, discardedPoints: %s, outputPoints: %s\r",
        udCommaInt(pInfo->sourcePointCount), udCommaInt(pInfo->uniquePointCount), udCommaInt(pInfo->discardedPointCount), udCommaInt(pInfo->outputPointCount));
    printf("\nOutput Filesize: %s\nPeak disk usage: %s\nTemp files: %s (%d)\n", udCommaInt(pInfo->outputFileSize), udCommaInt(pInfo->peakDiskUsage), udCommaInt(pInfo->peakTempFileUsage), pInfo->peakTempFileCount);

    if (pInfo->pTempFilesPrefix)
      udRemoveDir(pInfo->pTempFilesPrefix);

    printf("\nComplete.\n");
#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
  }
  udThread_DestroyCached();
#if __MEMORY_DEBUG__
  udSleep(2000); // Need to give threads a chance to exit
  udMemoryOutputLeaks();
#endif

  udMemoryDebugTrackingDeinit();

  if (pause || (pauseOnError && convResult != vE_Success))
  {
    printf("Press enter...");
    getchar();
  }

  vdkConvert_DestroyContext(pContext, &pModel);

  vdkContext_Disconnect(&pContext);
}
