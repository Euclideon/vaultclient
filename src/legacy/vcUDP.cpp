#include "vcUDP.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udJSON.h"

#include "vcModel.h"
#include "vcPOI.h"

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

bool vcUDP_ReadGeolocation(const char *pStr, udDouble3 &position, int &epsg)
{
#if UDPLATFORM_WINDOWS
  int count = sscanf_s(pStr, "%lf, %lf, %lf, %d", &position.x, &position.y, &position.z, &epsg);
#else
  int count = sscanf(pStr, "%lf, %lf, %lf, %d", &position.x, &position.y, &position.z, &epsg);
#endif

  return (count == 4);
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
    for (size_t i = 0; i < dataEntries.ArrayLength(); ++i)
    {
      const char *pName = dataEntries.Get("[%zu].Name", i).AsString();
      if (udStrEqual(pName, "AbsoluteModelPath"))
      {
        vcUDP_AddModel(pProgramState, pFilename, dataEntries.Get("[%zu].content", i).AsString(), firstLoad, nullptr, nullptr, 1.0);
        firstLoad = false;
      }
    }

    const udJSON &dataBlocks = xml.Get("DataBlock.DataBlock");
    for (size_t i = 0; i < dataBlocks.ArrayLength(); ++i)
    {
      if (udStrEqual(dataBlocks.Get("[%zu].Name", i).AsString(), "DataSetGroup"))
      {
        const udJSON &datasets = dataBlocks.Get("[%zu].DataBlock", i);
        for (size_t j = 0; j < datasets.ArrayLength(); ++j)
        {
          if (udStrEqual(datasets.Get("[%zu].Name", j).AsString(), "DataSetData"))
          {
            const udJSON &datasetData = datasets.Get("[%zu].DataEntry", j);

            //const char *pName = nullptr;
            const char *pPath = nullptr;
            const char *pLocation = nullptr;
            const char *pAngleX = nullptr;
            const char *pAngleY = nullptr;
            const char *pAngleZ = nullptr;
            const char *pScale = nullptr;

            for (size_t k = 0; k < datasetData.ArrayLength(); ++k)
            {
              if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Path"))
                pPath = datasetData.Get("[%zu].content", k).AsString();
              //else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Name"))
              //  pName = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Location"))
                pLocation = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleX"))
                pAngleX = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleY"))
                pAngleY = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "AngleZ"))
                pAngleZ = datasetData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(datasetData.Get("[%zu].Name", k).AsString(), "Scale"))
                pScale = datasetData.Get("[%zu].content", k).AsString();
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
                int epsgCode = 0;

                if (vcUDP_ReadGeolocation(pLocation, position, epsgCode))
                  pPosition = &position;

                udUnused(epsgCode); //TODO: Use this
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

              if (pScale != nullptr)
                scale = udStrAtof64(pScale);

              vcUDP_AddModel(pProgramState, pFilename, pPath, firstLoad, pPosition, pYPR, scale);
              firstLoad = false;
            }
          }
        }
      }
      else if (udStrEqual(dataBlocks.Get("[%zu].Name", i).AsString(), "LabelGroup"))
      {
        const udJSON &labels = dataBlocks.Get("[%zu].DataBlock", i);
        for (size_t j = 0; j < labels.ArrayLength(); ++j)
        {
          if (udStrEqual(labels.Get("[%zu].Name", j).AsString(), "Label"))
          {
            const udJSON &labelData = labels.Get("[%zu].DataEntry", j);

            const char *pName = nullptr;
            const char *pGeoLocation = nullptr;

            const char *pColour = nullptr;
            const char *pFontSize = nullptr;

            for (size_t k = 0; k < labelData.ArrayLength(); ++k)
            {
              if (udStrEqual(labelData.Get("[%zu].Name", k).AsString(), "Name"))
                pName = labelData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(labelData.Get("[%zu].Name", k).AsString(), "GeoLocation"))
                pGeoLocation = labelData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(labelData.Get("[%zu].Name", k).AsString(), "FontSize"))
                pFontSize = labelData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(labelData.Get("[%zu].Name", k).AsString(), "LabelColor"))
                pColour = labelData.Get("[%zu].content", k).AsString();
            }

            if (pName != nullptr && pGeoLocation != nullptr)
            {
              udDouble3 position = udDouble3::zero();
              int32_t epsgCode = 0;

              uint16_t size = (uint16_t)udStrAtou(pFontSize);
              uint32_t colour = (uint32_t)udStrAtoi(pColour); //These are stored as int (with negatives) in MDM

              if (vcUDP_ReadGeolocation(pGeoLocation, position, epsgCode))
                vcPOI_AddToList(pProgramState, pName, colour, size, position, epsgCode);
            }
          }
        }
      }
      else if (udStrEqual(dataBlocks.Get("[%zu].Name", i).AsString(), "PolygonGroup"))
      {
        const udJSON &polygons = dataBlocks.Get("[%zu].DataBlock", i);
        for (size_t j = 0; j < polygons.ArrayLength(); ++j)
        {
          if (udStrEqual(polygons.Get("[%zu].Name", j).AsString(), "PolygonData"))
          {
            const udJSON &polygonData = polygons.Get("[%zu].DataEntry", j);
            const udJSONArray *pNodeList = nullptr;

            const char *pName = nullptr;
            uint32_t colour = 0;
            bool isClosed = false;

            for (size_t k = 0; k < polygonData.ArrayLength(); ++k)
            {
              if (udStrEqual(polygonData.Get("[%zu].Name", k).AsString(), "PolygonName"))
                pName = polygonData.Get("[%zu].content", k).AsString();
              else if (udStrEqual(polygonData.Get("[%zu].Name", k).AsString(), "PolygonIsClosed"))
                isClosed = polygonData.Get("[%zu].content", k).AsBool();
              else if (udStrEqual(polygonData.Get("[%zu].Name", k).AsString(), "PolygonColour"))
                colour = (uint32_t)polygonData.Get("[%zu].content", k).AsInt(); // Stored as int in file, needs to be uint in vc
              else if (udStrEqual(polygonData.Get("[%zu].Name", k).AsString(), "Nodes"))
                pNodeList = polygonData.Get("[%zu].GeoLocationArray", k).AsArray();
            }

            if (pName != nullptr && pNodeList != nullptr)
            {
              int32_t epsgCode = 0;
              vcLineInfo info;

              memset(&info, 0, sizeof(info));

              info.pPoints = udAllocType(udDouble3, pNodeList->length + (isClosed ? 1 : 0), udAF_None);
              info.numPoints = 0;

              info.lineWidth = 1;
              info.lineColour = colour;

              for (size_t k = 0; k < pNodeList->length; ++k)
              {
                if (vcUDP_ReadGeolocation(pNodeList->GetElement(k)->AsString(""), info.pPoints[info.numPoints], epsgCode))
                  ++info.numPoints;
              }

              // If its closed, we need to readd the first point at the end
              if (isClosed && pNodeList->length > 0 && vcUDP_ReadGeolocation(pNodeList->GetElement(0)->AsString(""), info.pPoints[info.numPoints], epsgCode))
                ++info.numPoints;

              if (info.numPoints > 0)
                vcPOI_AddToList(pProgramState, pName, colour, 1.0, &info, epsgCode);

              udFree(info.pPoints);
            }
          }
        }
      }
      // TODO: Add bookmark support here.
    }
  }

epilogue:
  udFree(pXMLText);
}
