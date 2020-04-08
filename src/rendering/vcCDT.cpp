/*
 * Poly2Tri Copyright (c) 2009-2018, Poly2Tri Contributors
 * https://github.com/jhasse/poly2tri
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /*
 * Constrained Delaunay Triangulation
 * Modified by Asuna Wu 2020-2-18
 */
#include "vcCDT.h"
#include "poly2tri.h"
#include "udNew.h"

void vcCDT_FindBound(const udDouble3 *pPoints, size_t pointNum, double &oMinX, double &oMinY, double &oMaxX, double &oMaxY)
{
  oMinX = pPoints[0].x;
  oMinY = pPoints[0].y;
  oMaxX = oMinX;
  oMaxY = oMinY;
  for (size_t i = 0; i < pointNum; ++i)
  {
    const udDouble2 &p = pPoints[i].toVector2();
    oMinX = udMin(oMinX, p.x);
    oMinY = udMin(oMinY, p.y);
    oMaxX = udMax(oMaxX, p.x);
    oMaxY = udMax(oMaxY, p.y);
  }
}

bool vcCDT_ProcessOrignal(const udDouble3 *pWaterPoints, size_t pointNum, const std::vector< std::pair<const udDouble3 *, size_t> > &islandPoints, udDouble2 &oMin, udDouble2 &oMax, std::vector<udDouble2> *pResult)
{
  udResult result;

  if (pointNum < 3)
    return false;

  p2t::CDT *pCDT = nullptr;
  std::vector<p2t::Point *> polyline;
  std::vector< std::vector<p2t::Point *> > vps;
  std::vector<p2t::Triangle *> triangles;
  udDouble2 origin = pWaterPoints[0].toVector2();

  oMin = udDouble2::zero();
  oMax = udDouble2::zero();
  vcCDT_FindBound(pWaterPoints, pointNum, oMin.x, oMin.y, oMax.x, oMax.y);
  oMin -= origin;
  oMax -= origin;

  for (size_t i = 0; i < pointNum; i++)
    polyline.push_back(udNew(p2t::Point, pWaterPoints[i].x, pWaterPoints[i].y));

  pCDT = udNew(p2t::CDT, polyline);
  UD_ERROR_NULL(pCDT, udR_MemoryAllocationFailure);

  for (std::pair<const udDouble3*, size_t> island : islandPoints)
  {
    std::vector<p2t::Point *> vp;
    // don't process last element because I've assumed point list is a closed loop (last node matches first)
    for (size_t i= 0; i < island.second - 1; i ++)
    {
      const udDouble2& p = island.first[i].toVector2();
      vp.push_back(udNew(p2t::Point, p.x, p.y));
    }
    pCDT->AddHole(vp);
    vps.push_back(vp);
  }

  pCDT->Triangulate();
  triangles = pCDT->GetTriangles();
  for (p2t::Triangle* pTriangle : triangles)
  {
    pResult->push_back(udDouble2::create(pTriangle->GetPoint(1)->x, pTriangle->GetPoint(1)->y) - origin);
    pResult->push_back(udDouble2::create(pTriangle->GetPoint(2)->x, pTriangle->GetPoint(2)->y) - origin);
    pResult->push_back(udDouble2::create(pTriangle->GetPoint(0)->x, pTriangle->GetPoint(0)->y) - origin);
  }

  result = udR_Success;
epilogue:

  udDelete(pCDT);
  for (p2t::Point *pPoint : polyline)
    udDelete(pPoint);
  for (std::vector<p2t::Point *> vp : vps)
  {
    for (p2t::Point *pPoint : vp)
      udDelete(pPoint);
  }

  return result == udR_Success;
}
