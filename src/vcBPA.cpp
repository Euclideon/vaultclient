#include "vcBPA.h"

#include "vcMath.h"
#include "vcConvert.h"

#include "udConvert.h"
#include "udConvertCustom.h"
#include "udQueryContext.h"

#include "udChunkedArray.h"
#include "udWorkerPool.h"
#include "udSafeDeque.h"
#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udDebug.h"

#if VC_HASCONVERT

// small array
#define LOW_MEMORY 1

#if LOW_MEMORY
// for data structure
typedef uint8_t TINDEX; // max 255, needs bigger than BLOCK_SIZE
static const uint32_t BLOCK_SIZE = 64;
static const uint32_t HASH_ARRAY_LEN = 4096; // (1111 1111 1111) + 1, 4 bits for x/y/z
static const uint32_t HASH_ARRAY_MASK = 15; // 1111
#define GET_HASHINDEX(x, y, z) (((uint32_t)(x) & HASH_ARRAY_MASK) << 6 | ((uint32_t)(y) & HASH_ARRAY_MASK) << 3 | ((uint32_t)(z) & HASH_ARRAY_MASK) )

// for BPA
typedef uint32_t HASHKEY;
static const uint32_t BPA_LEVEL_LEN = 512; // max voxel (1<<10) supported by uint32.
static const uint32_t BPA_LEVEL_MASK = 511; // max voxel index (1<<10 - 1) (11 1111 1111)supported by uint32.
static const uint32_t MAX_POINTNUM = 1 << 25; // can read maximum 4,194,304 points in cube(2r*2r*2r*BPA_LEVEL_LEN*BPA_LEVEL_LEN*BPA_LEVEL_LEN)meter
#define GET_HASHKEY(x, y, z) (((uint32_t)(x) & BPA_LEVEL_MASK) << 20 | ((uint32_t)(y) & BPA_LEVEL_MASK) << 10 | ((uint32_t)(z) & BPA_LEVEL_MASK) )
#define GET_X(code) ( ((HASHKEY)(code) >> 20) & BPA_LEVEL_MASK )
#define GET_Y(code) ( ((HASHKEY)(code) >> 10) & BPA_LEVEL_MASK )
#define GET_Z(code) ( (HASHKEY)(code) & BPA_LEVEL_MASK )

#else
// for data structure
typedef uint16_t TINDEX; // max 65535, needs bigger than BLOCK_SIZE
static const uint32_t BLOCK_SIZE = 1 << 15; // 21 - 6
static const uint32_t HASH_ARRAY_LEN = 262144; // 1 << 18 + 1, 6 bits for x/y/z
static const uint32_t HASH_ARRAY_MASK = 63; // 111111, 1 << 6 -1
#define GET_HASHINDEX(gridX, gridY, gridZ) (((uint32_t)(gridX) & HASH_ARRAY_MASK) << 8 | ((uint32_t)(gridY) & HASH_ARRAY_MASK) << 4 | ((uint32_t)(gridZ) & HASH_ARRAY_MASK) )

// for BPA
typedef uint64_t HASHKEY;
static const uint32_t BPA_LEVEL_LEN = 2097152; // max voxel (1<<21) supported by uint64.
static const uint32_t BPA_LEVEL_MASK = 2097151; // (1<<21 -1)
static const uint32_t MAX_POINTNUM = 1 << 30; // can read maximum 1,073,741,824 points in cube(2r*2r*2r*LEVEL_LEN*LEVEL_LEN*LEVEL_LEN)meter

#define GET_HASHKEY(vx, vy, vz) (((uint32_t)(vx) & BPA_LEVEL_MASK) << 42 | ((uint32_t)(vy) & BPA_LEVEL_MASK) << 21 | ((uint32_t)(vz) & BPA_LEVEL_MASK) )
#define GET_X(code) ( ((VCODE)code >> 42) & BPA_LEVEL_MASK )
#define GET_Y(code) ( ((VCODE)code >> 21) & BPA_LEVEL_MASK )
#define GET_Z(code) ( (VCODE)(code) & BPA_LEVEL_MASK )

#endif

template<typename TData>
struct DataBlock
{
  TData *pBlockData;
  DataBlock *pNext; // next linked LinkedArray
  TINDEX index; // avaliable index  

  int blocklen;

  void Init()
  {
    pBlockData = udAllocType(TData, BLOCK_SIZE, udAF_Zero);
    index = 0;
    pNext = nullptr;
  }

  void Deinit()
  {
    DataBlock *pCurrTemp = this;
    DataBlock *pNextTemp = this->pNext;
    while (pNextTemp != nullptr)
    {
      while (pNextTemp->pNext != nullptr)
      {
        pCurrTemp = pNextTemp;
        pNextTemp = pCurrTemp->pNext;
      }
      udFree(pCurrTemp->pNext->pBlockData);
      pCurrTemp->pNext->index = 0;

      udFree(pCurrTemp->pNext);
      pCurrTemp = this;
      pNextTemp = this->pNext;
    }

    udFree(pBlockData);
    index = 0;
  }

  TData *AddData()
  {
    DataBlock *pFind = this;
    while (pFind->pNext != nullptr)
      pFind = pFind->pNext;

    if (pFind->index == BLOCK_SIZE)
    {
      pFind->pNext = udAllocType(DataBlock, 1, udAF_Zero);
      pFind = pFind->pNext;
      pFind->Init();
      blocklen++;
    }

    return &(pFind->pBlockData[pFind->index++]);
  }

};

template<typename TKey, typename TData>
struct vcBPAHashMap
{
  struct HashNode
  {
    TData data; // first block
    TKey hashKey; // hash value

    void Init()
    {
      data.Init();
      hashKey = 0;
    }

    void Deinit()
    {
      data.Deinit();
      hashKey = 0;
    }
  };

  struct HashNodeBlock
  {
    HashNode **pBlockData;
    HashNodeBlock *pNext;
    TINDEX index; // avaliable index

    void Init()
    {
      pBlockData = udAllocType(HashNode*, BLOCK_SIZE, udAF_Zero);
      index = 0;
      pNext = nullptr;
    }

    void DeinitBlock(HashNodeBlock *pNode)
    {
      if (pNode->index == 0)
        return;

      for (TINDEX i = 0; i < pNode->index; i++)
      {
        if (pNode->pBlockData[i])
        {
          pNode->pBlockData[i]->Deinit(); // [ HashNode *]->Deinit();
          udFree(pNode->pBlockData[i]); //udFree(HashNode *)
        }
      }
      udFree(pNode->pBlockData); //udFree([HashNode*])
      pNode->index = 0;
    }

    void Deinit()
    {
      HashNodeBlock *pCurrTemp = this;
      HashNodeBlock *pNexTemp = this->pNext;
      while (pNexTemp != nullptr)
      {
        while (pNexTemp->pNext != nullptr)
        {
          pCurrTemp = pNexTemp;
          pNexTemp = pCurrTemp->pNext;
        }

        DeinitBlock(pCurrTemp->pNext);  // Deinit(HashNodeBlock*)
        udFree(pCurrTemp->pNext);

        pCurrTemp = this;
        pNexTemp = this->pNext;
      }

      DeinitBlock(this);
      udFree(pNext);
      index = 0;
    }

    HashNode *AddHashNode(TKey hashKey)
    {
      HashNodeBlock *pFind = this;
      while (pFind->pNext != nullptr)
        pFind = pFind->pNext;

      if (pFind->index == BLOCK_SIZE)
      {
        pFind->pNext = udAllocType(HashNodeBlock, 1, udAF_Zero);
        pFind = pFind->pNext;
        pFind->Init();
      }

      HashNode* pNode = udAllocType(HashNode, 1, udAF_Zero);
      pNode->Init();
      pNode->hashKey = hashKey;
      pFind->pBlockData[pFind->index++] = pNode;

      return pNode;
    }

    HashNode *FindHashNode(TKey hashKey)
    {
      HashNodeBlock *pFind = this;
      while (pFind != nullptr)
      {
        for (TINDEX i = 0; i < pFind->index; i++)
        {
          HashNode *p = pFind->pBlockData[i];
          if (p->hashKey == hashKey)
            return p;
        }
        pFind = pFind->pNext;
      }

      return nullptr;
    }
  };

  HashNodeBlock **pArray;

  void Init()
  {
    pArray = udAllocType(HashNodeBlock*, HASH_ARRAY_LEN, udAF_Zero);
  }

  TData *AddNode(uint32_t x, uint32_t y, uint32_t z)
  {
    uint32_t hashIndex = GET_HASHINDEX(x, y, z);
    TKey hashKey = GET_HASHKEY(x, y, z);

    HashNodeBlock **pFirst = &pArray[hashIndex];
    if (*pFirst == nullptr)
    {
      *pFirst = udAllocType(HashNodeBlock, 1, udAF_Zero);
      (*pFirst)->Init();
    }

    HashNode *pNode = (*pFirst)->FindHashNode(hashKey);
    if (pNode == nullptr)
    {
      pNode = (*pFirst)->AddHashNode(hashKey);
    }
    return &pNode->data;
  }

  void Deinit()
  {
    if (!pArray) return;
    for (uint32_t i = 0; i < HASH_ARRAY_LEN; i++)
    {
      if (pArray[i])
      {
        pArray[i]->Deinit();
        udFree(pArray[i]);
      }
    }
    udFree(pArray);
    pArray = 0;
  }

  TData *FindData(uint32_t x, uint32_t y, uint32_t z)
  {
    uint32_t hashIndex = GET_HASHINDEX(x, y, z);
    TKey hashKey = GET_HASHKEY(x, y, z);
    HashNodeBlock **pFirst = &pArray[hashIndex];
    if (*pFirst == nullptr)
      return nullptr;

    HashNode *pNode = (*pFirst)->FindHashNode(hashKey);
    if (pNode != nullptr)
      return &pNode->data;

    return nullptr;
  }

};

template<typename TEdgeArray, typename TEdge>
TEdge *FindAvailableStackArray(TEdgeArray &pBottomArray)
{
  TEdgeArray *pFind = &pBottomArray;
  while (pFind->pTop != nullptr)
    pFind = pFind->pTop;

  if (pFind->index == TEdgeArray::ElementCount)
  {
    pFind->pTop = udAllocType(TEdgeArray, 1, udAF_Zero);
    pFind->pTop->pBottom = pFind;
    pFind = pFind->pTop;
  }
  return &pFind->edges[pFind->index++];
}

enum vcBPARunningStatus
{
  vcBPARS_Active,
  vcBPARS_BPADone,
  vcBPARS_Success,
  vcBPARS_Failed,
  vcBPARS_Close,
  vcBPARS_End
};

//4+1=5byte
struct vcBPAVertex
{
  udDouble3 position;
  uint32_t j; // position from buffer
  bool used; // 0 not // 0x0001 triangle // 
};

struct vcBPAVoxel
{
  DataBlock<vcBPAVertex> data;
  uint32_t pointNum;

  void Init()
  {
    data.Init();
    pointNum = 0;
  }
  void Deinit()
  {
    data.Deinit();
  }
  void ToArray(vcBPAVertex **pArray)
  {
    DataBlock<vcBPAVertex> *pFind = &data;
    uint32_t t = 0;
    while (pFind != nullptr)
    {
      for (uint32_t i = 0; i < pFind->index; i++)
      {
        pArray[t++] = &pFind->pBlockData[i];
      }
      pFind = pFind->pNext;
    }      
  }
};

struct vcBPATriangle
{
  udDouble3 vertices[3];
  udDouble3 ballCenter; // Ball used to construct triangle
};

struct vcBPATriangleArray
{
  DataBlock<vcBPATriangle> data;
  uint32_t pointNum;
  void Init()
  {
    data.Init();
    pointNum = 0;
  }
  void Deinit()
  {
    data.Deinit();
  }
};

struct vcBPAEdge
{
  udDouble3 *vertices[2];
  udDouble3 triangleZ;
  udDouble3 ballCenter;  
};

struct vcBPAEdgeArray
{
  enum { ElementCount = 512 };
  vcBPAEdge edges[ElementCount];

  vcBPAEdgeArray *pTop;
  vcBPAEdgeArray *pBottom;
  int16_t index;

  void Init()
  {
    pTop = 0;
    pBottom = 0;
    index = 0;
  }
};

struct vcBPAFrozenEdge
{
  udDouble3 vertices[2];
  udDouble3 ballCenter;
  udDouble3 triangleZ;
};

struct vcBPAFrozenEdgeArray
{
  enum { ElementCount = 32 };
  vcBPAFrozenEdge edges[ElementCount];

  vcBPAFrozenEdgeArray *pTop;
  vcBPAFrozenEdgeArray *pBottom;
  uint16_t index;

  void Init()
  {
    pTop = 0;
    pBottom = 0;
    index = 0;
  }
};

// old model
struct vcBPAGridHashNode
{
  vcBPAHashMap<HASHKEY, vcBPAVoxel> voxelHashMap; // linked hash array, never change after being created unitil .
  vcBPAHashMap<HASHKEY, vcBPATriangleArray> triangleHashMap; // linked hash array will be used to compare data
  vcBPAEdgeArray activeEdgesStack;
  vcBPAFrozenEdgeArray frozenEdgesStack;
  int32_t frozenEdgeCount;

  udDouble3 zero;
  udDouble3 end;
  uint32_t vxSize;
  uint32_t vySize;
  uint32_t vzSize;
  uint32_t pointNum;
  uint32_t triangleSize;
  udPointBufferF64 *pBuffer;

  void Init()
  {
    voxelHashMap.Init();
    triangleHashMap.Init();
    activeEdgesStack.Init();
    frozenEdgesStack.Init();
  }

  void CleanBPAData()
  {
    // release all active edges(should be anyone left)
    RemoveAllEdge<vcBPAEdgeArray>(activeEdgesStack);  

    // release all voxels
    voxelHashMap.Deinit();
  }

  void CleanCompareData()
  {
    // release all triangles
    triangleHashMap.Deinit();

    if (pBuffer != nullptr)
    {
      udPointBufferF64_Destroy(&pBuffer);
      pBuffer = nullptr;
    }

  }

  void Deinit()
  {
    // release all left frozen edges
    RemoveAllEdge<vcBPAFrozenEdgeArray>(frozenEdgesStack);
    CleanBPAData();
    CleanCompareData();
  }

  static bool FindPointInActiveEdges(vcBPAGridHashNode *pGrid, udDouble3 *p0)
  {
    vcBPAEdgeArray *pFind = &pGrid->activeEdgesStack;
    while (pFind->pTop != nullptr)
      pFind = pFind->pTop;
    do
    {
      for (int32_t i = pFind->index - 1; i >= 0; i--)
      {
        vcBPAEdge &edge = pFind->edges[i];
        if (edge.vertices[0] == p0 || edge.vertices[1] == p0)
          return true;
      }
      pFind = pFind->pBottom;
    } while (pFind != nullptr);

    return false;
  }

  template<typename T_ARRAY, typename TPoint>
  static bool FindEdge(T_ARRAY &pArray, TPoint p0, TPoint p1)
  {
    T_ARRAY *pFind = &pArray;

    while (pFind->pTop != nullptr)
      pFind = pFind->pTop;

    do
    {
      for (int32_t i = pFind->index - 1; i >= 0; i--)
      {
        if (pFind->edges[i].vertices[0] == p0 && pFind->edges[i].vertices[1] == p1)
          return true;          
      }
      pFind = pFind->pBottom;
    } while (pFind != nullptr);

    return false;
  }

  template<typename T_ARRAY, typename TEdge>
  static bool PopEdge(T_ARRAY &pArray, TEdge &edge)
  {
    T_ARRAY *pFind = &pArray;
    // empty
    if (pFind->index == 0)
      return false;

    while (pFind->pTop != nullptr)
      pFind = pFind->pTop;

    edge = pFind->edges[--pFind->index];
    if (pFind->index == 0)
    {
      if (pFind != &pArray)
      {
        pFind = pFind->pBottom;
        memset(pFind->pTop->edges, 0, sizeof(TEdge) * T_ARRAY::ElementCount);
        pFind->pTop->index = 0;
        udFree(pFind->pTop);
      }
      else
      {
        memset(pFind->edges, 0, sizeof(TEdge) * T_ARRAY::ElementCount);
      }      
    }

    return true;
  }

  template<typename T_ARRAY, typename TEdge, typename TPoint>
  static void RemoveEdge(T_ARRAY &pBottomArray, TPoint p0, TPoint p1)
  {    
    // empty
    if (pBottomArray.index == 2)
    {
      pBottomArray.index = 0;
      return;
    }      

    int32_t removeNum = 2;
    TEdge top1;
    PopEdge<T_ARRAY, TEdge>(pBottomArray, top1);

    TEdge top2;
    PopEdge<T_ARRAY, TEdge>(pBottomArray, top2);

    if ((top2.vertices[0] == p0 && top2.vertices[1] == p1) || (top2.vertices[1] == p0 && top2.vertices[0] == p1))
    {
      removeNum--;
    }

    if ((top1.vertices[0] == p0 && top1.vertices[1] == p1) || (top1.vertices[1] == p0 && top1.vertices[0] == p1))
    {
      top1 = top2;
      removeNum--;
    }

    if (removeNum == 0)
      return;      

    T_ARRAY *pFind = &pBottomArray;
    while (pFind->pTop != nullptr)
      pFind = pFind->pTop;

    // swap from the bottom
    do
    {
      for (int32_t i = pFind->index - 1; i >= 0; i--)
      {
        TEdge &edge = pFind->edges[i];
        if ((edge.vertices[0] == p0 && edge.vertices[1] == p1) || (edge.vertices[1] == p0 && edge.vertices[0] == p1))
        {
          edge = top1;
          if (--removeNum == 0)
            return;
        }
        if ((edge.vertices[0] == p1 && edge.vertices[1] == p0) || (edge.vertices[1] == p1 && edge.vertices[0] == p0))
        {
          edge = top2;
          if (--removeNum == 0)
            return;
        }
      }
      pFind = pFind->pBottom;
    } while (pFind != nullptr);
  }

   template<typename T_ARRAY>
  static void RemoveAllEdge(T_ARRAY &pArray)
  {
    if (pArray.index == 0)
      return;

    T_ARRAY *pFind = &pArray;
    if (pFind->index == 0) return;

    while (pFind->pTop != nullptr)
      pFind = pFind->pTop;

    while (pFind != &pArray)
    {
      pFind = pFind->pBottom;
      udFree(pFind->pTop);
    }

    pArray.index = 0;
  }
 
  static bool IfTriangleDuplicated(vcBPAGridHashNode *pGrid, uint32_t x, uint32_t y, uint32_t z, const udDouble3 &p0, const udDouble3 &p1, const udDouble3 &p2)
  {
    vcBPATriangleArray *pArray = pGrid->triangleHashMap.FindData(x, y, z);
    if (!pArray)
      return true;    

    DataBlock<vcBPATriangle> *pData = &pArray->data;
    while (pData != nullptr)
    {
      for (int32_t i = 0; i < pData->index; i++)
      {
        vcBPATriangle &t = pData->pBlockData[i];
        if ((t.vertices[0] == p0 && t.vertices[1] == p1 && t.vertices[2] == p2) || (t.vertices[0] == p1 && t.vertices[1] == p2 && t.vertices[2] == p0) || (t.vertices[0] == p2 && t.vertices[1] == p0 && t.vertices[2] == p1))
          return true;
      }
      pData = pData->pNext;
    }

    return false;
  }

};

// new model grid
struct vcVoxelGridHashNode
{
  vcBPAHashMap<HASHKEY, vcBPAVoxel> voxelHashMap; // linked hash array
  udDouble3 zero;
  udDouble3 end;
  uint32_t vxSize;
  uint32_t vySize;
  uint32_t vzSize;
  uint32_t pointNum;
  udPointBufferF64 *pBuffer;

  //for hash array
  uint64_t hashKey;
  vcVoxelGridHashNode *pNext;
  void Init()
  {
    voxelHashMap.Init();
  }
  void CleanCompareData()
  {
    voxelHashMap.Deinit();

    if (pBuffer)
    {
      udPointBufferF64_Destroy(&pBuffer);
      pBuffer = nullptr;
    }
    
  }
  void Deinit()
  {
    CleanCompareData();
  }
};

struct vcBPAManifold
{
  udWorkerPool *pPool;
  udContext* pContext;

  vcBPAHashMap< HASHKEY, vcBPAGridHashNode> oldModelMap;
  vcBPAHashMap< HASHKEY, vcVoxelGridHashNode> newModelMap;

  double baseGridSize;
  double ballRadius;
  double voxelSize;

  void Init()
  {
    oldModelMap.Init();
    newModelMap.Init();
  }

  void Deinit()
  {
    oldModelMap.Deinit();
    newModelMap.Deinit();

    pContext = nullptr;
    udWorkerPool_Destroy(&pPool);
  }
};

void vcBPA_QueryPoints(udContext* pContext, udPointCloud *pModel, const double centrePoint[3], const double halfSize[3], udPointBufferF64* pBuffer)
{
  udQueryFilter *pFilter = nullptr;
  udQueryFilter_Create(&pFilter);
  static double zero[3] = {0, 0, 0};
  udQueryFilter_SetAsBox(pFilter, centrePoint, halfSize, zero);

  udQueryContext *pQuery = nullptr;
  udQueryContext_Create(pContext, &pQuery, pModel, pFilter);
  udQueryContext_ExecuteF64(pQuery, pBuffer);
  udQueryContext_Destroy(&pQuery);

  udQueryFilter_Destroy(&pFilter);
}

void vcBPA_AddPoints(uint32_t j, double x, double y, double z, const udDouble3 &zero, vcBPAHashMap<HASHKEY, vcBPAVoxel> *voxelHashMap, double voxelSize)
{
  uint32_t vx = (uint32_t)((x - zero.x) / voxelSize);
  uint32_t vy = (uint32_t)((y - zero.y) / voxelSize);
  uint32_t vz = (uint32_t)((z - zero.z) / voxelSize);

  //add a point into the voxel
  vcBPAVoxel *pVoxel = voxelHashMap->FindData(vx, vy, vz);
  if (pVoxel == nullptr)
    pVoxel = voxelHashMap->AddNode(vx, vy, vz);    

  ++pVoxel->pointNum;

  vcBPAVertex *point = pVoxel->data.AddData();
  point->position = udDouble3::create(x, y, z);
  point->j = j;
  point->used = false;
}

udDouble3 vcBPA_GetTriangleNormal(const udDouble3 & p0, const udDouble3 & p1, const udDouble3 & p2)
{
  udDouble3 a = p1 - p0;
  udDouble3 b = p2 - p0;
  return udNormalize(udCross(a, b));
}

void vcBPA_FindUnusePoints(vcBPAVertex **pUnuseArray, vcBPAVoxel *pVoxel, uint32_t &unuseNum)
{
  unuseNum = 0;
  DataBlock<vcBPAVertex> *pPointArray = &pVoxel->data;
  while (pPointArray != nullptr)
  {
    for (int i = 0; i < pPointArray->index; i++)
    {
      if (!pPointArray->pBlockData[i].used)
        pUnuseArray[unuseNum++] = &pPointArray->pBlockData[i];
    }
    pPointArray = pPointArray->pNext;
  }  
}

struct SortPoint
{
  vcBPAVertex *p;
  double d;
};

size_t vcBPA_GetNearbyPoints(vcBPAVertex **&pp, vcBPAGridHashNode* pGrid, vcBPAVertex* pVertex, uint32_t x, uint32_t y, uint32_t z, double ballDiameterSq, bool chechUsed = true)
{
  udChunkedArray<SortPoint> nearbyPoints;
  nearbyPoints.Init(32);

  int32_t vcx = (int32_t)x;
  int32_t vcy = (int32_t)y;
  int32_t vcz = (int32_t)z;

  SortPoint sp;
  for (int32_t vx = udMax(vcx - 1, 0); vx <= udMin(vcx + 1, pGrid->vxSize-1); vx++)
  {
    for (int32_t vy = udMax(vcy - 1, 0); vy <= udMin(vcy + 1, pGrid->vySize-1); vy++)
    {
      for (int32_t vz = udMax(vcz - 1, 0); vz <= udMin(vcz + 1, pGrid->vzSize-1); vz++)
      {
        vcBPAVoxel *pVoxel = pGrid->voxelHashMap.FindData(vx, vy, vz);
        if (pVoxel == nullptr)
          continue;

        // check all points
        DataBlock<vcBPAVertex> *pPointArray = &pVoxel->data;
        while (pPointArray != nullptr)
        {
          for (int i = 0; i <= pPointArray->index; i++)
          {
            if (chechUsed && pPointArray->pBlockData[i].used)
              continue;
            if (pVertex && pVertex == &pPointArray->pBlockData[i] )
              continue;

            udDouble3& point = pPointArray->pBlockData[i].position;
            double dsqr = udMagSq3(pVertex->position - point);
            if (dsqr > ballDiameterSq)
              continue;

            sp.p = &pPointArray->pBlockData[i];
            sp.d = dsqr;
            size_t j = 0;
            for (; j < nearbyPoints.length; ++j)
            {
              double d = nearbyPoints.GetElement(j)->d;
              if (d > dsqr)
                break;
            }
            nearbyPoints.Insert(j, &sp);
          }

          pPointArray = pPointArray->pNext;
        }
      }
    }
  }

  size_t nearbyNum = nearbyPoints.length;
  if (nearbyNum > 0)
  {
    pp = udAllocType(vcBPAVertex *, nearbyNum, udAF_Zero);
    for (size_t i = 0; i < nearbyPoints.length; ++i)
      pp[i] = nearbyPoints[i].p;
  }  
  nearbyPoints.Deinit();

  return nearbyNum;
}

bool vcBPA_InsertEdge(vcBPAGridHashNode *pGrid, const udDouble3 &ballCenter, const udDouble3 &triangleZ, udDouble3 *e1, udDouble3 *e2, uint32_t x, uint32_t y, uint32_t z, double voxelSize, uint32_t gridXSize, uint32_t gridYSize, uint32_t gridZSize)
{
  // Mark edge as frozen if any point lies outside the grid
  udDouble3 d1 = *e1 - pGrid->zero;
  udDouble3 d2 = *e2 - pGrid->zero;
  int d1Frozen = 0;
  int d2Frozen = false;
  if (x < gridXSize - 1)
  {
    if (d1.x >= pGrid->end.x - voxelSize)
      ++d1Frozen;
    if (d2.x >= pGrid->end.x - voxelSize)
      ++d2Frozen;
  }

  if (y < gridYSize - 1)
  {
    if (d1.y >= pGrid->end.y - voxelSize)
      ++d1Frozen;
    if (d2.y >= pGrid->end.y - voxelSize)
      ++d2Frozen;
  }

  if (z < gridZSize - 1)
  {
    if (d1.z >= pGrid->end.z - voxelSize)
      ++d1Frozen;
    if (d2.z >= pGrid->end.z - voxelSize)
      ++d2Frozen;
  }

  // frozen edge
  if (d1Frozen == 3 && d2Frozen == 3)
  {
    vcBPAFrozenEdge *pEdge = FindAvailableStackArray<vcBPAFrozenEdgeArray, vcBPAFrozenEdge>(pGrid->frozenEdgesStack);
    ++ pGrid->frozenEdgeCount;

    pEdge->vertices[0] = *e1;
    pEdge->vertices[1] = *e2;
    pEdge->ballCenter = ballCenter;
    pEdge->triangleZ = triangleZ;
    return false;
  }

  vcBPAEdge *pEdge = FindAvailableStackArray<vcBPAEdgeArray, vcBPAEdge>(pGrid->activeEdgesStack);
  pEdge->vertices[0] = e1;
  pEdge->vertices[1] = e2;
  pEdge->ballCenter = ballCenter;
  pEdge->triangleZ = triangleZ;
  return true;
}

vcBPATriangle *vcBPA_CreateTriangle(vcBPAGridHashNode *pGrid, uint32_t vx, uint32_t vy, uint32_t vz, const udDouble3 &ballCenter, const udDouble3 &p0, const udDouble3 &p1, const udDouble3 &p2)
{
  vcBPATriangleArray *pArray = pGrid->triangleHashMap.FindData(vx, vy, vz);
  if (pArray == nullptr)
    pArray = pGrid->triangleHashMap.AddNode(vx, vy, vz);
    
  vcBPATriangle *t = pArray->data.AddData();
  t->vertices[0] = p0;
  t->vertices[1] = p1;
  t->vertices[2] = p2;
  t->ballCenter = ballCenter;

  ++pGrid->triangleSize;

#ifdef UD_DEBUG
  if (pGrid->triangleSize % 10000 == 0)
    udDebugPrintf("triangleSize %d\n", pGrid->triangleSize);
#endif // _DEBUG
  
  return t;
}

bool vcBPA_FindSeedTriangle(vcBPAGridHashNode *pGrid, vcBPAVertex *pVertex, uint32_t vx, uint32_t vy, uint32_t vz, uint32_t gridX, uint32_t gridY, uint32_t gridZ, double ballRadius, double voxelSize, uint32_t gridXSize, uint32_t gridYSize, uint32_t gridZSize)
{
  // find all points within 2r  
  double ballRadiusSq = ballRadius * ballRadius;
  vcBPAVertex **nearbyPoints = nullptr;
  size_t nearbyNum = vcBPA_GetNearbyPoints(nearbyPoints, pGrid, pVertex, vx, vy, vz, ballRadiusSq*4);
  if(nearbyNum == 0)
    return false;

  udDouble3 triangleNormal;
  bool inside = false;
  udDouble3 ballCenter;

  for (size_t i = 0; i < nearbyNum; ++i)
  {
    vcBPAVertex *p1 = nearbyPoints[i];
    for (size_t j = i + 1; j < nearbyNum; ++j)
    {
      vcBPAVertex *p2 = nearbyPoints[j];

      double d12 = udMagSq3(p1->position - p2->position);
      if (d12 > ballRadiusSq * 4) continue;

      // Check that the triangle normal is consistent with the vertex normals - i.e. pointing outward
      triangleNormal = vcBPA_GetTriangleNormal(pVertex->position, p1->position, p2->position);
      if (triangleNormal.z < 0) // Normal is pointing down, that's bad!
        ballCenter = udGetSphereCenterFromPoints(ballRadius, p1->position, pVertex->position, p2->position);
      else
        ballCenter = udGetSphereCenterFromPoints(ballRadius, pVertex->position, p1->position, p2->position);
      
      // Invalid triangle
      if (ballCenter == udDouble3::zero())
        continue;

      // Test that a p-ball with center in the outward halfspace touches all three vertices and contains no other points
      for (size_t k = 0; k < nearbyNum; k++)
      {
        if (nearbyPoints[k] == pVertex || nearbyPoints[k] == p1 || nearbyPoints[k] == p2) continue;
        if (ballRadiusSq - udMagSq3(nearbyPoints[k]->position - ballCenter) > FLT_EPSILON)
        {
          inside = true;
          break;
        }
      }

      // Check failed, find another
      if (!inside)
      {
        pVertex->used = true;
        p1->used = true;
        p2->used = true;

        udDouble3 *n0 = nullptr;
        udDouble3 *n1 = nullptr;
        udDouble3 *n2 = &p2->position;

        if (triangleNormal.z < 0)
        {
          n0 = &p1->position;
          n1 = &pVertex->position;
        } 
        else
        {
          n0 = &pVertex->position;
          n1 = &p1->position;          
        }

        vcBPA_InsertEdge(pGrid, ballCenter, *n1, n2, n0, gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);
        vcBPA_InsertEdge(pGrid, ballCenter, *n0, n1, n2, gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);
        vcBPA_InsertEdge(pGrid, ballCenter, *n2, n0, n1, gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);

        udDouble3 vTriangle = (ballCenter - pGrid->zero) / voxelSize;
        int32_t vTrianglex = udMax((int32_t)vTriangle.x, 0);
        vTrianglex = udMin(vTrianglex, BPA_LEVEL_LEN);

        int32_t vTriangley = udMax((int32_t)vTriangle.y, 0);
        vTriangley = udMin(vTriangley, BPA_LEVEL_LEN);

        int32_t vTrianglez = udMax((int32_t)vTriangle.z, 0);
        vTrianglez = udMin(vTrianglez, BPA_LEVEL_LEN);
        
        vcBPA_CreateTriangle(pGrid, (uint32_t)vTrianglex, (uint32_t)vTriangley, (uint32_t)vTrianglez, ballCenter, *n0, *n1, p2->position);
        udFree(nearbyPoints);
        return true;
      }
    }
  }

  udFree(nearbyPoints);
  return false;

}

struct vcBPATriangleSeek
{
  udDouble3 *vertices[3];
  udDouble3 ballCenter;
  vcBPAVertex *pCandidate;
};

bool vcBPA_BallPivot(vcBPAGridHashNode *pGrid, double ballRadius, vcBPAEdge &edge, vcBPAVertex *&pVertex, udDouble3 &ballCenter, double voxelSize)
{
  // find all points within 2r
  vcBPAVertex temp = {};
  temp.position = (*edge.vertices[0] + *edge.vertices[1])*0.5; // middle point
  double ballRadiusSq = ballRadius * ballRadius;

  udDouble3 d = (temp.position - pGrid->zero) / voxelSize;
  vcBPAVertex **nearbyPoints = nullptr;
  size_t nearbyNum = vcBPA_GetNearbyPoints(nearbyPoints, pGrid, &temp, (uint32_t)(d.x), (uint32_t)(d.y), (uint32_t)(d.z), ballRadiusSq*4);
  if (nearbyNum == 0)
    return false;

  udChunkedArray<vcBPATriangleSeek> triangles;
  triangles.Init(32);
  for (size_t i = 0; i < nearbyNum; ++i)
  {
    // Skip denegerate triangles and the triangle that made this edge
    if (&nearbyPoints[i]->position == edge.vertices[0] || &nearbyPoints[i]->position == edge.vertices[1] || nearbyPoints[i]->position == edge.triangleZ)
      continue;

    // Build potential seed triangle
    vcBPATriangleSeek t;
    t.vertices[0] = edge.vertices[0];
    t.vertices[1] = &nearbyPoints[i]->position;
    t.vertices[2] = edge.vertices[1];
    t.pCandidate = nearbyPoints[i];

    // Check that the triangle normal is consistent with the vertex normals - i.e. pointing outward
    udDouble3 triangleNormal = vcBPA_GetTriangleNormal(*t.vertices[0], *t.vertices[1], *t.vertices[2]);
    if (triangleNormal.z < 0) // Normal is pointing down, that's bad!
      continue;

    udDouble3 distances = {
      udMagSq3(*t.vertices[0] - *t.vertices[1]),
      udMagSq3(*t.vertices[1] - *t.vertices[2]),
      udMagSq3(*t.vertices[2] - *t.vertices[0])
    };
    
    if (distances.x > ballRadiusSq * 4 || distances.y > ballRadiusSq * 4 || distances.z > ballRadiusSq * 4)
      continue;

    udDouble3 ballCenterTest = udGetSphereCenterFromPoints(ballRadius, *t.vertices[0], *t.vertices[1], *t.vertices[2]);

    // Invalid triangle
    if (ballCenterTest == udDouble3::zero())
      continue;

    bool inside = false;
    for (size_t k = 0; k < nearbyNum; k++)
    {
      if (&nearbyPoints[k]->position == edge.vertices[0] || &nearbyPoints[k]->position == edge.vertices[1] || nearbyPoints[k]->position == edge.triangleZ)
        continue;
      if (ballRadiusSq - udMagSq3(nearbyPoints[k]->position - ballCenter) > FLT_EPSILON)
      {
        inside = true;
        break;
      }
    }

    // Ball contains points, invalid triangle
    if (inside)
      continue;

    t.ballCenter = ballCenterTest;
    triangles.PushBack(t);
  }

  if (triangles.length == 0)
  {
    triangles.Deinit();
    udFree(nearbyPoints);
    return false;
  }

  vcBPATriangleSeek &triangle = triangles[0];

  for (size_t i = 1; i < triangles.length; ++i)
  {
    udDouble3 points[3] = {
      edge.ballCenter - temp.position,
      triangle.ballCenter - temp.position,
      triangles[i].ballCenter - temp.position
    };

    double dots[2] = {
      udDot(points[0], points[1]),
      udDot(points[0], points[2]),
    };

    double angles[2] = {
      udACos(dots[0]),
      udACos(dots[1]),
    };

    // When points[1] is above the pivot point and points[2] is below the pivot point,
    // points[1] is the next in line.
    if (points[1].z > 0 && points[2].z < 0)
      continue;

    // When points[1] is below the pivot point and points[2] is above the pivot point,
    // points[2] is the next in line.
    if (points[1].z < 0 && points[2].z > 0)
    {
      triangle = triangles[i];
      continue;
    }

    if (points[1].z > 0) // Both are above the pivot point
    {
      if (angles[0] < angles[1])
        continue;

      if (angles[1] < angles[0])
      {
        triangle = triangles[i];
        continue;
      }
    }
    else // Both are below the pivot point
    {
      if (angles[0] > angles[1])
        continue;

      if (angles[1] > angles[0])
      {
        triangle = triangles[i];
        continue;
      }
    }
  }

  pVertex = triangle.pCandidate;
  ballCenter = triangle.ballCenter;
  triangles.Deinit();
  udFree(nearbyPoints);

  return true;
}


struct vcBPAConvertItemData
{
  vcBPAManifold *pManifold;

  vcVoxelGridHashNode *pNewModelGrid;
  vcBPAGridHashNode *pOldModelGrid;
  uint32_t gridx;
  uint32_t gridy;
  uint32_t gridz;
  udPointCloud *pOldModel;
  int32_t leftPoint;
};

struct vcBPAData
{
  udDouble3 zero;
  udDouble3 gridSize;
  uint32_t gridx;
  uint32_t gridy;
  uint32_t gridz;  
  uint32_t gridXSize;
  uint32_t gridYSize;
  uint32_t gridZSize;
};

struct vcBPAConvertItem
{
  vcBPAManifold *pManifold;
  udContext *pContext;
  udPointCloud *pOldModel;
  udPointCloud *pNewModel;
  vcConvertItem *pConvertItem;
  double gridSize;
  double ballRadius;
  int32_t voxelOffset;

  udSafeDeque<vcBPAConvertItemData> *pConvertItemData;
  vcBPAConvertItemData activeItem;
  uint32_t hashIndex;
  HASHKEY hashKey;

  vcBPAVertex **ppVertex; // points array of voxel
  uint32_t pointIndex; // index of points array of voxel
  uint32_t arrayLength; // length of points array of voxel


  udThread *pThread;
  udInterlockedInt32 running;

  void CleanReadData()
  {
    hashIndex = 0;
    hashKey = 0;

    if (ppVertex)
      udFree(ppVertex);
   
    pointIndex = 0;
    arrayLength = 0;
    if (activeItem.pNewModelGrid)
    {
      activeItem.pNewModelGrid->CleanCompareData();
      activeItem.pNewModelGrid = nullptr;
    }

    if (activeItem.pOldModelGrid)
    {
      activeItem.pOldModelGrid->CleanCompareData();
      activeItem.pOldModelGrid = nullptr;
    }
  }

  void Deinit()
  {   
    udThread_Join(pThread);
    udThread_Destroy(&pThread);

    CleanReadData();
    udSafeDeque_Destroy(&pConvertItemData);
    if (pManifold != nullptr)
    {
      pManifold->Deinit();
      udFree(pManifold);
    }
    activeItem = {};
  }
};

void vcBPA_DoGrid(vcBPAGridHashNode *pGrid, double ballRadius, double voxelSize, uint32_t gridX, uint32_t gridY, uint32_t gridZ, uint32_t gridXSize, uint32_t gridYSize, uint32_t gridZSize)
{
  udDebugPrintf("vcBPA_DoGrid grid(%d,%d,%d) gridSize(%d,%d,%d)\n", gridX, gridY, gridZ, gridXSize, gridYSize, gridZSize);

  vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNodeBlock *pCurrBlock = nullptr;
  uint32_t currBlockIndex = 0;
  uint32_t currHashIndex = 0;
  vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pNode = nullptr;

  uint32_t unUseIndex = 0;
  uint32_t unuseNum = 0;
  HASHKEY hashKey = 0;
  vcBPAVertex **pUnuseArray = nullptr;

  vcBPAEdge edge = {};
  vcBPAVertex *pVertex = nullptr;
  udDouble3 ballCenter = {};

  while (true)
  {
    if (vcBPAGridHashNode::PopEdge<vcBPAEdgeArray, vcBPAEdge>(pGrid->activeEdgesStack, edge))
    {
      pVertex = nullptr;
      ballCenter = {};

      if (vcBPA_BallPivot(pGrid, ballRadius, edge, pVertex, ballCenter, voxelSize))
      {
        if (!pVertex->used || vcBPAGridHashNode::FindPointInActiveEdges(pGrid, &pVertex->position))
        {
          udDouble3 vTriangle = (ballCenter - pGrid->zero) / voxelSize;
          int32_t vTrianglex = udMax((int32_t)vTriangle.x, 0);
          vTrianglex = udMin(vTrianglex, BPA_LEVEL_LEN);

          int32_t vTriangley = udMax((int32_t)vTriangle.y, 0);
          vTriangley = udMin(vTriangley, BPA_LEVEL_LEN);

          int32_t vTrianglez = udMax((int32_t)vTriangle.z, 0);
          vTrianglez = udMin(vTrianglez, BPA_LEVEL_LEN);

          if (pVertex->used && vcBPAGridHashNode::IfTriangleDuplicated(pGrid, (uint32_t)vTrianglex, (uint32_t)vTriangley, (uint32_t)vTrianglez, *edge.vertices[0], pVertex->position, *edge.vertices[1]))
            continue;

          vcBPA_CreateTriangle(pGrid, (uint32_t)vTrianglex, (uint32_t)vTriangley, (uint32_t)vTrianglez, ballCenter, *edge.vertices[0], pVertex->position, *edge.vertices[1]);
          // v, e1
          bool e1Active = vcBPA_InsertEdge(pGrid, ballCenter, *edge.vertices[0], &pVertex->position, edge.vertices[1], gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);
          // e0, v
          bool e2Active = vcBPA_InsertEdge(pGrid, ballCenter, *edge.vertices[1], edge.vertices[0], &pVertex->position, gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);

          if (pVertex->used)
          {
            // e1, v, pair exists
            if (e1Active)
            {
              if (vcBPAGridHashNode::FindEdge<vcBPAEdgeArray, udDouble3 *>(pGrid->activeEdgesStack, edge.vertices[1], &pVertex->position))
                vcBPAGridHashNode::RemoveEdge<vcBPAEdgeArray, vcBPAEdge, udDouble3 *>(pGrid->activeEdgesStack, edge.vertices[1], &pVertex->position);
            }
            else
            {
              if (vcBPAGridHashNode::FindEdge<vcBPAFrozenEdgeArray, udDouble3 >(pGrid->frozenEdgesStack, *edge.vertices[1], pVertex->position))
                vcBPAGridHashNode::RemoveEdge<vcBPAFrozenEdgeArray, vcBPAFrozenEdge, udDouble3>(pGrid->frozenEdgesStack, *edge.vertices[1], pVertex->position);
            }

            // v, e0 pair exists
            if (e2Active)
            {
              if (vcBPAGridHashNode::FindEdge<vcBPAEdgeArray, udDouble3 *>(pGrid->activeEdgesStack, &pVertex->position, edge.vertices[0]))
                vcBPAGridHashNode::RemoveEdge<vcBPAEdgeArray, vcBPAEdge, udDouble3 *>(pGrid->activeEdgesStack, &pVertex->position, edge.vertices[0]);
            }
            else
            {
              if (vcBPAGridHashNode::FindEdge<vcBPAFrozenEdgeArray, udDouble3 >(pGrid->frozenEdgesStack, pVertex->position, *edge.vertices[0]))
                vcBPAGridHashNode::RemoveEdge<vcBPAFrozenEdgeArray, vcBPAFrozenEdge, udDouble3>(pGrid->frozenEdgesStack, pVertex->position, *edge.vertices[0]);
            }            
          }
          pVertex->used = true;
        }
      }      
    }
    else
    {
      static uint32_t number = 0;
      if (pUnuseArray == nullptr)
      {
        if (pCurrBlock == nullptr)
        {
          while(currHashIndex < HASH_ARRAY_LEN)
          {
            pCurrBlock = pGrid->voxelHashMap.pArray[currHashIndex++];
            if (pCurrBlock == nullptr)
              continue;

            break;
          }

          if(pCurrBlock == nullptr && currHashIndex == HASH_ARRAY_LEN)
          {
            udDebugPrintf("vcBPA_DoGrid seek ends.\n");
            return;
          }

          // next block, reset index
          currBlockIndex = 0;
        }

        if (pNode == nullptr)
        {
          while (currBlockIndex < pCurrBlock->index)
          {
            pNode = pCurrBlock->pBlockData[currBlockIndex++];
            if (pNode == nullptr)
              continue;
            break;
          }

          if (pNode == nullptr && currBlockIndex == pCurrBlock->index)
          {
            pCurrBlock = pCurrBlock->pNext;
            currBlockIndex = 0;
            continue;
          }          
        }

        vcBPAVoxel *pVoxel = &pNode->data;
        number += pVoxel->pointNum;
        unUseIndex = 0;
        pUnuseArray = udAllocType(vcBPAVertex *, pVoxel->pointNum, udAF_Zero);
        vcBPA_FindUnusePoints(pUnuseArray, pVoxel, unuseNum);

        if (unuseNum == 0)
        {
          udFree(pUnuseArray);
          unUseIndex = 0;
          pNode = nullptr;
          continue;
        }

        hashKey = pNode->hashKey;
      }
      // points might used after vcBPA_FindSeedTriangle, so they need to be checked again.
      while (unUseIndex < unuseNum)
      {
        if (!pUnuseArray[unUseIndex]->used)
        {
          bool bRet = vcBPA_FindSeedTriangle(pGrid, pUnuseArray[unUseIndex++], GET_X(hashKey), GET_Y(hashKey), GET_Z(hashKey), gridX, gridY, gridZ, ballRadius, voxelSize, gridXSize, gridYSize, gridZSize);
          if (bRet)
            break;
        }
        else
        {
          ++unUseIndex;
        }
      }

      if (unUseIndex == unuseNum)
      {
        udFree(pUnuseArray);
        unuseNum = 0;
        unUseIndex = 0;
        pNode = nullptr;
      }             
    }    
  }
}


// the max index number is 0x11 1111 1111, and it needs to include the last frozen zoom, as |0-1022 frozen | 0-1022 frozen | 0-1023 |
// the max voxel count in one grid is 1024*1024*1024
bool vcBPA_CanSlice(vcBPAManifold *pManifold, const udDouble3 boundingBoxExtents)
{  
  double largestGridSize = BPA_LEVEL_LEN * pManifold->voxelSize;
  double maxBox = udMax(udAbs(boundingBoxExtents.x), udAbs(boundingBoxExtents.y));
  maxBox = udMax(maxBox, udAbs(boundingBoxExtents.z));
  if (maxBox / largestGridSize <= BPA_LEVEL_MASK)
  {
    pManifold->baseGridSize = largestGridSize; // could work
    return true;
  }

  return false;
}

udDouble3 *vcBPA_FindPoint(vcBPAGridHashNode *pGrid, uint32_t x, uint32_t y, uint32_t z, udDouble3 &point)
{
  vcBPAVoxel *pVoxel = pGrid->voxelHashMap.FindData(x, y, z);
  if (!pVoxel) return nullptr;

  DataBlock<vcBPAVertex> *pPointArray = &pVoxel->data;
  while (pPointArray != nullptr)
  {
    for (int i = 0; i <= pPointArray->index; i++)
    {
      if (pPointArray->pBlockData[i].position == point)
        return &pPointArray->pBlockData[i].position;
    }
    pPointArray = pPointArray->pNext;
  }

  return nullptr;
}

void vcBPA_UnfrozenEdges(vcBPAGridHashNode *pDestGrid, vcBPAGridHashNode *pSourceGrid, double minx, double maxx, double miny, double maxy, double minz, double maxz, double voxelSize, uint32_t gridX, uint32_t gridY, uint32_t gridZ, uint32_t gridXSize, uint32_t gridYSize, uint32_t gridZSize)
{
  if (pSourceGrid->frozenEdgeCount == 0) return;

  vcBPAFrozenEdgeArray *pArr = &pSourceGrid->frozenEdgesStack;
  if (pArr == nullptr) return;

  while (pArr->pTop != nullptr)
    pArr = pArr->pTop;
  do
  {
    if (pSourceGrid->frozenEdgeCount == 0) break;
    for (int32_t i = pArr->index - 1; i >= 0; i--)
    {
      if (pSourceGrid->frozenEdgeCount == 0) break;
      vcBPAFrozenEdge &frozenEdge = pArr->edges[i];
      bool p0here = (udClamp(frozenEdge.vertices[0].x, minx, maxx) != frozenEdge.vertices[0].x) && (udClamp(frozenEdge.vertices[0].y, miny, maxy) != frozenEdge.vertices[0].y) && (udClamp(frozenEdge.vertices[0].z, minz, maxz) != frozenEdge.vertices[0].z);
      bool p1here = (udClamp(frozenEdge.vertices[1].x, minx, maxx) != frozenEdge.vertices[1].x) && (udClamp(frozenEdge.vertices[1].y, miny, maxy) != frozenEdge.vertices[1].y) && (udClamp(frozenEdge.vertices[1].z, minz, maxz) != frozenEdge.vertices[1].z);
      if (p0here && p1here)
      {
        uint32_t c0x = uint32_t((frozenEdge.vertices[0].x - pDestGrid->zero.x) / voxelSize);
        uint32_t c0y = uint32_t((frozenEdge.vertices[0].y - pDestGrid->zero.y) / voxelSize);
        uint32_t c0z = uint32_t((frozenEdge.vertices[0].z - pDestGrid->zero.z) / voxelSize);
        udDouble3 *p0 = vcBPA_FindPoint(pDestGrid, c0x, c0y, c0z, frozenEdge.vertices[0]);
        if (p0 != nullptr)
        {
          uint32_t c1x = uint32_t((frozenEdge.vertices[1].x - pDestGrid->zero.x) / voxelSize);
          uint32_t c1y = uint32_t((frozenEdge.vertices[1].y - pDestGrid->zero.y) / voxelSize);
          uint32_t c1z = uint32_t((frozenEdge.vertices[1].z - pDestGrid->zero.z) / voxelSize);
          udDouble3 *p1 = vcBPA_FindPoint(pDestGrid, c1x, c1y, c1z, frozenEdge.vertices[1]);
          if (p1 != nullptr)
          {
            -- pSourceGrid->frozenEdgeCount;
            if (!vcBPAGridHashNode::FindEdge<vcBPAEdgeArray, udDouble3 *>(pDestGrid->activeEdgesStack, p0, p1))
              vcBPA_InsertEdge(pDestGrid, frozenEdge.ballCenter, frozenEdge.triangleZ, p0, p1, gridX, gridY, gridZ, voxelSize, gridXSize, gridYSize, gridZSize);
          }
        }
      }
    }

    pArr = pArr->pBottom;
  } while (pArr != nullptr);

  if (pSourceGrid->frozenEdgeCount == 0)
    vcBPAGridHashNode::RemoveAllEdge<vcBPAFrozenEdgeArray>(pSourceGrid->frozenEdgesStack);

}

void vcBPA_RunGridPopulation(void *pDataPtr, const vcBPAData &data, udAttributeSet *pAttributes)
{
  udDebugPrintf("vcBPA_RunGridPopulation.\n");
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pDataPtr;
  udDouble3 halfSize = data.gridSize * 0.5;
  udDouble3 center = data.zero + halfSize;

  //------------------------------------------------------------
  // add a hash node for the new model
  vcVoxelGridHashNode *pNewModelGrid = pData->pManifold->newModelMap.AddNode(data.gridx, data.gridy, data.gridz);
  pNewModelGrid->zero = data.zero;
  pNewModelGrid->end = data.zero + data.gridSize;
  pNewModelGrid->vxSize = (uint32_t)((data.gridSize.x + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);
  pNewModelGrid->vySize = (uint32_t)((data.gridSize.y + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);
  pNewModelGrid->vzSize = (uint32_t)((data.gridSize.z + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);

  udPointBufferF64_Create(&pNewModelGrid->pBuffer, MAX_POINTNUM, pAttributes);
  vcBPA_QueryPoints(pData->pManifold->pContext, pData->pNewModel, &center.x, &halfSize.x, pNewModelGrid->pBuffer);
  udDebugPrintf("vcBPA_QueryPoints pNewModel return %d.\n", pNewModelGrid->pBuffer->pointCount);

  double _time = udGetEpochMilliSecsUTCf();
  for (uint32_t j = 0; j < pNewModelGrid->pBuffer->pointCount; ++j)
    vcBPA_AddPoints(j, pNewModelGrid->pBuffer->pPositions[j * 3 + 0], pNewModelGrid->pBuffer->pPositions[j * 3 + 1], pNewModelGrid->pBuffer->pPositions[j * 3 + 2], pNewModelGrid->zero, &pNewModelGrid->voxelHashMap, pData->pManifold->voxelSize);
  pNewModelGrid->pointNum = pNewModelGrid->pBuffer->pointCount;
  _time = udGetEpochMilliSecsUTCf() - _time;
  udDebugPrintf("vcBPA_AddPoints from new model: num: %d, time costs: %f(ms) %f(s) %f(m)\n", pNewModelGrid->pBuffer->pointCount, _time, _time/1000, _time/60000);

  // get points in the grid of old model, if empty, skip
  udPointBufferF64 *pBuffer;
  udPointBufferF64_Create(&pBuffer, MAX_POINTNUM, nullptr);
  vcBPA_QueryPoints(pData->pManifold->pContext, pData->pOldModel, &center.x, &halfSize.x, pBuffer);
  udDebugPrintf("vcBPA_QueryPoints pOldModel return %d.\n", pBuffer->pointCount);
  if (pBuffer->pointCount == 0)
  {
    udPointBufferF64_Destroy(&pBuffer);

    //TODO: memory problem
    //while (pData->pConvertItemData->chunkedArray.length > 2)
    //  udSleep(100);

    vcBPAConvertItemData item = {};
    item.pManifold = pData->pManifold;
    item.pNewModelGrid = pNewModelGrid;
    item.pOldModelGrid = nullptr;
    item.gridx = data.gridx;
    item.gridy = data.gridy;
    item.gridz = data.gridz;
    item.leftPoint = pNewModelGrid->pointNum;
    item.pOldModel = pData->pOldModel;
    udSafeDeque_PushBack(pData->pConvertItemData, item);
    return;
  }

  vcBPAGridHashNode *pOldModelGrid = pData->pManifold->oldModelMap.AddNode(data.gridx, data.gridy, data.gridz);
  pOldModelGrid->pBuffer = pBuffer;
  pOldModelGrid->zero = pNewModelGrid->zero;
  pOldModelGrid->end = pNewModelGrid->end;

  //add frozen area
  if (data.gridx > 0) pOldModelGrid->zero.x -= pData->pManifold->voxelSize;
  if (data.gridy > 0) pOldModelGrid->zero.y -= pData->pManifold->voxelSize;
  if (data.gridz > 0) pOldModelGrid->zero.z -= pData->pManifold->voxelSize;
  pOldModelGrid->vxSize = (uint32_t)((pOldModelGrid->end.x - pOldModelGrid->zero.x + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);
  pOldModelGrid->vySize = (uint32_t)((pOldModelGrid->end.y - pOldModelGrid->zero.y + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);
  pOldModelGrid->vzSize = (uint32_t)((pOldModelGrid->end.z - pOldModelGrid->zero.z + pData->pManifold->voxelSize - UD_EPSILON) / pData->pManifold->voxelSize);

  _time = udGetEpochMilliSecsUTCf();
  for (uint32_t j = 0; j < pOldModelGrid->pBuffer->pointCount; ++j)
    vcBPA_AddPoints(j, pOldModelGrid->pBuffer->pPositions[j * 3 + 0], pOldModelGrid->pBuffer->pPositions[j * 3 + 1], pOldModelGrid->pBuffer->pPositions[j * 3 + 2], pOldModelGrid->zero, &pOldModelGrid->voxelHashMap, pData->pManifold->voxelSize);
  pOldModelGrid->pointNum = pOldModelGrid->pBuffer->pointCount;
  _time = udGetEpochMilliSecsUTCf() - _time;
  udDebugPrintf("vcBPA_AddPoints from old model: num: %d, time costs: %f ms %f s %f min\n", pOldModelGrid->pBuffer->pointCount, _time, _time / 1000, _time / 60000);

  if (data.gridx > 0)
  {
    vcBPAGridHashNode *lastX = pData->pManifold->oldModelMap.FindData(data.gridx - 1, data.gridy, data.gridz);
    if (lastX != nullptr)
      vcBPA_UnfrozenEdges(pOldModelGrid, lastX, pOldModelGrid->zero.x, data.zero.x, lastX->zero.y, lastX->zero.y + pData->pManifold->baseGridSize, lastX->zero.z, lastX->zero.z + pData->pManifold->baseGridSize, pData->pManifold->voxelSize, data.gridx, data.gridy, data.gridz, data.gridXSize, data.gridYSize, data.gridZSize);
  }

  if (data.gridy > 0)
  {
    vcBPAGridHashNode *lastY = pData->pManifold->oldModelMap.FindData(data.gridx, data.gridy - 1, data.gridz);
    if (lastY != nullptr)
      vcBPA_UnfrozenEdges(pOldModelGrid, lastY, lastY->zero.x, lastY->zero.x + pData->pManifold->baseGridSize - pData->pManifold->voxelSize, pOldModelGrid->zero.y, data.zero.y, lastY->zero.z, lastY->zero.z + pData->pManifold->baseGridSize - pData->pManifold->voxelSize, pData->pManifold->voxelSize, data.gridx, data.gridy, data.gridz, data.gridXSize, data.gridYSize, data.gridZSize);
  }

  if (data.gridz > 0)
  {
    vcBPAGridHashNode *lastZ = pData->pManifold->oldModelMap.FindData(data.gridx, data.gridy, data.gridz - 1);
    if (lastZ != nullptr)
      vcBPA_UnfrozenEdges(pOldModelGrid, lastZ, lastZ->zero.x, lastZ->zero.x + pData->pManifold->baseGridSize - pData->pManifold->voxelSize, lastZ->zero.y, lastZ->zero.y + pData->pManifold->baseGridSize, pOldModelGrid->zero.z, data.zero.z, pData->pManifold->voxelSize, data.gridx, data.gridy, data.gridz, data.gridXSize, data.gridYSize, data.gridZSize);
  }

  _time = udGetEpochMilliSecsUTCf();
  vcBPA_DoGrid(pOldModelGrid, pData->pManifold->ballRadius, pData->pManifold->voxelSize, data.gridx, data.gridy, data.gridz, data.gridXSize, data.gridYSize, data.gridZSize);
  pOldModelGrid->CleanBPAData();
  _time = udGetEpochMilliSecsUTCf() - _time;
  udDebugPrintf("vcBPA_DoGrid done. triangleSize:%d time costs: %f(s) %f(min). \n", pOldModelGrid->triangleSize, _time / 1000, _time / 60000);

  //TODO: memory problem
  //while (pData->pConvertItemData->chunkedArray.length > 2)
  //  udSleep(100);

  vcBPAConvertItemData item = {};
  item.pManifold = pData->pManifold;
  item.pNewModelGrid = pNewModelGrid;
  item.pOldModelGrid = pOldModelGrid;
  item.gridx = data.gridx;
  item.gridy = data.gridy;
  item.gridz = data.gridz;
  item.leftPoint = pNewModelGrid->pointNum;
  item.pOldModel = pData->pOldModel;
  udSafeDeque_PushBack(pData->pConvertItemData, item);
}

uint32_t vcBPA_ProcessThread(void *pDataPtr)
{
  udDebugPrintf("vcBPA_ProcessThread \n");
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pDataPtr;

  // data from new model
  udPointCloudHeader header = {};
  udPointCloud_GetHeader(pData->pNewModel, &header);
  udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
  udDouble3 localZero = udDouble3::create(header.boundingBoxCenter[0] - header.boundingBoxExtents[0], header.boundingBoxCenter[1] - header.boundingBoxExtents[1], header.boundingBoxCenter[2] - header.boundingBoxExtents[2]);
  udDouble3 unitSpaceExtents = udDouble3::create(header.boundingBoxExtents[0] * 2, header.boundingBoxExtents[1] * 2, header.boundingBoxExtents[2] * 2);
  udDouble3 newModelExtents = unitSpaceExtents * udDouble3::create(udMag3(storedMatrix.axis.x), udMag3(storedMatrix.axis.y), udMag3(storedMatrix.axis.z));
  udDouble3 newModelZero = (storedMatrix * udDouble4::create(localZero, 1.0)).toVector3();

  //try to slice
  if (!vcBPA_CanSlice(pData->pManifold, newModelExtents))
  {
    udDebugPrintf("slice failed, release pManifold \n");
    pData->pManifold->Deinit();
    udFree(pData->pManifold);

    pData->running.Set(vcBPARS_Failed);
    return 0;
  }

  double voxelOffset = 0.5 / pData->pManifold->voxelSize;
  pData->voxelOffset = (int32_t)voxelOffset;

  uint32_t gridXSize = (uint32_t)((newModelExtents.x + pData->pManifold->baseGridSize - UD_EPSILON) / pData->pManifold->baseGridSize);
  uint32_t gridYSize = (uint32_t)((newModelExtents.y + pData->pManifold->baseGridSize - UD_EPSILON) / pData->pManifold->baseGridSize);
  uint32_t gridZSize = (uint32_t)((newModelExtents.z + pData->pManifold->baseGridSize - UD_EPSILON) / pData->pManifold->baseGridSize);

  udDebugPrintf("slice success: radius:%f grid size:%f grid num:(%d, %d, %d) \n", pData->ballRadius, pData->pManifold->baseGridSize, gridXSize, gridYSize, gridZSize);

  static const int NUM = 1;
  udPointBufferF64 *pBuffer = nullptr;
  udPointBufferF64_Create(&pBuffer, NUM, nullptr);  

  udDouble3 zero;
  udDouble3 gridSize;
  uint32_t gridx = 0;
  uint32_t gridy = 0;
  uint32_t gridz = 0;

  double totalTime = udGetEpochMilliSecsUTCf();

  for (gridx = 0, zero.x = newModelZero.x; gridx < gridXSize; gridx++, zero.x += pData->pManifold->baseGridSize)
  {
    // real grid x size
    gridSize.x = udMin(pData->pManifold->baseGridSize, newModelExtents.x);
    for (gridy = 0, zero.y = newModelZero.y; gridy < gridYSize; gridy++, zero.y += pData->pManifold->baseGridSize)
    {
      // real grid y size
      gridSize.y = udMin(pData->pManifold->baseGridSize, newModelExtents.y);
      for (gridz = 0, zero.z = newModelZero.z; gridz < gridZSize; gridz++, zero.z += pData->pManifold->baseGridSize)
      {
        if (pData->running.Get() != vcBPARS_Active)
          goto quit;
        // real grid z size
        gridSize.z = udMin(pData->pManifold->baseGridSize, newModelExtents.z);

        udDouble3 halfSize = gridSize * 0.5;
        udDouble3 center = zero + halfSize;

        // Don't descend if there's no data        
        vcBPA_QueryPoints(pData->pManifold->pContext, pData->pNewModel, &center.x, &halfSize.x, pBuffer);        
        if (0 == pBuffer->pointCount)
          continue;

        // run grid BPA
        vcBPAData data;
        data.zero = zero;
        data.gridSize = gridSize;
        data.gridx = gridx;
        data.gridy = gridy;
        data.gridz = gridz;
        data.gridXSize = gridXSize;
        data.gridYSize = gridYSize;
        data.gridZSize = gridZSize;
        vcBPA_RunGridPopulation(pDataPtr, data, &header.attributes);
      }
    }
  }

quit:
  totalTime = udGetEpochMilliSecsUTCf() - totalTime;
  udPointBufferF64_Destroy(&pBuffer);

  // done
  if (pData->running.Get() == vcBPARS_Active && gridx == gridXSize && gridy == gridYSize && gridz == gridZSize)
  {
    vcBPAConvertItemData data = {};
    data.pManifold = pData->pManifold;
    data.pNewModelGrid = nullptr;
    data.pOldModelGrid = nullptr;
    data.pOldModel = pData->pOldModel;
    udSafeDeque_PushBack(pData->pConvertItemData, data);
    if (pData->running.Get() == vcBPARS_Active)
      pData->running.Set(vcBPARS_BPADone);
  }    
  else
  {
    if (pData->running.Get() == vcBPARS_Active)
      pData->running.Set(vcBPARS_Failed);

    pData->pManifold->Deinit();
    udFree(pData->pManifold);      
  }

  return 0;
}

udError vcBPA_ConvertOpen(udConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, udConvertCustomItemFlags flags)
{
  udUnused(everyNth);
  udUnused(origin);
  udUnused(pointResolution);
  udUnused(flags);

  vcBPAConvertItem *pData = (vcBPAConvertItem*)pConvertInput->pData;
  udSafeDeque_Create(&pData->pConvertItemData, 128);
  pData->running.Set(vcBPARS_Active);

  pData->pManifold = udAllocType(vcBPAManifold, 1, udAF_Zero);
  pData->pManifold->Init();
  udWorkerPool_Create(&pData->pManifold->pPool, (uint8_t)udGetHardwareThreadCount(), "vcBPAPool");
  pData->pManifold->pContext = pData->pContext;
  pData->pManifold->ballRadius = pData->ballRadius;
  pData->pManifold->voxelSize = pData->ballRadius * 2; // d = 2r   

  udThread_Create(&pData->pThread, vcBPA_ProcessThread, pData, udTCF_None, "BPAGridGeneratorThread");
  return udE_Success;
}

void vcBPA_FindNearByTriangle(udChunkedArray<vcBPATriangle> &triangles, vcBPAGridHashNode *pGrid, uint32_t x, uint32_t y, uint32_t z, int32_t voffset)
{
  if (pGrid == nullptr)
    return;

  int32_t vcx = (int32_t)x;
  int32_t vcy = (int32_t)y;
  int32_t vcz = (int32_t)z;

  for (int32_t vx = udMax(vcx - voffset, 0); vx <= udMin(vcx + voffset, pGrid->vxSize - 1); vx++)
  {
    for (int32_t vy = udMax(vcy - voffset, 0); vy <= udMin(vcy + voffset, pGrid->vySize - 1); vy++)
    {
      for (int32_t vz = udMax(vcz - voffset, 0); vz <= udMin(vcz + voffset, pGrid->vzSize - 1); vz++)
      {
        vcBPATriangleArray *pVoxel = pGrid->triangleHashMap.FindData(vx, vy, vz);
        if (pVoxel == nullptr)
          continue;

        // check all points
        DataBlock<vcBPATriangle> *pArray = &pVoxel->data;
        while (pArray != nullptr)
        {
          for (int i = 0; i <= pArray->index; i++)
          {
            triangles.PushBack(pArray->pBlockData[i]);
          }
          pArray = pArray->pNext;
        }
      }
    }
  }

}

void vcBPA_WaitNextGrid(vcBPAConvertItem *pData)
{
  do
  {
    if (pData->running.Get() > vcBPARS_BPADone) return;

    if (udSafeDeque_PopFront(pData->pConvertItemData, &pData->activeItem) == udR_Success)
      break;

    udSleep(1000);

  } while (true);

  udDebugPrintf("vcBPA_WaitNextGrid end\n");
}

vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *FindNextNode(vcBPAConvertItem *pData)
{
  bool useNext = false;
  vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNodeBlock *pBlock = pData->activeItem.pNewModelGrid->voxelHashMap.pArray[pData->hashIndex];
  while (pData->hashIndex < HASH_ARRAY_LEN)
  {
    if (pBlock == nullptr)
    {
      pBlock = pData->activeItem.pNewModelGrid->voxelHashMap.pArray[++pData->hashIndex];
      if (pBlock == nullptr)
        continue;
    }

    if (!useNext)
    {
      int32_t iNext = -1;
      for (TINDEX i = 0; i < pBlock->index; i++)
      {
        vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pSearch = pBlock->pBlockData[i];
        if (pSearch->hashKey == pData->hashKey) // current node found, to get next one
        {
          iNext = i + 1;
          break;
        }
      }

      // searching next block
      if (iNext == -1)
      {
        pBlock = pBlock->pNext;
        continue;
      }

      if (iNext >= 0 && iNext < pBlock->index)
      {
        vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pNode = pBlock->pBlockData[iNext];
        pData->hashKey = pNode->hashKey;
        return pNode;
      }

      useNext = true; // find next avaliable
      pBlock = pBlock->pNext;
      continue;
    }

    vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pNode = pBlock->pBlockData[0];
    pData->hashKey = pNode->hashKey;
    return pNode;
  }

  return NULL;
}

double vcBPA_SignedDistanceToTriangle(const udDouble3& v0, const udDouble3 &v1, const udDouble3 &v2, udDouble3 position, udDouble3 *pPoint)
{
  double distance = udDistanceToTriangle(v0, v1, v2, position, pPoint);
  udDouble3 normal = vcBPA_GetTriangleNormal(v0, v1, v2);
  udDouble3 p0_to_position = position - v0;
  if (udDot3(p0_to_position, normal) < 0.0)
    distance = -distance;
  return distance;
}

double vcBPA_FindClosestTriangle(udChunkedArray<vcBPATriangle> &triangles, const udDouble3 &position, udDouble3 *trianglePoint)
{
  double closestDistance = FLT_MAX;
  udDouble3 nearestPoint = {};
  size_t index = 0;
  for (size_t i = 0; i < triangles.length; i++)
  {
    vcBPATriangle &t = triangles[i];
    udDouble3 p;
    double d = vcBPA_SignedDistanceToTriangle(t.vertices[0], t.vertices[1], t.vertices[2], position, &p);
    if (d < closestDistance)
    {
      index = i;
      nearestPoint = p;
      closestDistance = d;
    }      
  }

  if (index < triangles.length)
  {
    *trianglePoint = nearestPoint;
    return closestDistance;
  }

  return FLT_MAX;
}

void vcBPA_ReadVertextData(vcBPAVertex *pPoint, udChunkedArray<vcBPATriangle> &triangles, udPointBufferF64 *pBuffer, vcVoxelGridHashNode *pNewModelGrid, const uint32_t &displacementOffset, udUInt3 &displacementDistanceOffset)
{
  // to find cloest triangle
  const udDouble3 &position = pPoint->position;
  udDouble3 trianglePoint = {};
  double distance = vcBPA_FindClosestTriangle(triangles, position, &trianglePoint);

  // Position XYZ
  memcpy(&pBuffer->pPositions[pBuffer->pointCount * 3], &pNewModelGrid->pBuffer->pPositions[pPoint->j * 3], sizeof(double) * 3);

  // Copy all of the original attributes
  ptrdiff_t pointAttrOffset = ptrdiff_t(pBuffer->pointCount) * pBuffer->attributeStride;
  for (uint32_t k = 0; k < pNewModelGrid->pBuffer->attributes.count; ++k)
  {
    udAttributeDescriptor &oldAttrDesc = pNewModelGrid->pBuffer->attributes.pDescriptors[k];
    uint32_t attributeSize = (oldAttrDesc.typeInfo & (udATI_SizeMask << udATI_SizeShift));

    // Get attribute old offset and pointer
    uint32_t attrOldOffset = 0;
    if (udAttributeSet_GetOffsetOfNamedAttribute(&pNewModelGrid->pBuffer->attributes, oldAttrDesc.name, &attrOldOffset) != udE_Success)
      continue;

    void *pOldAttr = udAddBytes(pNewModelGrid->pBuffer->pAttributes, (ptrdiff_t)pPoint->j * pNewModelGrid->pBuffer->attributeStride + attrOldOffset);

    // Get attribute new offset and pointer
    uint32_t attrNewOffset = 0;
    if (udAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, oldAttrDesc.name, &attrNewOffset) != udE_Success)
      continue;

    void *pNewAttr = udAddBytes(pBuffer->pAttributes, pointAttrOffset + attrNewOffset);

    // Copy attribute data
    memcpy(pNewAttr, pOldAttr, attributeSize);
  }

  // Displacement
  float *pDisplacement = (float *)udAddBytes(pBuffer->pAttributes, pointAttrOffset + displacementOffset);
  *pDisplacement = (float)distance;

  for (int elementIndex = 0; elementIndex < 3; ++elementIndex) //X,Y,Z
  {
    float *pDisplacementDistance = (float *)udAddBytes(pBuffer->pAttributes, pointAttrOffset + displacementDistanceOffset[elementIndex]);
    if (distance != FLT_MAX)
      *pDisplacementDistance = (float)((trianglePoint[elementIndex] - position[elementIndex]) / distance);
    else
      *pDisplacementDistance = 0.0;
  }
}

void vcBPA_ReadGrid(vcBPAConvertItem *pData, udPointBufferF64 *pBuffer, uint32_t displacementOffset, udUInt3 displacementDistanceOffset)
{
  vcVoxelGridHashNode *pNewModelGrid = pData->activeItem.pNewModelGrid;
  vcBPAGridHashNode *pOldModelGrid = pData->activeItem.pOldModelGrid;  

  while (pBuffer->pointCount < pBuffer->pointsAllocated)
  {
    if (pData->ppVertex == nullptr)
    {      
      vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pNode = FindNextNode(pData);
      if (pNode == nullptr)
      {
        assert(pData->hashIndex == HASH_ARRAY_LEN);
        udDebugPrintf("read end. \n");
        return;
      } 
      assert(pData->hashKey >= 0);
      vcBPAVoxel &voxel = pNode->data;
      pData->ppVertex = udAllocType(vcBPAVertex *, voxel.pointNum, udAF_Zero);
      voxel.ToArray(pData->ppVertex);
      pData->pointIndex = 0;
      pData->arrayLength = voxel.pointNum;
    }

    if (pData->ppVertex == nullptr) //memory run out
      return;

    uint32_t vx = GET_X(pData->hashKey);
    uint32_t vy = GET_Y(pData->hashKey);
    uint32_t vz = GET_Z(pData->hashKey);

    udChunkedArray<vcBPATriangle> triangles;
    triangles.Init(32);
    int voxelMove = 1;   
    while (triangles.length == 0)
    {
      vcBPA_FindNearByTriangle(triangles, pOldModelGrid, vx, vy, vz, 1);
      voxelMove += pData->voxelOffset/3;
      if (voxelMove >= pData->voxelOffset)
        break;
    }

    while (pData->pointIndex < pData->arrayLength)
    {
      vcBPAVertex *pPoint = pData->ppVertex[pData->pointIndex++];
      vcBPA_ReadVertextData(pPoint, triangles, pBuffer, pNewModelGrid, displacementOffset, displacementDistanceOffset);

      ++pBuffer->pointCount;
      --pData->activeItem.leftPoint;

      if (pBuffer->pointCount == pBuffer->pointsAllocated)
        break;

      if (pData->activeItem.leftPoint == 0) // stop search
        return;
    }

    triangles.Deinit();
    if (pData->pointIndex == pData->arrayLength)
    {
      udFree(pData->ppVertex);
      pData->pointIndex = 0;
      pData->arrayLength = 0;
    }
  } 

}

udError vcBPA_ConvertReadPoints(udConvertCustomItem *pConvertInput, udPointBufferF64 *pBuffer)
{
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pConvertInput->pData;
  pBuffer->pointCount = 0;
  if (pData->running.Get() == vcBPARS_Success)
    return udE_Success;  

  uint32_t displacementOffset = 0;
  udUInt3 displacementDistanceOffset = {};
  udError error = udE_Failure;

  error = udAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacement", &displacementOffset);
  if (error != udE_Success)
    return error;

  error = udAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionX", &displacementDistanceOffset.x);
  if (error != udE_Success)
    return error;

  error = udAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionY", &displacementDistanceOffset.y);
  if (error != udE_Success)
    return error;

  error = udAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionZ", &displacementDistanceOffset.z);
  if (error != udE_Success)
    return error;

  while (true)
  {
    if (pData->activeItem.pNewModelGrid == nullptr || (pData->activeItem.pNewModelGrid != nullptr && pData->activeItem.leftPoint == 0))
    {
      pData->CleanReadData();
      vcBPA_WaitNextGrid(pData);

      // done
      if (pData->activeItem.pNewModelGrid == nullptr)
      {
        pData->CleanReadData();
        pData->running.Set(vcBPARS_Success);
        return udE_Success;
      }

      while (pData->hashIndex < HASH_ARRAY_LEN)
      {
        vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNodeBlock *pBlock = pData->activeItem.pNewModelGrid->voxelHashMap.pArray[pData->hashIndex];
        if (pBlock != nullptr)
        {
          vcBPAHashMap<HASHKEY, vcBPAVoxel>::HashNode *pNode = pBlock->pBlockData[0];
          if (pNode != nullptr)
          {
            pData->hashKey = pNode->hashKey;

            vcBPAVoxel &voxel = pNode->data;            
            pData->ppVertex = udAllocType(vcBPAVertex *, voxel.pointNum, udAF_Zero);
            voxel.ToArray(pData->ppVertex);
            pData->pointIndex = 0;
            pData->arrayLength = voxel.pointNum;
            break;
          }
        }
        ++pData->hashIndex;
      }

      if (pData->hashIndex == HASH_ARRAY_LEN)
      {
        udDebugPrintf("read new grid error. \n");
        pData->CleanReadData();
        pData->running.Set(vcBPARS_Failed);
        return udE_Failure;
      }
    }

    vcBPA_ReadGrid(pData, pBuffer, displacementOffset, displacementDistanceOffset);

    if(pBuffer->pointCount == pBuffer->pointsAllocated)
      return udE_Success;
  }

  return udE_Success;
}

void vcBPA_ConvertClose(udConvertCustomItem *pConvertInput)
{
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pConvertInput->pData;
  if (pData->running.Get() == vcBPARS_Active)
    pData->running.Set(vcBPARS_Close);

  pData->Deinit();
}

void vcBPA_ConvertDestroy(udConvertCustomItem *pConvertInput)
{
  vcBPAConvertItem *pBPA = (vcBPAConvertItem*)pConvertInput->pData;
  udPointCloud_Unload(&pBPA->pOldModel);
  udPointCloud_Unload(&pBPA->pNewModel);
  udAttributeSet_Destroy(&pConvertInput->attributes);
  udFree(pConvertInput->pData);
}

void vcBPA_CompareExport(vcState *pProgramState, const char *pOldModelPath, const char *pNewModelPath, double ballRadius, double gridSize, const char *pName)
{
  vcConvertItem *pConvertItem = nullptr;
  vcConvert_AddEmptyJob(pProgramState, &pConvertItem);
  udLockMutex(pConvertItem->pMutex);

  udPointCloudHeader header = {};
  vcBPAConvertItem *pBPA = udAllocType(vcBPAConvertItem, 1, udAF_Zero);
  pBPA->pContext = pProgramState->pUDSDKContext;
  udPointCloud_Load(pBPA->pContext, &pBPA->pOldModel, pOldModelPath, nullptr);
  udPointCloud_Load(pBPA->pContext, &pBPA->pNewModel, pNewModelPath, &header);
  pBPA->pConvertItem = pConvertItem;
  pBPA->running.Set(vcBPARS_Active);
  pBPA->ballRadius = ballRadius;
  pBPA->gridSize = gridSize; // metres

  udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
  udDouble3 boundingBoxCenter = udDouble3::create(header.boundingBoxCenter[0], header.boundingBoxCenter[1], header.boundingBoxCenter[2]);
  udDouble3 boundingBoxExtents = udDouble3::create(header.boundingBoxExtents[0], header.boundingBoxExtents[1], header.boundingBoxExtents[2]);

  const char *pMetadata = nullptr;
  udPointCloud_GetMetadata(pBPA->pNewModel, &pMetadata);
  udJSON metadata = {};
  metadata.Parse(pMetadata);

  for (uint32_t i = 0; i < metadata.MemberCount() ; ++i)
  {
    const udJSON *pElement = metadata.GetMember(i);
    // Removed error checking because convertInfo metadata triggers vE_NotSupported
    udConvert_SetMetadata(pConvertItem->pConvertContext, metadata.GetMemberName(i), pElement->AsString());
  }

  udConvertCustomItem item = {};
  item.pName = pName;

  uint32_t displacementOffset = 0;
  bool addDisplacement = true;

  uint32_t displacementDirectionOffset = 0;
  bool addDisplacementDirection = true;

  if (udAttributeSet_GetOffsetOfNamedAttribute(&header.attributes, "udDisplacement", &displacementOffset) == udE_Success)
    addDisplacement = false;

  if (udAttributeSet_GetOffsetOfNamedAttribute(&header.attributes, "udDisplacementDirectionX", &displacementDirectionOffset) == udE_Success)
    addDisplacementDirection = false;

  udAttributeSet_Create(&item.attributes, udSAC_None, header.attributes.count + (addDisplacement ? 1 : 0) + (addDisplacementDirection ? 3 : 0));

  for (uint32_t i = 0; i < header.attributes.count; ++i)
  {
    item.attributes.pDescriptors[i].blendType = header.attributes.pDescriptors[i].blendType;
    item.attributes.pDescriptors[i].typeInfo = header.attributes.pDescriptors[i].typeInfo;
    udStrcpy(item.attributes.pDescriptors[i].name, header.attributes.pDescriptors[i].name);
    ++item.attributes.count;
  }

  if (addDisplacement)
  {
    item.attributes.pDescriptors[item.attributes.count].blendType = udABT_FirstChild;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = udATI_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacement");
    ++item.attributes.count;
  }

  if (addDisplacementDirection)
  {
    item.attributes.pDescriptors[item.attributes.count].blendType = udABT_FirstChild;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = udATI_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacementDirectionX");
    ++item.attributes.count;

    item.attributes.pDescriptors[item.attributes.count].blendType = udABT_FirstChild;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = udATI_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacementDirectionY");
    ++item.attributes.count;

    item.attributes.pDescriptors[item.attributes.count].blendType = udABT_FirstChild;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = udATI_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacementDirectionZ");
    ++item.attributes.count;
  }

  item.sourceResolution = header.convertedResolution;
  item.pointCount = metadata.Get("UniquePointCount").AsInt64(); // This will need a small scale applied (~1.2 seems to be correct for models pfox tested)
  item.pointCountIsEstimate = true;
  item.pData = pBPA;
  item.pOpen = vcBPA_ConvertOpen;
  item.pReadPointsFloat = vcBPA_ConvertReadPoints;
  item.pClose = vcBPA_ConvertClose;
  item.pDestroy = vcBPA_ConvertDestroy;
  udDouble4 temp = storedMatrix * udDouble4::create(boundingBoxCenter - boundingBoxExtents, 1.0);
  for (uint32_t i = 0; i < udLengthOf(item.boundMin); ++i)
    item.boundMin[i] = temp[i];

  temp = storedMatrix * udDouble4::create(boundingBoxCenter + boundingBoxExtents, 1.0);
  for (uint32_t i = 0; i < udLengthOf(item.boundMax); ++i)
    item.boundMax[i] = temp[i];

  item.boundsKnown = true;
  udConvert_AddCustomItem(pConvertItem->pConvertContext, &item);

  udFree(item.pName);
  udReleaseMutex(pBPA->pConvertItem->pMutex);
}

#endif //VC_HASCONVERT
