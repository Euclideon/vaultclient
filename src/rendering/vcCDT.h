#ifndef vcCDT_h__
#define vcCDT_h__
/*
* Constrained Delaunay Triangulation
* Using the 3rd library poly2tri.
*/

#include "vcMath.h"
#include <vector>

// triangulate a contour/polygon, places results in STL vector as series of triangles.
bool vcCDT_Process(const udDouble2 *pWaterPoints, size_t pointNum,
  const std::vector< std::pair<const udDouble2*, size_t> > &islandPoints
  , udDouble2 &oMin
  , udDouble2 &oMax
  , udDouble4x4 &oOrigin
  , std::vector<udDouble2> *pResult);


#endif
