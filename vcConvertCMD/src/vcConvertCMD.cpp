#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udThread.h"
#include "udAsyncJob.h"
#include "udConvert.h"
#include "udOctree.h"
#include "udFile.h"
#include "stdio.h"

#define CONVERTCMD_VERSION "0.2.0"

// Recursively add inputs
void RecurseAddInput(udConvert &conv, udConvert::InputFile::SourceProjection sourceProjection, const char *pFolder)
{
  udFindDir *pFindDir = nullptr;
  if (udOpenDir(&pFindDir, pFolder) == udR_Success)
  {
    do
    {
      udFilename path;
      path.SetFilenameWithExt(pFindDir->pFilename);
      path.SetFolder(pFolder);
      if (pFindDir->isDirectory)
      {
        if (pFindDir->pFilename[0] != '.')
        {
          udDebugPrintf("Recursing %s\n", path.GetPath());
          RecurseAddInput(conv, sourceProjection, path.GetPath());
        }
      }
      else
      {
        if (conv.AddInput(path.GetPath()) == udR_Success)
        {
          if (sourceProjection != udConvert::InputFile::Source_Force32)
            conv.inputs[conv.inputs.length - 1].sourceProjection = sourceProjection;
          printf("Added %s                     \r", path.GetPath());
        }
      }
    } while (udReadDir(pFindDir) == udR_Success);
    udCloseDir(&pFindDir);
  }
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2018
int main(int argc, const char **ppArgv)
{
  udMemoryDebugTrackingInit();
  bool cmdlineError = false;
  bool autoOverwrite = false;
  bool pause = false;
  bool pauseOnError = false;
  udResult convResult = udR_NothingToDo;
  udConvert::InputFile::SourceProjection sourceProjection = udConvert::InputFile::Source_Force32;

  udConvert conv;
  conv.Init();
  udOctreeLoadInfo loadInfo;

  udFile_RegisterHTTP();
  udOctree_Init(nullptr); // Initialise streamer (required for UDG)
  memset(&loadInfo, 0, sizeof(loadInfo));

  for (int i = 1; i < argc; )
  {
    if (udStrEquali(ppArgv[i], "-iPassword"))
    {
      loadInfo.pModelPassword = ppArgv[i + 1];
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-oPassword"))
    {
      conv.passwordProtect = true;
      udStrcpy(conv.password, sizeof(conv.password), ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-resolution"))
    {
      conv.overridePointResolution = true;
      conv.pointResolution = udStrAtof64(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-srid"))
    {
      conv.overrideZone = true;
      int32_t srid = udStrAtoi(ppArgv[i + 1]);
      if (udGeoZone_SetFromSRID(&conv.zone, srid) != udR_Success)
      {
        printf("Error setting srid %d\n", srid);
        cmdlineError = true;
        break;
      }
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-every"))
    {
      conv.everyNth = udStrAtou(ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-recurse"))
    {
      RecurseAddInput(conv, sourceProjection, ppArgv[i + 1]);
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-watermark"))
    {
      if (conv.SetWatermark(ppArgv[i + 1]) != udR_Success)
        cmdlineError = true;
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-licence"))
    {
      if (conv.SetLicence(ppArgv[i + 1]) != udR_Success)
        cmdlineError = true;
      i += 2;
    }
    else if (udStrEquali(ppArgv[i], "-mem"))
    {
      conv.tempFilesThreshold = udStrAtou64(ppArgv[i + 1]) * 1024ULL * 1048576ULL;
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
          convResult = conv.AddInput(foundFile.GetPath());
          if (convResult != udR_Success)
            printf("Unable to convert %s: %s\n", foundFile.GetPath(), udResultAsString(convResult));
          if (sourceProjection != udConvert::InputFile::Source_Force32)
            conv.inputs[conv.inputs.length - 1].sourceProjection = sourceProjection;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
      }
      else
      {
        conv.AddInput(ppArgv[i + 1]);
      }
      i += 2;
#else //Non-Windows
      ++i; // Skip "-i"
      do
      {
        convResult = conv.AddInput(ppArgv[i++]);
        if (convResult != udR_Success)
        {
          printf("Unable to convert %s: %s\n", ppArgv[i], udResultAsString(convResult));
        }
        else
        {
          if (sourceProjection != udConvert::InputFile::Source_Force32)
            conv.inputs[conv.inputs.length - 1].sourceProjection = sourceProjection;
        }
      } while (i < argc && ppArgv[i][0] != '-'); // Continue until another option is seen
#endif
    }
    else if (udStrEquali(ppArgv[i], "-latlong"))
    {
      sourceProjection = udConvert::InputFile::SourceLatLong;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-longlat"))
    {
      sourceProjection = udConvert::InputFile::SourceLongLat;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-cartesian"))
    {
      sourceProjection = udConvert::InputFile::SourceCartesian;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-uniquelod"))
    {
      conv.generateLOD = false;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-fastReconvert"))
    {
      conv.fastReconvert = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-skipErrors"))
    {
      conv.skipErrorsWherePossible = true;
      ++i;
    }
    else if (udStrEquali(ppArgv[i], "-nopalette"))
    {
      conv.paletteiseColor = false;
      ++i;
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
      udStrcpy(conv.outputName, sizeof(conv.outputName), ppArgv[i + 1]);
      conv.GenerateTempFolder();
      i += 2;
    }
    else
    {
      printf("Unrecognised option: %s\n", ppArgv[i]);
      cmdlineError = true;
      break;
    }
  }

  if (cmdlineError || conv.inputs.length == 0)
  {
    printf("udConvertCMD %s\n", CONVERTCMD_VERSION);
    printf("Usage: udConvertCMD [options] -i inputfile [-i anotherInputFile] -o outputfile.uds\n");
    printf("   input file types supported: .las, .uds\n");
    printf("   -recurse <folder>   - Recursively add all files as an input from a folder\n");
    printf("   -watermark <file>   - Include image as a watermark for the output model\n");
    printf("   -licence <file>     - Include licence file in the output model\n");
    printf("   -iPassword <secret> - Password for reading already-encrypted input UDS file(s)\n");
    printf("   -oPassword <secret> - Encrypt output with specified password\n");
    printf("   -resolution <res>   - override the resolution (0.01 = 1cm, 0.001 = 1mm)\n");
    printf("   -srid <sridCode>    - override the srid code for geolocation\n");
    printf("   -nopalette          - Store the color attribute as lossless 24-bit rather than compressing to a palette per block\n");
    printf("   -every <n>          - Only process every n'th point (skip the others)\n");
    printf("   -mem <n>            - Use around n GB of ram before using temp files (default 50%% of ram)\n");
    printf("   -latlong            - assume future inputs are specified in lat,long\n");
    printf("   -longlat            - assume future inputs are specified in long,lat\n");
    printf("   -cartesian          - assume future inputs are specified in cartesian\n");
    printf("   -skipErrors         - skip minor errors where possible (eg parse errors)\n");
    printf("   -uniquelod          - disables level of detail blending (matching Geoverse Convert result - not recommended)\n");
    printf("   -fastReconvert      - A fast mode when reconverting a single color-only UDS\n");
    printf("   -pause              - Require enter key to be pressed before exiting\n");
    printf("   -pauseOnError       - If any error occurs, require enter key to be pressed before exiting\n");
  }
  else
  {
    if (!autoOverwrite && udFileExists(conv.outputName) == udR_Success)
    {
      printf("Output file %s exists, overwrite [Y/n]?", conv.outputName);
      int answer = getchar();
      if (answer != 'y' && answer != 'Y' && answer != '\n')
        exit(printf("Exiting\n"));
    }
#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_AWAYMODE_REQUIRED | ES_CONTINUOUS);
#endif
    conv.metadata.Set("convertInfo.program = 'udConvertCMD'");
    conv.metadata.Set("convertInfo.version = '%s'", CONVERTCMD_VERSION);
    for (size_t i = 0; i < conv.inputs.length; ++i)
    {
      udFilename fn(conv.inputs[i].pFilename);
      conv.metadata.Set("convertInfo.inputs[] = '%s'", fn.GetFilenameWithExt());
    }

    if (conv.fastReconvert)
    {
      if (conv.inputs.length != 1 || !conv.inputs[0].IsUDS() || conv.inputs[0].attributeCount != 1 || conv.inputs[0].attributes[0].typeInfo != udATI_color32)
      {
        printf("-fastReconvert option ignored: this only applied to a single color-only UDS input without the everyNth\n");
      }
      else
      {
        // Change some settings to ensure the model data isn't moved (this would change the morton order if it was)
        conv.range = (1ULL << conv.inputs[0].octreeHeader.octreeDepth) - 1;
        conv.minPos = udDouble3::create(0,0,0);
      }
    }

    udAsyncJob *pJob = nullptr;
    udAsyncJob_Create(&pJob);
    convResult = conv.Convert(pJob);
    if (conv.writerOptions.pTempFilesPrefix)
      udCreateDir(conv.writerOptions.pTempFilesPrefix);
    printf("Converting: srid=%d depth=%d attributes=", conv.zone.srid, conv.writerOptions.headerData.octreeDepth);
    for (size_t i = 0; i < conv.outputAttributes.length; ++i)
      printf("%s%s", i > 0 ? "," : "", conv.outputAttributes[i].name);
    printf("\n");
    size_t inputFilesRead = 0;
    while (!udAsyncJob_GetResultTimeout(pJob, &convResult, 2000))
    {
      for (; inputFilesRead < conv.currentInputIndex; ++inputFilesRead)
        printf("%s: %s/%s points read         \n", conv.inputs[inputFilesRead].pFilename, udCommaInt(conv.inputs[inputFilesRead].pointsRead), udCommaInt(conv.inputs[inputFilesRead].pointsCount));
      if (conv.currentInputIndex < conv.inputs.length)
        printf("%s: %s/%s depth=%d    \r", conv.inputs[inputFilesRead].pFilename, udTempStr_CommaInt(conv.inputs[inputFilesRead].pointsRead), conv.everyNth ? udTempStr_CommaInt(conv.inputs[inputFilesRead].pointsCount / conv.everyNth) : udTempStr_CommaInt(conv.inputs[inputFilesRead].pointsCount), conv.writerOptions.headerData.octreeDepth);
      else
        printf("sourcePoints: %s, uniquePoints: %s, discardedPoints: %s, outputPoints: %s\r",
          udCommaInt(conv.writerStats.sourcePointCount), udCommaInt(conv.writerStats.uniquePointCount), udCommaInt(conv.writerStats.discardedPointCount), udCommaInt(conv.writerStats.outputPointCount));
    }
    printf("\nOutput Filesize: %s\nPeak disk usage: %s\nTemp files: %s (%d)\n", udCommaInt(conv.writerStats.outputFileSize), udCommaInt(conv.writerStats.peakDiskUsage), udCommaInt(conv.writerStats.peakTempFileUsage), conv.writerStats.peakTempFileCount);

    if (conv.writerOptions.pTempFilesPrefix)
      udRemoveDir(conv.writerOptions.pTempFilesPrefix);

    printf("\nComplete. Result = %s\n", udResultAsString(convResult));
#if UDPLATFORM_WINDOWS
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
    udAsyncJob_Destroy(&pJob);
  }
  conv.Deinit();
  udOctree_Deinit();
  udThread_DestroyCached();
#if __MEMORY_DEBUG__
  udSleep(2000); // Need to give threads a chance to exit
  udMemoryOutputLeaks();
#endif

  udMemoryDebugTrackingDeinit();

  if (pause || (pauseOnError && convResult != udR_Success))
  {
    printf("Press enter...");
    getchar();
  }
}

