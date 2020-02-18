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

void FindBound(const udDouble3 *pPoints, size_t pointNum
  , double &oMinX, double &oMinY
  , double &oMaxX, double &oMaxY)
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

bool vcCDT_ProcessOrignal(const udDouble3 *pWaterPoints, size_t pointNum,
  const std::vector< std::pair<const udDouble3 *, size_t> > &islandPoints
  , udDouble2 &oMin
  , udDouble2 &oMax
  , std::vector<udDouble2> *pResult)
{
  if (pointNum < 3)
    return false;

  udDouble2 origin = pWaterPoints[0].toVector2();
  oMin = udDouble2::zero();
  oMax = udDouble2::zero();
  FindBound(pWaterPoints, pointNum, oMin.x, oMin.y, oMax.x, oMax.y);
  oMin -= origin;
  oMax -= origin;

  p2t::CDT *cdt = nullptr;
  std::vector<p2t::Point *> polyline;
  std::vector<p2t::Triangle *> triangles;  
  std::vector< std::vector<p2t::Point *> > vps;

  for (size_t i = 0; i < pointNum; i++)
  {
    polyline.push_back(new p2t::Point(pWaterPoints[i].x, pWaterPoints[i].y));
  }

  cdt = new p2t::CDT(polyline);

  for (auto island : islandPoints)
  {
    std::vector<p2t::Point *> vp;
    // 'island.second - 1' because I've assumed point list is a closed loop (last node matches first)
    for (size_t i= 0; i < island.second - 1; i ++)
    {
      const udDouble2& p = island.first[i].toVector2();
      vp.push_back(new p2t::Point(p.x, p.y));
    }
    cdt->AddHole(vp);

    vps.push_back(vp);
  }

  cdt->Triangulate();
  triangles = cdt->GetTriangles();

  for (auto tri : triangles)
  {
    pResult->push_back(udDouble2::create(tri->GetPoint(1)->x, tri->GetPoint(1)->y) - origin);
    pResult->push_back(udDouble2::create(tri->GetPoint(2)->x, tri->GetPoint(2)->y) - origin);
    pResult->push_back(udDouble2::create(tri->GetPoint(0)->x, tri->GetPoint(0)->y) - origin);
  }

  delete cdt;  

  for (auto p : polyline)
    delete p;
  polyline.clear();

  triangles.clear();

  for (auto vp : vps)
  {
    for (auto p : vp)
      delete p;
    vp.clear();
  }
  vps.clear();  

  return true;
}
