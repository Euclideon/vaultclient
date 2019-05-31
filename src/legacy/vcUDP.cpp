#include "vcUDP.h"

#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

#include "vcModel.h"
#include "vcPOI.h"
#include "vcFolder.h"

enum vcUDPItemDataType
{
  vcUDPIDT_Dataset,
  vcUDPIDT_Label,
  vcUDPIDT_Polygon,

  vcUDPIDT_Count,
};

const char *g_vcUDPTypeNames[vcUDPIDT_Count] = {
  "DataSetData",
  "Label",
  "PolygonData",
};

const char *g_vcUDPTypeGroupNames[vcUDPIDT_Count] = {
  "DataSetGroup",
  "LabelGroup",
  "PolygonGroup",
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
      uint32_t colour;
      bool isClosed;
      udDouble3 *pPoints;
      int numPoints;
      int32_t epsgCode;
    } polygon;
  };
};

vdkProjectNode *vcUDP_AddModel(vcState *pProgramState, const char *pUDPFilename, const char *pModelName, const char *pModelFilename, bool firstLoad, udDouble3 *pPosition, udDouble3 *pYPR, double scale)
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
    file.SetFromFullPath(pModelFilename);

  vdkProjectNode *pNode;
  vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "UDS", pModelName, pModelFilename, nullptr);

  vcModel *pModel = new vcModel(pNode, pProgramState);
  pNode->pUserData = pModel;
  pModel->m_meterScale = scale;
  //pModel->m_defaultMatrix = udDouble4x4::translation(*pPosition) * udDouble4x4::rotationYPR(*pYPR) * udDouble4x4::scaleUniform(scale); // ?????

  return pNode;
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
      case vcUDPIDT_Polygon:
        item.polygon.pName = nullptr;
        item.polygon.colour = 0xFFFFFFFF; // Default colour is white
        item.polygon.isClosed = false;
        item.polygon.pPoints = nullptr;
        item.polygon.numPoints = 0;
        item.polygon.epsgCode = 0;
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
            {
              item.polygon.pName = datasetData.Get("[%zu].content", j).AsString("");
            }
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PolygonIsClosed"))
            {
              item.polygon.isClosed = datasetData.Get("[%zu].content", j).AsBool();
            }
            else if (udStrEqual(datasetData.Get("[%zu].Name", j).AsString(), "PolygonColour"))
            {
              item.polygon.colour = (uint32_t)datasetData.Get("[%zu].content", j).AsInt(); // Stored as int in file, needs to be uint in vc
            }
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

void vcUDP_AddDataSetData(vcState *pProgramState, const char *pFilename, std::vector<vcUDPItemData> *pItemData, size_t index, bool *pFirstLoad)
{
  const vcUDPItemData &item = pItemData->at(index);

  if (item.dataset.pPath != nullptr)
  {
    udDouble3 position = udDouble3::zero();
    udDouble3 ypr = udDouble3::zero();
    double scale = 1.0;

    udDouble3 *pPosition = nullptr;
    udDouble3 *pYPR = nullptr;

    if (item.dataset.pLocation != nullptr)
    {
      int epsgCode = 0;

      if (vcUDP_ReadGeolocation(item.dataset.pLocation, position, epsgCode))
        pPosition = &position;

      udUnused(epsgCode); //TODO: Use this
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
      scale = udStrAtof64(item.dataset.pScale);

    pItemData->at(index).sceneFolder = { pProgramState->sceneExplorer.clickedItem.pParent != nullptr ? pProgramState->sceneExplorer.clickedItem.pParent : pProgramState->sceneExplorer.pProjectRoot->m_pNode, vcUDP_AddModel(pProgramState, pFilename, item.dataset.pName, item.dataset.pPath, *pFirstLoad, pPosition, pYPR, scale) };

    *pFirstLoad = false;
  }
}

void vcUDP_AddLabelData(vcState *pProgramState, std::vector<vcUDPItemData> *pLabelData, size_t index)
{
  const vcUDPItemData &item = pLabelData->at(index);

  if (item.label.pName != nullptr && item.label.pGeoLocation != nullptr)
  {
    vdkProjectNode *pNode;
    vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "POI", item.label.pName, nullptr, nullptr);

    udDouble3 position = udDouble3::zero();
    int32_t epsgCode = 0;

    uint16_t size = (uint16_t)udStrAtou(item.label.pFontSize);
    uint32_t colour = (uint32_t)udStrAtoi(item.label.pColour); //These are stored as int (with negatives) in MDM

    const char *pSize;
    if (size > 16)
      pSize = "Large";
    else if (size < 16)
      pSize = "Small";
    else
      pSize = "Medium";

    vdkProjectNode_SetMetadataString(pNode, "textSize", pSize);
    vdkProjectNode_SetMetadataUint(pNode, "nameColour", colour);

    if (vcUDP_ReadGeolocation(item.label.pGeoLocation, position, epsgCode))
    {
      udGeoZone *pZone = udAllocType(udGeoZone, 1, udAF_Zero);
      udGeoZone_SetFromSRID(pZone, epsgCode);

      vcPOI *pPOI = new vcPOI(pNode, pProgramState);
      pNode->pUserData = pPOI;
      pPOI->m_pCurrentProjection = pZone;
      pPOI->m_pPreferredProjection = (udGeoZone*)udMemDup(pZone, sizeof(udGeoZone), 0, udAF_Zero);
    }

    vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, (double*)&position);
    pLabelData->at(index).sceneFolder = { pProgramState->sceneExplorer.clickedItem.pParent != nullptr ? pProgramState->sceneExplorer.clickedItem.pParent : pProgramState->sceneExplorer.pProjectRoot->m_pNode, pNode };
  }
}

void vcUDP_AddPolygonData(vcState *pProgramState, std::vector<vcUDPItemData> *pLabelData, size_t index)
{
  const vcUDPItemData &item = pLabelData->at(index);

  if (item.polygon.pName != nullptr && item.polygon.pPoints != nullptr && item.polygon.numPoints > 0)
  {
    vdkProjectNode *pNode;
    vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "POI", item.polygon.pName, nullptr, nullptr);

    if (item.polygon.isClosed)
      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Polygon, item.polygon.numPoints, (double*)item.polygon.pPoints);
    else
      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_MultiPoint, item.polygon.numPoints, (double*)item.polygon.pPoints);

    vdkProjectNode_SetMetadataDouble(pNode, "lineWidth", 1.0);
    vdkProjectNode_SetMetadataUint(pNode, "lineColourPrimary", item.polygon.colour);
    vdkProjectNode_SetMetadataUint(pNode, "lineColourSecondary", item.polygon.colour);
    vdkProjectNode_SetMetadataString(pNode, "textSize", "Medium");

    udGeoZone *pZone = udAllocType(udGeoZone, 1, udAF_Zero);
    udGeoZone_SetFromSRID(pZone, item.polygon.epsgCode);

    vcPOI *pPOI = new vcPOI(pNode, pProgramState);
    pNode->pUserData = pPOI;
    pPOI->m_pCurrentProjection = pZone;
    pPOI->m_pPreferredProjection = (udGeoZone*)udMemDup(pZone, sizeof(udGeoZone), 0, udAF_Zero);
  }
}

void vcUDP_AddItemData(vcState *pProgramState, const char *pFilename, std::vector<vcUDPItemData> *pItemData, size_t index, bool *pFirstLoad)
{
  const vcUDPItemData &item = pItemData->at(index);

  // Ensure parent is loaded and set as the item to add items to
  if (item.parentID != 0)
  {
    for (size_t i = 0; i < pItemData->size(); ++i)
    {
      if (pItemData->at(i).id == item.parentID)
      {
        if (pItemData->at(i).sceneFolder.pItem == nullptr)
          vcUDP_AddItemData(pProgramState, pFilename, pItemData, i, pFirstLoad);

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
      vdkProjectNode *pNode;
      vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "Folder", item.dataset.pName, nullptr, nullptr);

      vcFolder *pFolder = new vcFolder(pNode, pProgramState);
      pNode->pUserData = pFolder;

      pItemData->at(index).sceneFolder = { pProgramState->sceneExplorer.clickedItem.pParent != nullptr ? pProgramState->sceneExplorer.clickedItem.pParent : pProgramState->sceneExplorer.pProjectRoot->m_pNode, pNode };
    }
  }
  else
  {
    switch (item.type)
    {
    case vcUDPIDT_Dataset:
      vcUDP_AddDataSetData(pProgramState, pFilename, pItemData, index, pFirstLoad);
      break;
    case vcUDPIDT_Label:
      vcUDP_AddLabelData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Polygon:
      vcUDP_AddPolygonData(pProgramState, pItemData, index);
      break;
    case vcUDPIDT_Count:
      // Failure
      break;
    }
  }

  // Find the highest lower tree index sibling
  int64_t maxLowerTreeIndex = 0;
  int mLTIndex = index;
  for (int i = 0; i < pItemData->size(); ++i)
  {
    if (pItemData->at(i).sceneFolder.pParent == item.sceneFolder.pParent && pItemData->at(i).treeIndex > maxLowerTreeIndex && pItemData->at(i).treeIndex < item.treeIndex)
    {
      maxLowerTreeIndex = pItemData->at(index).treeIndex;
      mLTIndex = i;
    }
  }

  // If none found then we become the first child
  if (mLTIndex == index)
    vdkProjectNode_InsertUnder(pProgramState->sceneExplorer.pProject, item.sceneFolder.pParent, item.sceneFolder.pItem);
  else
    vdkProjectNode_InsertAfter(pProgramState->sceneExplorer.pProject, pItemData->at(mLTIndex).sceneFolder.pItem, item.sceneFolder.pItem);

  pProgramState->sceneExplorer.clickedItem = { nullptr, nullptr };
}

void vcUDP_LoadItem(vcState *pProgramState, const char *pFilename, const udJSON &groupDataBlock, std::vector<vcUDPItemData> *pItemData, vcUDPItemDataType type, bool *pFirstLoad)
{
  const udJSON &items = groupDataBlock.Get("DataBlock");
  vcUDP_ParseItemData(items, pItemData, type);

  for (size_t i = 0; i < items.ArrayLength(); ++i)
  {
    vcUDP_AddItemData(pProgramState, pFilename, pItemData, i, pFirstLoad);
  }
}

void vcUDP_Load(vcState *pProgramState, const char *pFilename)
{
  udResult result;
  udJSON xml;
  const char *pXMLText = nullptr;
  bool firstLoad = true;
  std::vector<vcUDPItemData> dataSetData;
  std::vector<vcUDPItemData> labelData;
  std::vector<vcUDPItemData> polygonData;

  std::vector<vcUDPItemData> *data[vcUDPIDT_Count] = { &dataSetData, &labelData, &polygonData };

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
        vcUDP_AddModel(pProgramState, pFilename, nullptr, dataEntries.Get("[%zu].content", i).AsString(), firstLoad, nullptr, nullptr, 1.0);
        firstLoad = false;
      }
    }

    const udJSON &dataBlocks = xml.Get("DataBlock.DataBlock");
    for (size_t i = 0; i < dataBlocks.ArrayLength(); ++i)
    {
      const udJSON &groupDataBlock = dataBlocks.Get("[%zu]", i);

      int j = 0;
      for (; j < vcUDPIDT_Count; ++j)
        if (udStrEqual(groupDataBlock.Get("Name").AsString(), g_vcUDPTypeGroupNames[j]))
          break;

      vcUDP_LoadItem(pProgramState, pFilename, groupDataBlock, data[j], (vcUDPItemDataType)j, &firstLoad);

      // TODO: Add bookmark support here. ?? maybe not here after 31/5 changes
    }
  }

epilogue:
  udFree(pXMLText);
}
