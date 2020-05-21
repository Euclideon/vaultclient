#ifdef FBXSDK_ON

#include "udPlatform.h"

#if UDPLATFORM_MACOSX
# pragma GCC diagnostic ignored "-Wpragma-pack"
# pragma clang diagnostic ignored "-Wpragma-pack"
#endif

#include <fbxsdk.h>

#include "udChunkedArray.h"
#include "udFile.h"
#include "udStringUtil.h"
#include "udMath.h"
#include "udPlatformUtil.h"
#include "udWorkerPool.h"

#include "vcFBXPoly.h"
#include "vcMesh.h"
#include "vcTexture.h"

struct vcFBXPolyAsyncLoadMeshInfo
{
  vcPolygonModel *pPolygonModel;
  vcPolygonModelMesh *pMesh;
  vcP3N3UV2Vertex *pVerts;

  int *pIndices;
  uint32_t currentIndices;
};

void vcFBX_MainThreadCreateMesh(void *pData)
{
  vcFBXPolyAsyncLoadMeshInfo *pLoadInfo = (vcFBXPolyAsyncLoadMeshInfo*)pData;
  if (!pLoadInfo->pPolygonModel->keepLoading)
    return;

  vcMesh_Create(&pLoadInfo->pMesh->pMesh, vcP3N3UV2VertexLayout, (int)udLengthOf(vcP3N3UV2VertexLayout), pLoadInfo->pVerts, pLoadInfo->pMesh->numVertices, pLoadInfo->pIndices, pLoadInfo->currentIndices);

  udFree(pLoadInfo->pVerts);
  udFree(pLoadInfo->pIndices);
}


bool LoadScene(FbxManager *pManager, FbxDocument *pScene, const char *pFilename)
{
  int lFileMajor, lFileMinor, lFileRevision;
  int lSDKMajor, lSDKMinor, lSDKRevision;
  //int lFileFormat = -1;
  //int lAnimStackCount;
  bool lStatus;

  // Get the file version number generate by the FBX SDK.
  FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

  // Create an importer.
  FbxImporter *lImporter = FbxImporter::Create(pManager, "");

  // Initialize the importer by providing a filename.
  const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
  lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

  if (!lImportStatus)
  {
    //FbxString error = lImporter->GetStatus().GetErrorString();
    //FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
    //FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

    if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
    {
      //FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
      //FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
    }

    return false;
  }

  // Import the scene.
  lStatus = lImporter->Import(pScene);
  if (lStatus == true)
  {
    // Check the scene integrity!
    FbxStatus status;
    FbxArray< FbxString *> details;
    FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(pScene), &status, &details);
    lStatus = sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData);
    if (lStatus == false)
    {
      if (details.GetCount())
      {
        //FBXSDK_printf("Scene integrity verification failed with the following errors:\n");
        //for (int i = 0; i < details.GetCount(); i++)
        //  FBXSDK_printf("   %s\n", details[i]->Buffer());

        FbxArrayDelete<FbxString *>(details);
      }
    }
  }

  // Destroy the importer.
  lImporter->Destroy();

  return lStatus;
}

udResult vcFBX_LoadPolygonModel(vcPolygonModel **ppModel, const char *pFilename, udWorkerPool *pWorkerPool)
{
  udResult result = udR_Failure_;
  vcPolygonModel *pModel = udAllocType(vcPolygonModel, 1, udAF_Zero);
  pModel->keepLoading = true;

  FbxManager *pFBXManager = nullptr;
  FbxScene *pFBXScene = nullptr;
  FbxIOSettings *pIOSettings = nullptr;

  bool lResult;
  int numGeoms;
  FbxSystemUnit units;
  FbxGeometryConverter *pConverter = nullptr;

  // Prepare the FBX SDK.
  pFBXManager = FbxManager::Create();
  UD_ERROR_NULL(pFBXManager, udR_InvalidConfiguration);

  // Create an IOSettings object. This object holds all import/export settings.
  pIOSettings = FbxIOSettings::Create(pFBXManager, IOSROOT);
  pFBXManager->SetIOSettings(pIOSettings);

  pConverter = new FbxGeometryConverter(pFBXManager);

  // Create the FBX Scene
  pFBXScene = FbxScene::Create(pFBXManager, "Imported Scene");
  UD_ERROR_NULL(pFBXManager, udR_InvalidConfiguration);

  // Load the scene.
  lResult = LoadScene(pFBXManager, pFBXScene, pFilename);

  pConverter->Triangulate(pFBXScene, true);

  if (pFBXScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
    FbxSystemUnit::m.ConvertScene(pFBXScene);

  // Load meshes
  numGeoms = pFBXScene->GetGeometryCount();
  //printf("Geoms: %d\n", numGeoms);

  UD_ERROR_IF(numGeoms < 1, udR_ObjectNotFound);

  pModel->meshCount = numGeoms;
  pModel->pMeshes = udAllocType(vcPolygonModelMesh, pModel->meshCount, udAF_Zero);

  pModel->modelOffset = udDouble4x4::identity();

  for (int i = 0; i < numGeoms; ++i)
  {
    FbxNode *pNode = pFBXScene->GetGeometry(i)->GetNode();
    FbxMesh *pMesh = pNode->GetMesh();

    //printf("Polygons: %d\n", pMesh->GetPolygonCount());

    FbxVector4 *pPoints = pMesh->GetControlPoints();
    int vertCount = pMesh->GetControlPointsCount();

    int *pIndices = pMesh->GetPolygonVertices();
    int indexCount = pMesh->GetPolygonVertexCount();

    FbxArray<FbxVector4> normals;
    bool hasNormals = pMesh->GetPolygonVertexNormals(normals);

    FbxSurfaceMaterial *pMaterial = pNode->GetMaterial(0);

    int totalMaterials = pNode->GetMaterialCount();

    FbxStringList uvNames = {};
    pMesh->GetUVSetNames(uvNames);

    FbxProperty diffuse = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);

    const char *pTextureFilename = nullptr;

    udDouble3 textureScale = {};
    udDouble3 textureTranslate = {};
    bool textureSwap = false;
    FbxLayerElement::EMappingMode mapMode = FbxLayerElement::EMappingMode::eByControlPoint;
    FbxLayerElement::EReferenceMode refMode = FbxLayerElement::EReferenceMode::eDirect;
    FbxLayerElementArrayTemplate<int> *pIArray = nullptr;
    FbxLayerElementArrayTemplate<FbxVector2> *pDArray = nullptr;

    pModel->pMeshes[i].material.colour = 0xFFFFFFFF;

    if (diffuse.IsValid())
    {
      if (diffuse.GetSrcObjectCount<FbxTexture>() > 0)
      {
        for (int j = 0; j < diffuse.GetSrcObjectCount<FbxFileTexture>(); ++j)
        {
          FbxFileTexture *pTexture = diffuse.GetSrcObject<FbxFileTexture>(j);

          FbxTexture::EMappingType map = pTexture->GetMappingType();
          FbxTexture::ETextureUse use = pTexture->GetTextureUse();
          FbxString UVSet = pTexture->UVSet.Get();

          if (map != FbxTexture::eUV || use != FbxTexture::eStandard)
            continue;

          textureScale = udDouble3::create(pTexture->Scaling.Get()[0], pTexture->Scaling.Get()[1], pTexture->Scaling.Get()[2]);
          textureTranslate = udDouble3::create(pTexture->Translation.Get()[0], pTexture->Translation.Get()[1], pTexture->Translation.Get()[2]);;
          textureSwap = pTexture->GetSwapUV();

          const FbxGeometryElementUV *pUV = pMesh->GetElementUV(UVSet);
          if (pUV == nullptr)
            pUV = pMesh->GetElementUV(0);

          pTextureFilename = pTexture->GetFileName();
          
          mapMode = pUV->GetMappingMode();
          refMode = pUV->GetReferenceMode();
          pIArray = &pUV->GetIndexArray();
          pDArray = &pUV->GetDirectArray();
        }
      }
      else
      {
        FbxDataType type = diffuse.GetPropertyDataType();

        if (FbxDouble3DT == type || FbxColor3DT == type || FbxColor4DT == type || FbxDouble4DT == type)
        {
          const FbxColor col = diffuse.Get<FbxColor>();

          uint32_t colour = 0xFF000000;
          colour |= (((uint8_t)(col.mBlue * 0xFF)) << 16);
          colour |= (((uint8_t)(col.mGreen * 0xFF)) << 8);
          colour |= ((uint8_t)(col.mRed * 0xFF));

          pModel->pMeshes[i].material.colour = colour;
        }
      }
    }

    vcP3N3UV2Vertex *pVerts = udAllocType(vcP3N3UV2Vertex, pMesh->GetControlPointsCount(), udAF_None);
    int previousIndex = 0;

    for (int vertIndex = 0; vertIndex < vertCount; ++vertIndex)
    {
      double *pRawPositions = pPoints[vertIndex];
      pVerts[vertIndex].position = udFloat3::create(udDouble3::create(pRawPositions[0], pRawPositions[1], pRawPositions[2]));

      if (hasNormals)
      {
        double *pRawNormal = normals[vertIndex];
        pVerts[vertIndex].normal = udFloat3::create(udDouble3::create(pRawNormal[0], pRawNormal[1], pRawNormal[2]));
      }

      if (pDArray != nullptr)
      {
        FbxVector2 newVec = { 0, 0 };
        uint32_t uvIndex = 0;

        switch (mapMode)
        {
        case FbxLayerElement::eByControlPoint:
          uvIndex = vertIndex;
          break;
        case FbxLayerElement::eByPolygon:
          uvIndex = 0;
          for (int indexIndex = 0; indexIndex < indexCount; ++indexIndex)
          {
            if (pIndices[indexIndex] == vertIndex)
            {
              uvIndex = indexIndex / 3;
              break;
            }
          }
          //uvIndex = pFBX->currMeshPolygon;
          break;
        case FbxLayerElement::eByPolygonVertex:
          uvIndex = 0;
          for (int indexIndex = previousIndex + 1; indexIndex != previousIndex; indexIndex = (indexIndex + 1) % indexCount)
          {
            if (pIndices[indexIndex] == vertIndex)
            {
              previousIndex = indexIndex;
              uvIndex = indexIndex;
              break;
            }
          }
          //uvIndex = pFBX->currMeshPolygon * 3 + i;
          break;
        case FbxLayerElement::eNone: // falls through
        case FbxLayerElement::eByEdge: // falls through
        case FbxLayerElement::eAllSame:
          uvIndex = 0;
          break;
        }

        if (refMode != FbxGeometryElement::eDirect)
          uvIndex = pIArray->GetAt(uvIndex);

        newVec = pDArray->GetAt(uvIndex);

        udDouble3 UV = udDouble3::create(newVec[0], 1.0 - newVec[1], 0.0) * textureScale + textureTranslate;

        if (textureSwap)
        {
          double swap = UV[0];
          UV[0] = UV[1];
          UV[1] = swap;
        }

        pVerts[vertIndex].uv = udFloat2::create(UV.toVector2());
      }
    }

    vcFBXPolyAsyncLoadMeshInfo *pLoadInfo = udAllocType(vcFBXPolyAsyncLoadMeshInfo, 1, udAF_Zero);
    UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

    pModel->pMeshes[i].flags = 0;
    pModel->pMeshes[i].materialID = 0;
    pModel->pMeshes[i].LOD = 0;
    pModel->pMeshes[i].numVertices = vertCount;
    pModel->pMeshes[i].numElements = indexCount;

    pLoadInfo->pPolygonModel = pModel;
    pLoadInfo->pMesh = &pModel->pMeshes[i];
    pLoadInfo->pVerts = pVerts;
    pLoadInfo->pIndices = (int32_t*)udMemDup(pIndices, indexCount * sizeof(int32_t), 0, udAF_None);
    pLoadInfo->currentIndices = indexCount;

    if (pTextureFilename != nullptr)
    {
      //const char *pTextureFilepath = udTempStr("%s%s", pOBJReader->basePath.GetPath(), pMaterial->map_Kd);
      const int MaxTextureSize = 1024;
      vcTexture_AsyncCreateFromFilename(&pModel->pMeshes[i].material.pTexture, pWorkerPool, pTextureFilename, vcTFM_Linear, false, vcTWM_Repeat, MaxTextureSize);
    }

    if (udWorkerPool_AddTask(pWorkerPool, nullptr, pLoadInfo, true, vcFBX_MainThreadCreateMesh) != udR_Success)
    {
      udFree(pLoadInfo->pVerts);
      udFree(pLoadInfo->pIndices);
      udFree(pLoadInfo);
    }

    //result = vcMesh_Create(&pModel->pMeshes[i].pMesh, vcP3N3UV2VertexLayout, (int)udLengthOf(vcP3N3UV2VertexLayout), pVerts, vertCount, pIndices, indexCount);
  }

  result = udR_Success;
  *ppModel = pModel;
  pModel = nullptr;

epilogue:
  if (pConverter != nullptr)
    delete pConverter;

  if (pFBXManager != nullptr)
    pFBXManager->Destroy();

  return result;
}


#endif
