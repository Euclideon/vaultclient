#include "vcUDP.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udJSON.h"

void vcUDP_AddModel(vcState *pProgramState, const char *pUDPFilename, const char *pModelFilename, bool firstLoad, udDouble3 *pPosition, udDouble3 *pYPR, double scale)
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

  vcModel_AddToList(pProgramState, file, firstLoad, pPosition, pYPR, scale);
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
        vcUDP_AddModel(pProgramState, pFilename, dataEntries.Get("[%zu].content", i).AsString(), firstLoad, nullptr, nullptr, 1.0);
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

            const char *pName = nullptr;
            const char *pPath = nullptr;
            const char *pLocation = nullptr;
            const char *pAngleX = nullptr;
            const char *pAngleY = nullptr;
            const char *pAngleZ = nullptr;

            for (size_t k = 0; k < datasetData.ArrayLength(); k++)
            {
              if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Name"))
                pName = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Path"))
                pPath = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Location"))
                pLocation = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleX"))
                pAngleX = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleY"))
                pAngleY = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleZ"))
                pAngleZ = datasetData.Get("[%zu].content", k).AsString();
            }

            if (pPath != nullptr)
            {
              udDouble3 position = udDouble3::zero();
              udDouble3 ypr = udDouble3::zero();
              double scale = 1.0;

              udDouble3 *pPosition = nullptr;
              udDouble3 *pYPR = nullptr;

              if (pLocation != nullptr)
              {
                //TODO: Handle the SRID that lives here as well for some reason

                int offset = 0;
                int offset2 = 0;
                position.x = udStrAtof64(pLocation, &offset);
                position.y = udStrAtof64(&pLocation[offset + 1], &offset2);
                position.z = udStrAtof64(&pLocation[offset + offset2 + 2]);

                pPosition = &position;
              }

              if (pAngleX != nullptr || pAngleY != nullptr || pAngleZ != nullptr) // At least one of the angles is set
              {
                if (pAngleZ != nullptr)
                  ypr.x = UD_DEG2RAD(udStrAtof64(pAngleZ));

                if (pAngleX != nullptr)
                  ypr.y = UD_DEG2RAD(udStrAtof64(pAngleX));

                if (pAngleY != nullptr)
                  ypr.z = UD_DEG2RAD(udStrAtof64(pAngleY));

                pYPR = &ypr;
              }

              vcUDP_AddModel(pProgramState, pFilename, pPath, firstLoad, pPosition, pYPR, scale);
              firstLoad = false;
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
