#include "vcUDP.h"

#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

enum vcUDPItemDataType
{
  vcUDPIDT_Dataset,
  vcUDPIDT_Label,
  vcUDPIDT_Polygon,
  vcUDPIDT_Bookmark,
  vcUDPIDT_Measure,

  vcUDPIDT_Count,
};

const char *g_vcUDPTypeNames[vcUDPIDT_Count] = {
  "DataSetData",
  "Label",
  "PolygonData",
  "BookmarkData",
  "Measure",
};

const char *g_vcUDPTypeGroupNames[vcUDPIDT_Count] = {
  "DataSetGroup",
  "LabelGroup",
  "PolygonGroup",
  "BookmarkDataRoot",
  "MeasureGroup",
};

struct vcUDPItemData
{
  int64_t id;
  bool isFolder;
  int64_t parentID;
  int64_t treeIndex;
  vcSceneItemRef sceneFolder;

  vcUDPItemDataType type;
  union
  {
    struct
    {
      const char *pName;
      const char *pPath;
      const char *pLocation;
      const char *pAngleX;
      const char *pAngleY;
      const char *pAngleZ;
      const char *pScale;
    } dataset;
    struct
    {
      const char *pName;
      const char *pGeoLocation;
      const char *pColour;
      const char *pFontSize;
      const char *pHyperlink;
    } label;
    struct
    {
      const char *pName;
      const char *pColour;
      bool isClosed;
      udDouble3 *pPoints;
      int numPoints;
      int32_t epsgCode;
    } polygon;
    struct
    {
      const char *pName;
      const char *pGeoLocation;
      const char *pRotation;
    } bookmark;
    struct
    {
      const char *pName;
      const char *geoLocation[2];
      const char *pColour;
    } measure;
  };
};

udProjectNode *vcUDP_AddModel(vcState *pProgramState, const char *pUDPFilename, const char *pModelName, const char *pModelFilename, int epsgCode, udDouble3 *pPosition, udDouble3 *pYPR, double *pScale)
{
  // If the model filename is nullptr there's nothing to load
  if (pModelFilename == nullptr)
    return nullptr;

  udFilename file(pUDPFilename);

  // If the path doesn't match the following patterns we should assume it's relative to the UDP:
  // <drive/protocol>:<path> (contains a ":")
  // /<path> (starts with "/")
  // \\<UNC Path> (starts with "\\")
  if (udStrchr(pModelFilename, ":") == nullptr && pModelFilename[0] != '/' && !udStrBeginsWith(pModelFilename, "\\\\"))
    file.SetFilenameWithExt(pModelFilename);
  else
    file.SetFromFullPath("%s", pModelFilename);

  udProjectNode *pNode = nullptr;
  udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;
  udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "UDS", pModelName, file.GetPath(), nullptr);

  if (epsgCode != 0 && pPosition != nullptr)
  {
    udGeoZone tempZone = {};
    if (udGeoZone_SetFromSRID(&tempZone, epsgCode) == udR_Success)
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, tempZone, udPGT_Point, pPosition, 1);
  }

  if (pPosition != nullptr)
  {
    udProjectNode_SetMetadataDouble(pNode, "transform.position.x", pPosition->x);
    udProjectNode_SetMetadataDouble(pNode, "transform.position.y", pPosition->y);
    udProjectNode_SetMetadataDouble(pNode, "transform.position.z", pPosition->z);
  }

  if (pYPR != nullptr)
  {
    udProjectNode_SetMetadataDouble(pNode, "transform.rotation.y", pYPR->x);
    udProjectNode_SetMetadataDouble(pNode, "transform.rotation.p", pYPR->y);
    udProjectNode_SetMetadataDouble(pNode, "transform.rotation.r", pYPR->z);
  }

  if (pScale != nullptr)
    udProjectNode_SetMetadataDouble(pNode, "transform.scale", *pScale);

  return pNode;
}

bool vcUDP_ReadGeolocation(const char *pStr, udDouble3 &position, int &epsg)
{
  int count = 0;
  int charCount = -1;

  for (; charCount != 0 && count < 3; ++count)
  {
    charCount = 0;
    position[count] = udStrAtof64(pStr, &charCount);
    pStr += charCount;
    pStr = udStrchr(pStr, ",");
    if (pStr != nullptr)
      ++pStr;
  }

  if (charCount > 0)
    epsg = udStrAtoi(pStr, &charCount);

  if (charCount > 0)
    ++count;

  return (count == 4);
}

bool vcUDP_ReadDouble3(const char *pStr, udDouble3 &rotation)
{
  int count = 0;

  for (int charCount = -1; charCount != 0 && count < 3; ++count)
  {
    charCount = 0;
    rotation[count] = udStrAtof64(pStr, &charCount);
    pStr += charCount;
    pStr = udStrchr(pStr, ",");
    ++pStr;
  }

  return (count == 3);
}

void vcUDP_ParseItemData(const udJSON &items, std::vector<vcUDPItemData> *pItemData, vcUDPItemDataType type)
{
  for (size_t i = 0; i < items.ArrayLength(); ++i)
  {
    if (udStrEqual(items.Get("[%zu].Name", i).AsString(), g_vcUDPTypeNames[type]))
    {
      const udJSON &datasetData = items.Get("[%zu].DataEntry", i);

      vcUDPItemData item = {};
      item.id = items.Get("[%zu].Id", i).AsInt64();
      item.isFolder = false;
      item.parentID = 0;
      item.treeIndex = 0;
      item.sceneFolder = { nullptr, nullptr };
      item.type = type;

      switch (type)
      {
      case vcUDPIDT_Dataset:
        item.dataset.pName = nullptr;
        item.dataset.pPath = nullptr;
        item.dataset.pLocation = nullptr;
        item.dataset.pAngleX = nullptr;
        item.dataset.pAngleY = nullptr;
        item.dataset.pAngleZ = nullptr;
        item.dataset.pScale = nullptr;
        break;
      case vcUDPIDT_Label:
        item.label.pName = nullptr;
        item.label.pGeoLocation = nullptr;
        item.label.pColour = nullptr;
        item.label.pFontSize = nullptr;
        item.label.pHyperlink = nullptr;
        break;
      case vcUDPIDT_Polygon:
        item.polygon.pName = nullptr;
        item.polygon.pColour = nullptr;
        item.polygon.isClosed = false;
        item.polygon.pPoints = nullptr;
        item.polygon.numPoints = 0;
        item.polygon.epsgCode = 0;
        break;
      case vcUDPIDT_Bookmark:
        item.bookmark.pName = nullptr;
        item.bookmark.pGeoLocation = nullptr;
        item.bookmark.pRotation = nullptr;
        break;
      case vcUDPIDT_Measure:
        item.measure.pName = nullptr;
        item.measure.geoLocation[0] = nullptr;
        item.measure.geoLocation[1] = nullptr;
        item.measure.pColour = nullptr;
        break;
      case vcUDPIDT_Count:
        // Failure
        break;
      }

      for (size_t j = 0; j < datasetData.ArrayLength(); ++j)
      {
        if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "IsFolder"))
        {
          item.isFolder = datasetData.Get("[%zu].content", j).AsBool();
        }
        else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "ParentId"))
        {
          item.parentID = datasetData.Get("[%zu].content", j).AsInt64();
        }
        else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "TreeIndex"))
        {
          item.treeIndex = datasetData.Get("[%zu].content", j).AsInt64();
        }
        else
        {
          switch (type)
          {
          case vcUDPIDT_Dataset:
            if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Path"))
              item.dataset.pPath = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Name"))
              item.dataset.pName = datasetData.Get("[%zu].content", j).AsString("");
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Location"))
              item.dataset.pLocation = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "AngleX"))
              item.dataset.pAngleX = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "AngleY"))
              item.dataset.pAngleY = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "AngleZ"))
              item.dataset.pAngleZ = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Scale"))
              item.dataset.pScale = datasetData.Get("[%zu].content", j).AsString();
            break;
          case vcUDPIDT_Label:
            if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Name"))
              item.label.pName = datasetData.Get("[%zu].content", j).AsString("");
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "GeoLocation"))
              item.label.pGeoLocation = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "LabelColor"))
              item.label.pColour = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "FontSize"))
              item.label.pFontSize = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Hyperlink"))
              item.label.pHyperlink = datasetData.Get("[%zu].content", j).AsString();
            break;
          case vcUDPIDT_Polygon:
            if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PolygonName"))
              item.polygon.pName = datasetData.Get("[%zu].content", j).AsString("");
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PolygonIsClosed"))
              item.polygon.isClosed = datasetData.Get("[%zu].content", j).AsBool();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PolygonColour"))
              item.polygon.pColour = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Nodes"))
            {
              const udJSONArray *pNodeList = datasetData.Get("[%zu].GeoLocationArray", j).AsArray();
              item.polygon.pPoints = udAllocType(udDouble3, pNodeList->length, udAF_None);
              item.polygon.numPoints = 0;

              for (size_t k = 0; k < pNodeList->length; ++k)
              {
                if (vcUDP_ReadGeolocation(pNodeList->GetElement(k)->AsString(""), item.polygon.pPoints[item.polygon.numPoints], item.polygon.epsgCode))
                  ++item.polygon.numPoints;
              }
            }
            break;
          case vcUDPIDT_Bookmark:
            if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "BookmarkName"))
              item.bookmark.pName = datasetData.Get("[%zu].content", j).AsString("");
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "GeoLocation"))
              item.bookmark.pGeoLocation = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PerspectiveOrientation"))
              item.bookmark.pRotation = datasetData.Get("[%zu].content", j).AsString();
            break;
          case vcUDPIDT_Measure:
            if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "Name"))
              item.measure.pName = datasetData.Get("[%zu].content", j).AsString("");
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "GeoLocationP0"))
              item.measure.geoLocation[0] = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "GeoLocationP1"))
              item.measure.geoLocation[1] = datasetData.Get("[%zu].content", j).AsString();
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "MeasureColor"))
              item.measure.pColour = datasetData.Get("[%zu].content", j).AsString();
            break;
          case vcUDPIDT_Count:
            // Failure
            break;
          }
        }
      }

      pItemData->push_back(item);
    }
  }
}

void vcUDP_AddDataSetData(vcState *pProgramState, const char *pFilename, std::vector<vcUDPItemData> *pItemData, size_t index)
{
  const vcUDPItemData &item = pItemData->at(index);

  if (item.dataset.pPath != nullptr)
  {
    int epsgCode = 0;
    udDouble3 position = udDouble3::zero();
    udDouble3 ypr = udDouble3::zero();
    double scale = 1.0;

    udDouble3 *pPosition = nullptr;
    udDouble3 *pYPR = nullptr;
    double *pScale = nullptr;

    if (item.dataset.pLocation != nullptr)
    {
      if (vcUDP_ReadGeolocation(item.dataset.pLocation, position, epsgCode))
        pPosition = &position;
    }

    if (item.dataset.pAngleX != nullptr || item.dataset.pAngleY != nullptr || item.dataset.pAngleZ != nullptr) // At least one of the angles is set
    {
      if (item.dataset.pAngleZ != nullptr)
        ypr.x = UD_DEG2RAD(udStrAtof64(item.dataset.pAngleZ));

      if (item.dataset.pAngleX != nullptr)
        ypr.y = UD_DEG2RAD(udStrAtof64(item.dataset.pAngleX));

      if (item.dataset.pAngleY != nullptr)
        ypr.z = UD_DEG2RAD(udStrAtof64(item.dataset.pAngleY));

      pYPR = &ypr;
    }

    if (item.dataset.pScale != nullptr)
    {
      scale = udStrAtof64(item.dataset.pScale);
      pScale = &scale;
    }

    udProjectNode *pNode = vcUDP_AddModel(pProgramState, pFilename, item.dataset.pName, item.dataset.pPath, epsgCode, pPosition, pYPR, pScale);

    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;
    pItemData->at(index).sceneFolder = { pParentNode, pNode };
  }
}

void vcUPD_AddBookmarkData(vcState *pProgramState, std::vector<vcUDPItemData> *pItemData, size_t index)
{
  const vcUDPItemData &item = pItemData->at(index);

  if (item.bookmark.pName != nullptr && item.bookmark.pGeoLocation != nullptr)
  {
    udProjectNode *pNode = nullptr;
    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;
    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "Camera", item.bookmark.pName, nullptr, nullptr);

    udGeoZone zone;
    int epsgCode = 0;
    udDouble3 temp = udDouble3::zero();
    if (vcUDP_ReadGeolocation(item.bookmark.pGeoLocation, temp, epsgCode))
    {
      udGeoZone_SetFromSRID(&zone, (int32_t)epsgCode);
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, zone, udPGT_Point, &temp, 1);
    }

    temp = udDouble3::zero();
    if (item.bookmark.pRotation != nullptr && vcUDP_ReadDouble3(item.bookmark.pRotation, temp))
    {
      udProjectNode_SetMetadataDouble(pNode, "transform.rotation.x", temp.x);
      udProjectNode_SetMetadataDouble(pNode, "transform.rotation.y", temp.y);
      udProjectNode_SetMetadataDouble(pNode, "transform.rotation.z", temp.z);
    }

    pItemData->at(index).sceneFolder = { pParentNode, pNode };
  }
}

void vcUPD_AddMeasureData(vcState *pProgramState, std::vector<vcUDPItemData> *pItemData, size_t index)
{
  const vcUDPItemData &item = pItemData->at(index);

  if (item.measure.pName != nullptr && item.measure.geoLocation[0] != nullptr && item.measure.geoLocation[1] != nullptr)
  {
    udProjectNode *pNode = nullptr;
    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;

    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", item.bookmark.pName, nullptr, nullptr);

    int epsgCode = 0;
    udDouble3 temp[2];
    udGeoZone zone;
    uint32_t colour = item.measure.pColour ? (uint32_t)udStrAtoi(item.measure.pColour) : 0xffffffff; //These are stored as int (with negatives) in MDM

    udProjectNode_SetMetadataUint(pNode, "lineColourPrimary", colour);
    udProjectNode_SetMetadataUint(pNode, "lineColourSecondary", colour);
    udProjectNode_SetMetadataBool(pNode, "showLength", true);

    if (vcUDP_ReadGeolocation(item.measure.geoLocation[0], temp[0], epsgCode))
    {
      udGeoZone_SetFromSRID(&zone, (int32_t)epsgCode);
    }

    if (vcUDP_ReadGeolocation(item.measure.geoLocation[1], temp[1], epsgCode))
    {
      udGeoZone_SetFromSRID(&zone, (int32_t)epsgCode);
    }

    vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, zone, udPGT_LineString, temp, 2);

    pItemData->at(index).sceneFolder = { pParentNode, pNode };
  }
}
void vcUDP_AddLabelData(vcState *pProgramState, std::vector<vcUDPItemData> *pLabelData, size_t index)
{
  const vcUDPItemData &item = pLabelData->at(index);

  if (item.label.pName != nullptr && item.label.pGeoLocation != nullptr)
  {
    udProjectNode *pNode = nullptr;
    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;

    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", item.label.pName, nullptr, nullptr);

    udDouble3 position = udDouble3::zero();
    int32_t epsgCode = 0;

    uint16_t size = item.label.pFontSize ? (uint16_t)udStrAtou(item.label.pFontSize) : 16;
    uint32_t colour = item.label.pColour ? (uint32_t)udStrAtoi(item.label.pColour) : 0xffffffff; //These are stored as int (with negatives) in MDM

    const char *pSize;
    if (size > 16)
      pSize = "Large";
    else if (size < 16)
      pSize = "Small";
    else
      pSize = "Medium";

    udProjectNode_SetMetadataString(pNode, "textSize", pSize);
    udProjectNode_SetMetadataUint(pNode, "nameColour", colour);

    if (vcUDP_ReadGeolocation(item.label.pGeoLocation, position, epsgCode))
    {
      udGeoZone zone;
      udGeoZone_SetFromSRID(&zone, epsgCode);
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, zone, udPGT_Point, &position, 1);
    }

    pLabelData->at(index).sceneFolder = { pParentNode, pNode };
  }
}

void vcUDP_AddPolygonData(vcState *pProgramState, std::vector<vcUDPItemData> *pLabelData, size_t index)
{
  const vcUDPItemData &item = pLabelData->at(index);

  if (item.polygon.pName != nullptr && item.polygon.pPoints != nullptr && item.polygon.numPoints > 0)
  {
    udProjectNode *pNode = nullptr;
    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;

    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", item.polygon.pName, nullptr, nullptr);

    udGeoZone zone;
    if (udGeoZone_SetFromSRID(&zone, item.polygon.epsgCode) != udR_Success)
      pProgramState->geozone = zone;

    if (item.polygon.isClosed)
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, zone, udPGT_Polygon, item.polygon.pPoints, item.polygon.numPoints);
    else
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, zone, udPGT_MultiPoint, item.polygon.pPoints, item.polygon.numPoints);

    udDouble3 *pTemp = item.polygon.pPoints;
    udFree(pTemp);

    uint32_t colour = item.polygon.pColour ? (uint32_t)udStrAtoi(item.polygon.pColour) : 0xffffffff;

    udProjectNode_SetMetadataDouble(pNode, "lineWidth", 1.0);
    udProjectNode_SetMetadataUint(pNode, "lineColourPrimary", colour);
    udProjectNode_SetMetadataUint(pNode, "lineColourSecondary", colour);
    udProjectNode_SetMetadataString(pNode, "textSize", "Medium");

    pLabelData->at(index).sceneFolder = { pParentNode, pNode };
  }
}

void vcUDP_AddItemData(vcState *pProgramState, const char *pFilename, std::vector<vcUDPItemData> *pItemData, size_t index)
{
  pProgramState->sceneExplorer.clickedItem = { nullptr, nullptr };
  const vcUDPItemData &item = pItemData->at(index);

  // Ensure parent is loaded and set as the item to add items to
  if (item.parentID != 0)
  {
    for (size_t i = 0; i < pItemData->size(); ++i)
    {
      if (pItemData->at(i).id == item.parentID && pItemData->at(i).isFolder && pItemData->at(i).type == item.type)
      {
        if (pItemData->at(i).sceneFolder.pItem == nullptr)
          vcUDP_AddItemData(pProgramState, pFilename, pItemData, i);

        pProgramState->sceneExplorer.clickedItem = pItemData->at(i).sceneFolder;
        break;
      }
    }
  }

  // Create the folders and items
  if (item.isFolder)
  {
    if (pItemData->at(index).sceneFolder.pParent == nullptr)
    {
      udProjectNode *pNode = nullptr;
      udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;
      udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "Folder", item.dataset.pName, nullptr, nullptr);

      vcFolder *pFolder = new vcFolder(&pProgramState->activeProject, pNode, pProgramState);
      pNode->pUserData = pFolder;

      pItemData->at(index).sceneFolder = { pParentNode, pNode };
    }
  }
  else
  {
    switch (item.type)
    {
    case vcUDPIDT_Dataset:
      vcUDP_AddDataSetData(pProgramState, pFilename, pItemData, index);
      break;
    case vcUDPIDT_Label:
      vcUDP_AddLabelData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Polygon:
      vcUDP_AddPolygonData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Bookmark:
      vcUPD_AddBookmarkData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Measure:
      vcUPD_AddMeasureData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Count:
      // Failure
      break;
    }
  }
}

void vcUDP_LoadItem(vcState *pProgramState, const char *pFilename, const udJSON &groupDataBlock, std::vector<vcUDPItemData> *pItemData, vcUDPItemDataType type)
{
  const udJSON &items = groupDataBlock.Get("DataBlock");
  size_t start = pItemData->size();
  vcUDP_ParseItemData(items, pItemData, type);

  for (; start < pItemData->size(); ++start)
  {
    vcUDP_AddItemData(pProgramState, pFilename, pItemData, start);
  }
}

void vcUDP_Load(vcState *pProgramState, const char *pFilename)
{
  udResult result;
  udJSON xml;
  const char *pXMLText = nullptr;
  std::vector<vcUDPItemData> itemData;

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
        vcUDP_AddModel(pProgramState, pFilename, nullptr, dataEntries.Get("[%zu].content", i).AsString(), 0, nullptr, nullptr, nullptr);
      }
    }

    const udJSON &dataBlocks = xml.Get("DataBlock.DataBlock");
    for (size_t i = 0; i < dataBlocks.ArrayLength(); ++i)
    {
      const udJSON &groupDataBlock = dataBlocks.Get("[%zu]", i);

      for (int j = 0; j < vcUDPIDT_Count; ++j)
      {
        if (udStrEqual(groupDataBlock.Get("Name").AsString(), g_vcUDPTypeGroupNames[j]))
        {
          vcUDP_LoadItem(pProgramState, pFilename, groupDataBlock, &itemData, (vcUDPItemDataType)j);
          break;
        }
      }
    }

    for (size_t itemNum = 0; itemNum < itemData.size(); ++itemNum)
    {
      vcUDPItemData item = itemData.at(itemNum);

      // Order the items
      // Find insert before child, the lowest greater tree index
      int64_t minGreaterTreeIndex = 0;
      size_t minGTIndex = itemNum;
      for (size_t i = 0; i < itemData.size(); ++i)
      {
        if (itemData.at(i).sceneFolder.pParent == item.sceneFolder.pParent && item.type == itemData.at(i).type)
        {
          if (itemData.at(i).treeIndex < minGreaterTreeIndex && itemData.at(i).treeIndex > item.treeIndex)
          {
            minGreaterTreeIndex = itemData.at(i).treeIndex;
            minGTIndex = i;
          }
        }
      }

      // If any found then do the move, otherwise this node should bubble to the front
      if (minGTIndex != itemNum)
        udProjectNode_MoveChild(pProgramState->activeProject.pProject, item.sceneFolder.pParent, item.sceneFolder.pParent, item.sceneFolder.pItem, itemData.at(itemNum).sceneFolder.pItem);
    }
  }

epilogue:
  udFree(pXMLText);
}
