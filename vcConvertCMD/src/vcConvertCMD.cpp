#define _CRT_SECURE_NO_WARNINGS

#include "udPlatformUtil.h"
#include "udThread.h"
#include "udStringUtil.h"
#include "udFile.h"
#include "udChunkedArray.h"
#include "udConvert.h"
#include "udContext.h"
#include "udConfig.h"
#include "udMath.h"
#include "udPointCloud.h"
#include "udQueryContext.h"

#include <stdio.h>
#include <inttypes.h>

#ifdef GIT_BUILD
#  define CONVERTCMD_VERSION "1.0." UDSTRINGIFY(GIT_BUILD)
#else
#  define CONVERTCMD_VERSION "1.0.DEVELOPER BUILD / DO NOT DISTRIBUTE"
#endif

void vcConvertCMD_ShowOptions()
{
  printf("Usage: udStreamConvertCMD [udStream server] [username] [password] [options] -i inputfile [additionInputFiles...] [-ilist file] [-region minX minY minZ maxX maxY maxZ] -o outputfile.uds\n");
  printf("   -i <inputFile>              - Add an input to the convert job. More files can be specified immediately after, folders will be searched for supported files recursively\n");
  printf("   -ilist <file>               - Add inputs from a text file that has 1 input file per line\n");
  printf("   -resolution <res>           - override the resolution (0.01 = 1cm, 0.001 = 1mm)\n");
  printf("   -srid <sridCode>            - override the srid code for geolocation\n");
  printf("   -globalOffset <x,y,z>       - add an offset to all points, no spaces allowed in the x,y,z value\n");
  printf("   -pause                      - Require enter key to be pressed before exiting\n");
  printf("   -pauseOnError               - If any error occurs, require enter key to be pressed before exiting\n");
  printf("   -proxyURL <url>             - Set the proxy URL\n");
  printf("   -proxyUsername <username>   - Set the username to use with the proxy\n");
  printf("   -proxyPassword <password>   - Set the password to use with the proxy\n");
  printf("   -copyright <details>        - Adds the copyright information to the \"Copyright\" metadata field\n");
  printf("   -quicktest                  - Does a small test to test if positioning/geolocation is correct\n");
  printf("   -region                     - Parameters to export the region. Exp: -region 0.0 0.0 0.0 1.0 1.0 1.0\n");
  printf("   -retainprimitives           - Include the primitives (triangles, etc) from supported formats\n");
}

struct vcConvertData
{
  udConvertContext *pConvertContext;

  bool ended;
  udError response;
};

uint32_t vcConvertCMD_DoConvert(void *pDataPtr)
{
  vcConvertData *pConvData = (vcConvertData*)pDataPtr;

  pConvData->response = udConvert_DoConvert(pConvData->pConvertContext);
  pConvData->ended = true;

  return 0;
}

struct vcConvertCMDSettings
{
  double resolution;
  const char *pOutputFilename;
  const char *pProxyURL;
  const char *pProxyUsername;
  const char *pProxyPassword;
  int32_t srid;
  double globalOffset[3];
  bool pause;
  bool pauseOnError;
  bool autoOverwrite;
  bool quicktest;
  bool retainPrimitives;

  const char *pCopyright;

  bool useRegion;
  udDouble3 regionMin;
  udDouble3 regionMax;

  udChunkedArray<const char*> files;
};

enum vcCCExitCodes
{
  vcCCEC_Success,
  vcCCEC_MissingOptions,
  vcCCEC_NoInputs,
  vcCCEC_SessionFailed,
  vcCCEC_ContextFailed,
  vcCCEC_InvalidRegion,
};

bool vcConvertCMD_ProcessCommandLine(int argc, const char **ppArgv, vcConvertCMDSettings *pSettings)
{
  bool cmdlineError = false;

  for (int i = 4; i < argc; )
  {
    if (udStrEquali(ppArgv[i], "-resolution") && i + 1 < argc)
    {
      pSettings->resolution = udStrAtof64(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-srid") && i + 1 < argc)
    {
      pSettings->srid = udStrAtoi(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-globalOffset") && i + 1 < argc)
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
    else if (udStrEquali(ppArgv[i], "-i") && i + 1 < argc) // This can read many inputs until another `-` is found
    {
      ++i; // Skip "-i"
      do
      {
        pSettings->files.PushBack(udStrdup(ppArgv[i++]));
      } while (i < argc && ppArgv[i][0] != '-'); // Continue until another option is seen
    }
    else if (udStrEquali(ppArgv[i], "-ilist") && i + 1 < argc)
    {
      char *pFileList = nullptr;
      size_t currentFileCount = pSettings->files.length;

      if (udFile_Load(ppArgv[i + 1], &pFileList) == udR_Success && udStrlen(pFileList) > 3)
      {
        char *pCarat = pFileList;
        const char *pFound = nullptr;
        size_t len = 0;

        do
        {
          pFound = udStrchr(pCarat, "\r\n", &len);
          if (len > 1)
          {
            pCarat[len] = '\0';
            pSettings->files.PushBack(udStrdup(pCarat));
          }

          pCarat += len + 1;
        } while (pCarat[0] != '\0');

        udFree(pFileList);
      }

      printf("Found %zu files listed in %s\n", pSettings->files.length - currentFileCount, ppArgv[i + 1]);
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
    else if (udStrEquali(ppArgv[i], "-quicktest"))
    {
      printf("Quicktest!\n");
      pSettings->quicktest = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-retainprimitives"))
    {
      pSettings->retainPrimitives = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-o") && i + 1 < argc)
    {
      // -O implies automatic overwrite, -o does a check
      pSettings->autoOverwrite = udStrEqual(ppArgv[i], "-O");
      pSettings->pOutputFilename = ppArgv[i + 1];

      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyURL") && i + 1 < argc)
    {
      pSettings->pProxyURL = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyUsername") && i + 1 < argc)
    {
      pSettings->pProxyUsername = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-proxyPassword") && i + 1 < argc)
    {
      pSettings->pProxyPassword = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-copyright") && i + 1 < argc)
    {
      pSettings->pCopyright = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-region") && i + 6 < argc)
    {
      pSettings->useRegion = true;
      pSettings->regionMin = udDouble3::create(udStrAtof64(ppArgv[i+1]), udStrAtof64(ppArgv[i+2]), udStrAtof64(ppArgv[i+3]));
      pSettings->regionMax = udDouble3::create(udStrAtof64(ppArgv[i+4]), udStrAtof64(ppArgv[i+5]), udStrAtof64(ppArgv[i+6]));

      i += 7;
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
  udError result = udE_Success;

  udContext *pContext = nullptr;
  const udConvertInfo *pInfo = nullptr;
  udConvertContext *pModel = nullptr;
  vcConvertCMDSettings settings = {};

  settings.files.Init(256);

  printf("udStreamConvertCMD %s\n", CONVERTCMD_VERSION);

  if (argc < 4)
  {
    vcConvertCMD_ShowOptions();
    exit(vcCCEC_MissingOptions);
  }

  bool cmdlineError = vcConvertCMD_ProcessCommandLine(argc, ppArgv, &settings);

  if (settings.files.length == 0)
  {
    printf("No inputs were entered!\n");
    exit(vcCCEC_NoInputs);
  }

  if (settings.useRegion && (settings.files.length > 2 || !udStrEndsWithi(settings.files[0], ".uds") || settings.pOutputFilename == nullptr))
  {
    printf("Region export only works on a single UDS file and requires an output filename!\n");
    exit(vcCCEC_InvalidRegion);
  }

  if (settings.pProxyURL)
    udConfig_ForceProxy(settings.pProxyURL);

  if (settings.pProxyUsername)
    udConfig_SetProxyAuth(settings.pProxyUsername, settings.pProxyPassword);

  result = udContext_TryResume(&pContext, ppArgv[1], "udStreamCMD", ppArgv[2], true);

  if (result != udE_Success)
  {
    result = udContext_Connect(&pContext, ppArgv[1], "udStreamCMD", ppArgv[2], ppArgv[3]);
    if (result == udE_ConnectionFailure)
      printf("Could not connect to server.");
    else if (result == udE_NotAllowed)
      printf("Username or Password incorrect.");
    else if (result == udE_OutOfSync)
      printf("Your clock doesn't match the remote server clock.");
    else if (result == udE_SecurityFailure)
      printf("Could not open a secure channel to the server.");
    else if (result == udE_ServerFailure)
      printf("Unable to negotiate with server, please confirm the server address");
    else if (result != udE_Success)
      printf("Unknown error occurred (Error=%d), please try again later.", result);

    if (result != udE_Success)
      exit(vcCCEC_SessionFailed);
  }

  // Region is just an export
  if (settings.useRegion)
  {
    udPointCloud *pPCModel = nullptr;
    udQueryFilter *pFilter = nullptr;

    udPointCloud_Load(pContext, &pPCModel, settings.files[0], nullptr);

    if (settings.regionMin != settings.regionMax)
    {
      udDouble3 centre = (settings.regionMin + settings.regionMax) / 2;
      udDouble3 halfSize = (settings.regionMax - settings.regionMin) / 2;
      udDouble3 ypr = udDouble3::zero();

      udQueryFilter_Create(&pFilter);
      udQueryFilter_SetAsBox(pFilter, &centre.x, &halfSize.x, &ypr.x);
    }

    if (udPointCloud_Export(pPCModel, settings.pOutputFilename, pFilter) == udE_Success)
      exit(vcCCEC_Success);

    printf("Export Failed?");

    exit(vcCCEC_InvalidRegion);
  }

  result = udConvert_CreateContext(pContext, &pModel);
  if (result != udE_Success)
  {
    printf("Could not created convert context");
    exit(vcCCEC_ContextFailed);
  }

  udConvert_GetInfo(pModel, &pInfo);

  // Process settings
  if (settings.resolution != 0)
    udConvert_SetPointResolution(pModel, true, settings.resolution);

  if (settings.srid != 0)
  {
    if (udConvert_SetSRID(pModel, true, settings.srid) != udE_Success)
    {
      printf("Error setting srid %d\n", settings.srid);
      cmdlineError = true;
    }
  }

  if (settings.globalOffset[0] != 0.0 || settings.globalOffset[1] != 0.0 || settings.globalOffset[2] != 0.0)
  {
    if (udConvert_SetGlobalOffset(pModel, settings.globalOffset) != udE_Success)
    {
      printf("Error setting global offset %1.1f,%1.1f,%1.1f\n", settings.globalOffset[0], settings.globalOffset[1], settings.globalOffset[2]);
      cmdlineError = true;
    }
  }

  if (settings.quicktest)
  {
    if (udConvert_SetEveryNth(pModel, 1000) != udE_Success)
      cmdlineError = true;
  }

  if (settings.retainPrimitives)
  {
    if (udConvert_SetRetainPrimitives(pModel, 1) != udE_Success)
      cmdlineError = true;
  }

  printf("Performing Initial Preprocess...\n");
  for (size_t i = 0; i < settings.files.length; ++i)
  {
    udFindDir *pFindDir = nullptr;
    udResult res = udOpenDir(&pFindDir, settings.files[i]);
    if (res == udR_Success)
    {
      do
      {
        if (pFindDir->isDirectory)
          continue;

        udFilename foundFile(pFindDir->pFilename);
        foundFile.SetFolder(settings.files[i]);
        printf("\tPreprocessing '%s'... [%" PRIu64 " total items]\n", foundFile.GetPath(), pInfo->totalItems);
        result = udConvert_AddItem(pModel, foundFile.GetPath());
        if (result != udE_Success)
          printf("\t\tUnable to convert %s [Error:%d]:\n", foundFile.GetPath(), result);
      } while (udReadDir(pFindDir) == udR_Success);
      res = udCloseDir(&pFindDir);
    }
    else
    {
      printf("\tPreprocessing '%s'... [%" PRIu64 " total items]\n", settings.files[i], pInfo->totalItems);
      result = udConvert_AddItem(pModel, settings.files[i]);
      if (result != udE_Success)
        printf("\t\tUnable to convert %s [Error:%d]:\n", settings.files[i], result);
    }

    udFree(settings.files[i]);
  }

  if (settings.pOutputFilename)
    udConvert_SetOutputFilename(pModel, settings.pOutputFilename);

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
    convdata.pConvertContext = pModel;

    printf("Converting--\n");

    if (settings.pCopyright != nullptr)
    {
      printf("\tCopyright: %s\n", settings.pCopyright);
      udConvert_SetMetadata(convdata.pConvertContext, "Copyright", settings.pCopyright);
    }

    printf("\tOutput: %s\n", pInfo->pOutputName);
    printf("\tTemp: %s\n", pInfo->pTempFilesPrefix);
    printf("\tTotal Input Files: %" PRIu64 "\n\n", pInfo->totalItems);

    udThread_Create(nullptr, vcConvertCMD_DoConvert, &convdata);

    udConvertItemInfo itemInfo = {};

    uint64_t previousItem = UINT64_MAX;

    while (!convdata.ended)
    {
      uint64_t currentItem = pInfo->currentInputItem; // Just copying this for thread safety

      if (currentItem == previousItem)
        printf("\r");
      else
        printf("\n");

      previousItem = currentItem;

      if (currentItem < pInfo->totalItems)
      {
        udConvert_GetItemInfo(pModel, currentItem, &itemInfo);
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
      udConvert_GetItemInfo(pModel, inputFilesRead, &itemInfo);
      printf("%s: %s/%s points read         \n", itemInfo.pFilename, udCommaInt(itemInfo.pointsRead), udCommaInt(itemInfo.pointsCount));
    }

    printf("\nOutput Filesize: %s\nPeak disk usage: %s\nTemp files: %s (%d)\n", udCommaInt(pInfo->outputFileSize), udCommaInt(pInfo->peakDiskUsage), udCommaInt(pInfo->peakTempFileUsage), pInfo->peakTempFileCount);

    if (pInfo->pTempFilesPrefix)
      udRemoveDir(pInfo->pTempFilesPrefix);

#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
  }

  printf("\nStatus: %d\n", (int)result);

  if (settings.pause || (settings.pauseOnError && result != udE_Success))
  {
    printf("Press enter...");
    getchar();
  }

  udConvert_DestroyContext(&pModel);
  udContext_Disconnect(&pContext, false);
  settings.files.Deinit();

  return -result;
}
