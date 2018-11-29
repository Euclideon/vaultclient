#include "vcUDP.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udJSON.h"

void vcUDP_AddModel(vcState *pProgramState, const char *pUDPFilename, const char *pModelFilename, bool firstLoad)
{
  // If the model filename is nullptr there's nothing to load
  if (pModelFilename == nullptr)
    return;

  udFilename file(pUDPFilename);

  // If the path doesn't match the following patterns we should assume it's relative to the UDP:
  // <drive/protocol>:<path> (contains a ":")
  // /<path> (starts with "/")
  // \\<UNC Path> (starts with "\\")
  if (udStrchr(pModelFilename, ":") == nullptr && pModelFilename[0] != '/' && !udStrBeginsWith(pModelFilename, "\\\\"))
    file.SetFilenameWithExt(pModelFilename);
  else
    file.SetFromFullPath(pModelFilename);

  vcModel_AddToList(pProgramState, file, firstLoad);
}

void vcUDP_Load(vcState *pProgramState, const char *pFilename)
{
  udResult result;
  udJSON xml;
  const char *pXMLText = nullptr;
  bool firstLoad = true;
  UD_ERROR_CHECK(udFile_Load(pFilename, (void**)&pXMLText));
  UD_ERROR_CHECK(xml.Parse(pXMLText));

  if (xml.Get("DataBlock").IsObject() && udStrEqual(xml.Get("DataBlock.Name").AsString(), "ProjectData"))
  {
    const udJSON &dataEntries = xml.Get("DataBlock.DataEntry");
    for (size_t i = 0; i < dataEntries.ArrayLength(); i++)
    {
      const char *pName = dataEntries.Get("[%zu].Name", i).AsString();
      if (udStrEqual(pName, "AbsoluteModelPath"))
      {
        vcUDP_AddModel(pProgramState, pFilename, dataEntries.Get("[%zu].content", i).AsString(), firstLoad);
        firstLoad = false;
      }
    }

    const udJSON &dataBlocks = xml.Get("DataBlock.DataBlock");
    for (size_t i = 0; i < dataBlocks.ArrayLength(); i++)
    {
      if (udStrEqual(dataBlocks.Get("[%zu].Name", i).AsString(), "DataSetGroup"))
      {
        const udJSON &datasets = dataBlocks.Get("[%zu].DataBlock", i);
        for (size_t j = 0; j < datasets.ArrayLength(); j++)
        {
          if (udStrEqual(datasets.Get("[%zu].Name", j).AsString(), "DataSetData"))
          {
            const udJSON &datasetData = datasets.Get("[%zu].DataEntry", j);
            for (size_t k = 0; k < datasetData.ArrayLength(); k++)
            {
              if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Path"))
              {
                vcUDP_AddModel(pProgramState, pFilename, datasetData.Get("[%zu].content", k).AsString(), firstLoad);
                firstLoad = false;
              }
            }
          }
        }
      }
      // TODO: Add bookmark, label support here.
    }
  }

epilogue:
  udFree(pXMLText);
}
