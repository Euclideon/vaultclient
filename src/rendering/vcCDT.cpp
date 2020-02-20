 /*
 * Constrained Delaunay Triangulation
 * Using the 3rd library poly2tri.
 */
#include "vcCDT.h"
#include "poly2tri/sweep.h"
#include <udPlatformUtil.h>
#include <fstream>

bool vcCDT_Process(const udDouble2 *pWaterPoints, size_t pointNum,
  const std::vector< std::pair<const udDouble2 *, size_t> > &islandPoints
  , udDouble2 &oMin
  , udDouble2 &oMax
  , udDouble4x4 &oOrigin
  , std::vector<udDouble2> *pResult)
{
  if (pointNum < 3)
    return false;

  udResult result = udR_Success;
  int i = 0;

  oMin = udDouble2::zero();
  oMax = udDouble2::zero();
  udDouble2 origin = pWaterPoints[0];
  oOrigin = udDouble4x4::translation(origin.x, origin.y, 0.0f); 

  p2t::Sweep *pCDT = new p2t::Sweep();
  std::vector< p2t::Point * > pLocalIslands;
  
  p2t::Point *pLocalWater = udAllocType(p2t::Point, pointNum, udAF_Zero);
  UD_ERROR_NULL(pLocalWater, udR_MemoryAllocationFailure);

  // add water vertices
  for (i = 0; i < pointNum; i++)
  {
    pLocalWater[i].x = pWaterPoints[i].x - origin.x;
    pLocalWater[i].y = pWaterPoints[i].y - origin.y;
  }
  pCDT->AddPoints(pLocalWater, pointNum);

  // add islands vertices
  for (auto island : islandPoints)
  {
    p2t::Point *pArr = udAllocType(p2t::Point, island.second, udAF_Zero);
    UD_ERROR_NULL(pArr, udR_MemoryAllocationFailure);
    for (i= 0; i < island.second; i ++)
    {
      pArr[i].x = island.first[i].x - origin.x;
      pArr[i].y = island.first[i].y - origin.y;
    }
    pCDT->AddPoints(pArr, island.second);
    pLocalIslands.push_back(pArr);
  }

  // get water bound
  pCDT->InitTriangulation(oMin.x, oMin.y, oMax.x, oMax.y);

  // triangulation process
  pCDT->Triangulate();
  pCDT->FinalizationPolygon(pResult);

  // clean up
  delete pCDT;

  if (pLocalWater != nullptr)
    udFree(pLocalWater);
  for (auto pArr : pLocalIslands)
  {
    if (pArr != nullptr)
      udFree(pArr);
  }

  return true;

epilogue:
  if (pLocalWater != nullptr)
    udFree(pLocalWater);
  for (auto pArr : pLocalIslands)
  {
    if (pArr != nullptr)
      udFree(pArr);
  }

  return false;
}
