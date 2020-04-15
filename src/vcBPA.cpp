#include "vcBPA.h"

#include "vcMath.h"
#include "vcConvert.h"

#include "vdkConvert.h"
#include "vdkConvertCustom.h"
#include "vdkQuery.h"

#include "udChunkedArray.h"
#include "udWorkerPool.h"
#include "udSafeDeque.h"
#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#if VC_HASCONVERT
enum vcBPAEdgeStatus
{
  vcBPAES_Active, // This an edge that will be used for pivoting
  vcBPAES_Boundary, // This is an edge that can't be used for pivoting
  vcBPAES_Frozen, // This is an edge that is outside the current processing region (This is for Out-of-core support)
};

struct vcBPATriangle;

struct vcBPAVertex
{
  udDouble3 position;
  bool used;
};

struct vcBPAEdge
{
  udULong2 edge;
  vcBPAEdgeStatus status;

  vcBPATriangle *pTriangle;

  static vcBPAEdge create(udULong2 edge, vcBPAEdgeStatus status, vcBPATriangle *pTriangle)
  {
    return{ edge, status, pTriangle };
  }
};

struct vcBPATriangle
{
  udULong3 vertices;
  udDouble3 ballCenter; // Ball used to construct triangle

  static vcBPATriangle create(udULong3 vertices, udDouble3 ballCenter)
  {
    return{ vertices, ballCenter };
  }
};

struct vcBPAGrid
{
  udDouble3 center;
  bool visited;

  vdkPointBufferF64 *pBuffer;
  udChunkedArray<vcBPAVertex> vertices;
  udChunkedArray<vcBPATriangle> triangles;
  udChunkedArray<vcBPAEdge> edges;

  size_t lastSeedTriangleIndex;
  size_t activeEdgeIndex;
  udDouble3 minPos;
  udDouble3 maxPos;

  static vcBPAGrid create(udDouble3 center, udDouble3 minPos, udDouble3 maxPos)
  {
    vcBPAGrid grid = {};
    grid.center = center;
    grid.visited = false;
    grid.activeEdgeIndex = 0;
    grid.minPos = minPos;
    grid.maxPos = maxPos;
    return grid;
  }

  void Init(vdkAttributeSet *pAttributes)
  {
    vertices.Init(512);
    triangles.Init(512);
    edges.Init(512);
    vdkPointBufferF64_Create(&pBuffer, 1 << 20, pAttributes);
  }

  void Deinit()
  {
    vdkPointBufferF64_Destroy(&pBuffer);
    edges.Deinit();
    triangles.Deinit();
    vertices.Deinit();
  }
};

struct vcBPAOctNode
{
  udDouble3 center;
  udDouble3 extents;
  bool visited;

  static vcBPAOctNode create(udDouble3 center, udDouble3 extents)
  {
    vcBPAOctNode node = {};
    node.center = center;
    node.extents = extents;
    node.visited = false;
    return node;
  }
};

struct vcBPAManifold
{
  udChunkedArray<vcBPAGrid> grids;
  udChunkedArray<vcBPAOctNode> nodes;

  udWorkerPool *pPool;

  double gridSize;
  double ballRadius;
  vdkContext *pContext;
};

void vcBPA_Init(vcBPAManifold **ppManifold, vdkContext *pContext)
{
  vcBPAManifold *pManifold = udAllocType(vcBPAManifold, 1, udAF_Zero);
  pManifold->grids.Init(1 << 10);
  pManifold->nodes.Init(1 << 13);
  udWorkerPool_Create(&pManifold->pPool, (uint8_t)udGetHardwareThreadCount(), "vcBPAPool");
  pManifold->pContext = pContext;
  *ppManifold = pManifold;
}

void vcBPA_Deinit(vcBPAManifold **ppManifold)
{
  if (ppManifold == nullptr || *ppManifold == nullptr)
    return;

  vcBPAManifold *pManifold = *ppManifold;
  *ppManifold = nullptr;
  udWorkerPool_Destroy(&pManifold->pPool);
  pManifold->nodes.Deinit();
  for (size_t i = 0; i < pManifold->grids.length; ++i)
    pManifold->grids[i].Deinit();

  pManifold->grids.Deinit();
  udFree(pManifold);
}

bool vcBPA_NodeHasData(vcBPAManifold *pManifold, vdkPointCloud *pModel, vcBPAOctNode *pNode)
{
  bool hasData = false;
  vdkPointBufferF64 *pBuffer = nullptr;
  vdkPointBufferF64_Create(&pBuffer, 1, nullptr);

  vdkQueryFilter *pFilter = nullptr;
  vdkQueryFilter_Create(&pFilter);
  udDouble3 zero = udDouble3::zero();
  vdkQueryFilter_SetAsBox(pFilter, &pNode->center.x, &pNode->extents.x, &zero.x);

  vdkQuery *pQuery = nullptr;
  vdkQuery_Create(pManifold->pContext, &pQuery, pModel, pFilter);
  vdkQuery_ExecuteF64(pQuery, pBuffer);
  vdkQuery_Destroy(&pQuery);

  vdkQueryFilter_Destroy(&pFilter);

  hasData = (pBuffer->pointCount != 0);

  vdkPointBufferF64_Destroy(&pBuffer);

  return hasData;
}

void vcBPA_PopulateGrid(vdkContext *pContext, vdkPointCloud *pModel, vdkAttributeSet *pAttributes, udDouble3 aabbCenter, udDouble3 aabbExtents, vcBPAGrid *pGrid, bool *pHasPoints)
{
  pGrid->Init(pAttributes);

  vdkQueryFilter *pFilter = nullptr;
  vdkQueryFilter_Create(&pFilter);
  udDouble3 zero = udDouble3::zero();
  vdkQueryFilter_SetAsBox(pFilter, &aabbCenter.x, &aabbExtents.x, &zero.x);

  vdkQuery *pQuery = nullptr;
  vdkQuery_Create(pContext, &pQuery, pModel, pFilter);
  vdkQuery_ExecuteF64(pQuery, pGrid->pBuffer);
  vdkQuery_Destroy(&pQuery);
  vdkQueryFilter_Destroy(&pFilter);

  pGrid->vertices.ReserveBack(pGrid->pBuffer->pointCount);
  for (uint32_t j = 0; j < pGrid->pBuffer->pointCount; ++j)
  {
    udDouble3 position = udDouble3::create(pGrid->pBuffer->pPositions[j * 3 + 0], pGrid->pBuffer->pPositions[j * 3 + 1], pGrid->pBuffer->pPositions[j * 3 + 2]);

    if (udPointInAABB(position, pGrid->minPos, pGrid->maxPos))
      *pHasPoints = true;

    pGrid->vertices.PushBack({ position, false });
  }
}

bool vcBPA_GetGrid(vcBPAManifold *pManifold, vdkPointCloud *pModel, vdkAttributeSet *pAttributes, vcBPAGrid **ppGrid)
{
  double startTime = udGetEpochMilliSecsUTCf();

  // Iterate octree to find grids
  for (size_t i = 0; i < pManifold->nodes.length; ++i)
  {
    if (pManifold->nodes[i].visited)
      continue;

    pManifold->nodes[i].visited = true;

    // Don't descend if the extents is smaller than the grid size, instead add grid to list
    if (udMaxElement(pManifold->nodes[i].extents) < pManifold->gridSize)
    {
      udDouble3 minPos = pManifold->nodes[i].center - pManifold->nodes[i].extents;
      udDouble3 maxPos = pManifold->nodes[i].center + pManifold->nodes[i].extents;
      pManifold->grids.PushBack(vcBPAGrid::create(pManifold->nodes[i].center, minPos, maxPos));

      continue;
    }

    // Don't descend if there's no data
    if (!vcBPA_NodeHasData(pManifold, pModel, pManifold->nodes.GetElement(i)))
      continue;

    udDouble3 extents = pManifold->nodes[i].extents / 2.0;

    // Iterate each octant, hence 8
    for (int j = 0; j < 8; ++j)
    {
      udDouble3 center = pManifold->nodes[i].center;

      // Generate new center position based on which octant (j) that we're in
      for (int k = 0; k < udDouble3::ElementCount; ++k)
      {
        if ((j & (1 << k)) == 0)
          center[k] -= extents[k];
        else
          center[k] += extents[k];
      }

      pManifold->nodes.PushBack(vcBPAOctNode::create(center, extents));
    }
  }

  printf("Processing Grids (now at %.3f)\n", udGetEpochMilliSecsUTCf() - startTime);
  startTime = udGetEpochMilliSecsUTCf();

  // Iterate generated grids and return valid grids
  for (size_t i = 0; i < pManifold->grids.length; ++i)
  {
    if (pManifold->grids[i].visited)
      continue;

    pManifold->grids[i].visited = true;

    udDouble3 aabbCenter = pManifold->grids[i].center;
    udDouble3 aabbExtents = udDouble3::create((pManifold->grids[i].maxPos - pManifold->grids[i].minPos) / 2.0);

    bool hasPoints = false;
    vcBPA_PopulateGrid(pManifold->pContext, pModel, pAttributes, aabbCenter, aabbExtents, pManifold->grids.GetElement(i), &hasPoints);

    if (!hasPoints)
    {
      pManifold->grids[i].Deinit();
      continue;
    }

    if (pManifold->grids[i].vertices.length == 0)
      continue;

    *ppGrid = pManifold->grids.GetElement(i);

    printf("Processing Grids SUCCESS (now at %.3f)\n", udGetEpochMilliSecsUTCf() - startTime);
    return true;
  }

  printf("Processing Grids FAILED (now at %.3f)\n", udGetEpochMilliSecsUTCf() - startTime);
  return false;
}

int vcBPA_GetPointsInBallCount(vcBPAGrid *pGrid, const udChunkedArray<uint64_t> &nearbyPoints, udDouble3 ballCenter, double ballRadius)
{
  int count = 0;

  for (size_t i = 0; i < nearbyPoints.length; )
  {
    size_t runLen = nearbyPoints.GetElementRunLength(i);
    const uint64_t *pVertex = nearbyPoints.GetElement(i);
    for (size_t j = 0; j < runLen; ++j)
    {
      double diff = (ballRadius * ballRadius) - udMagSq3(pGrid->vertices[pVertex[j]].position - ballCenter);
      if (diff > (-FLT_EPSILON))
        ++count;
    }

    i += runLen;
  }

  return count;
}

int vcBPA_GetPointsInBallCount(vcBPAGrid *pGrid, udDouble3 ballCenter, double ballRadius)
{
  int count = 0;
  for (size_t i = 0; i < pGrid->vertices.length; )
  {
    size_t runLen = pGrid->vertices.GetElementRunLength(i);
    const vcBPAVertex *pVertex = pGrid->vertices.GetElement(i);

    for (size_t j = 0; j < runLen; ++j)
    {
      double diff = (ballRadius * ballRadius) - udMagSq3(pVertex[j].position - ballCenter);
      if (diff > (-FLT_EPSILON))
        ++count;
    }

    i += runLen;
  }

  return count;
}

bool vcBPA_NotUsed(vcBPAGrid *pGrid, uint64_t vertex)
{
  return !pGrid->vertices[vertex].used;
}

void vcBPA_GetNearbyPoints(vcBPAGrid *pGrid, udChunkedArray<uint64_t> *pPoints, uint64_t point, double ballRadius)
{
  double ballDiameterSq = (2 * ballRadius) * (2 * ballRadius);
  udDouble3 p0 = pGrid->vertices[point].position;
  for (uint64_t i = 0; i < (uint64_t)pGrid->vertices.length; ++i)
  {
    if (i == point)
      continue;

    udDouble3 p1 = pGrid->vertices[i].position;
    double d01 = udMagSq3(p0 - p1);

    if (d01 > ballDiameterSq)
      continue;

    size_t j = 0;
    for (; j < pPoints->length; ++j)
    {
      udDouble3 p2 = pGrid->vertices[*pPoints->GetElement(j)].position;

      if (udMagSq3(p0 - p2) > d01)
        break;
    }
    pPoints->Insert(j, &i);
  }
}

udDouble3 vcBPA_GetTriangleNormal(const udDouble3 & p0, const udDouble3 & p1, const udDouble3 & p2)
{
  udDouble3 a = p1 - p0;
  udDouble3 b = p2 - p0;
  return udNormalize(udCross(a, b));
}

vcBPATriangle vcBPA_FindSeedTriangle(vcBPAGrid *pGrid, double ballRadius)
{
  // Pick any point not already in the triangle list
  size_t pointIndex = pGrid->lastSeedTriangleIndex;
  do
  {
    for (; pointIndex < pGrid->vertices.length; ++pointIndex)
    {
      if (vcBPA_NotUsed(pGrid, pointIndex))
        break;
    }

    if (pointIndex == pGrid->vertices.length)
      break;

    udDouble3 p0 = pGrid->vertices[pointIndex].position;

    // Reject point if it's outside the grid min/max positions
    if (!udPointInAABB(p0, pGrid->minPos, pGrid->maxPos))
    {
      ++pointIndex;
      continue;
    }

    udChunkedArray<uint64_t> nearbyPoints;
    nearbyPoints.Init(32);
    vcBPA_GetNearbyPoints(pGrid, &nearbyPoints, pointIndex, ballRadius);

    // Consider all pairs of points in its neighbourhood in order of distance from the point
    for (size_t i = 0; i < nearbyPoints.length; ++i)
    {
      if (!vcBPA_NotUsed(pGrid, nearbyPoints[i]))
        continue;

      udDouble3 p1 = pGrid->vertices[nearbyPoints[i]].position;

      for (size_t j = i + 1; j < nearbyPoints.length; ++j)
      {
        if (!vcBPA_NotUsed(pGrid, nearbyPoints[j]))
          continue;

        udDouble3 p2 = pGrid->vertices[nearbyPoints[j]].position;

        // Build potential seed triangle
        udULong3 triangle = udULong3::create((uint64_t)pointIndex, (uint64_t)nearbyPoints[i], (uint64_t)nearbyPoints[j]);

        // Check that the triangle normal is consistent with the vertex normals - i.e. pointing outward
        udDouble3 triangleNormal = vcBPA_GetTriangleNormal(p0, p1, p2);
        if (triangleNormal.z < 0) // Normal is pointing down, that's bad!
        {
          triangle.x = (uint64_t)nearbyPoints[i];
          triangle.y = (uint64_t)pointIndex;
        }

        // Test that a p-ball with center in the outward halfspace touches all three vertices and contains no other points
        udDouble3 ballCenter = udGetSphereCenterFromPoints(ballRadius, pGrid->vertices[triangle.x].position, pGrid->vertices[triangle.y].position, pGrid->vertices[triangle.z].position);

        // Invalid triangle
        if (ballCenter == udDouble3::zero())
          continue;

        // nearbyPoints doesn't contain pointIndex so this will return 2 points instead of 3
        int pointsInBall = vcBPA_GetPointsInBallCount(pGrid, nearbyPoints, ballCenter, ballRadius);

        // Ball contains points, invalid triangle
        if (pointsInBall > 2)
          continue;

        UDASSERT(pointsInBall == 2, "The ball should contain at least the three points of the triangle!");

        nearbyPoints.Deinit();

        pGrid->lastSeedTriangleIndex = pointIndex;

        // Return first valid triangle
        return vcBPATriangle::create(triangle, ballCenter);
      }
    }

    nearbyPoints.Deinit();
    ++pointIndex;
  } while (pointIndex < pGrid->vertices.length);

  pGrid->lastSeedTriangleIndex = pGrid->vertices.length;
  return vcBPATriangle::create(udULong3::zero(), udDouble3::zero());
}

vcBPATriangle *vcBPA_OutputTriangle(vcBPAGrid *pGrid, vcBPATriangle triangle)
{
  pGrid->triangles.PushBack(triangle);
  for (int i = 0; i < 3; ++i)
    pGrid->vertices[triangle.vertices[i]].used = true;

  return pGrid->triangles.GetElement(pGrid->triangles.length - 1);
}

void vcBPA_InsertEdge(vcBPAGrid *pGrid, vcBPAEdge edge)
{
  // Mark edge as frozen if any point lies outside the grid
  for (int i = 0; i < 2; ++i)
  {
    udDouble3 point = pGrid->vertices[edge.edge[i]].position;
    if (!udPointInAABB(point, pGrid->minPos, pGrid->maxPos))
      edge.status = vcBPAES_Frozen;
  }

  pGrid->edges.Insert(pGrid->activeEdgeIndex, &edge);
}

bool vcBPA_GetActiveEdge(vcBPAGrid *pGrid, vcBPAEdge *pEdge)
{
  if (pGrid->edges.length == 0)
    return false;

  size_t i;
  // All edges before the active edge should already be processed.
  // New edges are inserted before the active edge and are processed then.
  for (i = pGrid->activeEdgeIndex; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].status == vcBPAES_Active)
      break;
  }

  // Try from the start until the active edge
  if (i == pGrid->edges.length)
  {
    for (i = 0; i < pGrid->activeEdgeIndex; ++i)
    {
      if (pGrid->edges[i].status == vcBPAES_Active)
        break;
    }

    if (i == pGrid->activeEdgeIndex)
      return false;
  }

  pGrid->activeEdgeIndex = i;
  *pEdge = pGrid->edges[i];
  return true;
}

bool vcBPA_BallPivot(vcBPAGrid *pGrid, double ballRadius, vcBPAEdge edge, uint64_t *pVertex, udDouble3 *pBallCenter)
{
  udChunkedArray<vcBPATriangle> triangles;
  triangles.Init(32);
  for (size_t i = 0; i < pGrid->vertices.length; ++i)
  {
    // Skip denegerate triangles and the triangle that made this edge
    if (i == edge.pTriangle->vertices.x || i == edge.pTriangle->vertices.y || i == edge.pTriangle->vertices.z)
      continue;

    // Build potential seed triangle
    udULong3 triangle = udULong3::create((uint64_t)edge.edge.x, (uint64_t)i, (uint64_t)edge.edge.y);
    udDouble3 vertices[3] = {
      pGrid->vertices[triangle.x].position,
      pGrid->vertices[triangle.y].position,
      pGrid->vertices[triangle.z].position
    };

    // Check that the triangle normal is consistent with the vertex normals - i.e. pointing outward
    udDouble3 triangleNormal = vcBPA_GetTriangleNormal(vertices[0], vertices[1], vertices[2]);
    if (triangleNormal.z < 0) // Normal is pointing down, that's bad!
      continue;

    udDouble3 distances = {
      udMagSq3(vertices[0] - vertices[1]),
      udMagSq3(vertices[1] - vertices[2]),
      udMagSq3(vertices[2] - vertices[0])
    };
    double ballDiameterSq = (2 * ballRadius) * (2 * ballRadius);
    if (distances.x > ballDiameterSq || distances.y > ballDiameterSq || distances.z > ballDiameterSq)
      continue;

    udDouble3 ballCenter = udGetSphereCenterFromPoints(ballRadius, vertices[0], vertices[1], vertices[2]);

    // Invalid triangle
    if (ballCenter == udDouble3::zero())
      continue;

    int pointsInBall = vcBPA_GetPointsInBallCount(pGrid, ballCenter, ballRadius);

    // Ball contains points, invalid triangle
    if (pointsInBall > 3)
      continue;

    UDASSERT(pointsInBall == 3, "The ball should contain at least the three points of the triangle!");

    triangles.PushBack(vcBPATriangle::create(udULong3::create(edge.edge.x, i, edge.edge.y), ballCenter));
  }

  vcBPATriangle triangle = {};
  udDouble3 pivotPoint = (pGrid->vertices[edge.edge.y].position + pGrid->vertices[edge.edge.x].position) / udDouble3::create(2.0);

  for (size_t i = 0; i < triangles.length; ++i)
  {
    if (triangle.vertices == udULong3::zero())
    {
      triangle = triangles[i];
      continue;
    }

    udDouble3 points[3] = {
      edge.pTriangle->ballCenter - pivotPoint,
      triangle.ballCenter - pivotPoint,
      triangles[i].ballCenter - pivotPoint
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

  triangles.Deinit();

  if (triangle.vertices != udULong3::zero())
  {
    *pVertex = triangle.vertices.y;
    *pBallCenter = triangle.ballCenter;
    return true;
  }
  return false;
}

bool vcBPA_OnFront(vcBPAGrid *pGrid, uint64_t vertex)
{
  for (size_t i = 0; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].status != vcBPAES_Active)
      continue;

    for (int j = 0; j < 2; ++j)
    {
      if (pGrid->edges[i].edge[j] == vertex)
        return true;
    }
  }
  return false;
}

bool vcBPA_FrontContains(vcBPAGrid *pGrid, udULong2 edge)
{
  for (size_t i = 0; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].edge == edge)
      return true;
  }

  return false;
}

void vcBPA_Join(vcBPAGrid *pGrid, udULong2 edge, uint64_t vertex, vcBPATriangle *pTriangle)
{
  pGrid->edges.RemoveAt(pGrid->activeEdgeIndex);
  if (!vcBPA_FrontContains(pGrid, { vertex, edge.y }))
    vcBPA_InsertEdge(pGrid, vcBPAEdge::create({ vertex, edge.y }, vcBPAES_Active, pTriangle));

  if (!vcBPA_FrontContains(pGrid, { edge.x, vertex }))
    vcBPA_InsertEdge(pGrid, vcBPAEdge::create({ edge.x, vertex }, vcBPAES_Active, pTriangle));
}

void vcBPA_Glue(vcBPAGrid *pGrid, udULong2 edge)
{
  bool canBreak = false;
  for (size_t i = 0; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].edge == edge || pGrid->edges[i].edge == udULong2::create(edge.y, edge.x))
    {
      pGrid->edges.RemoveAt(i);

      if (pGrid->activeEdgeIndex > i)
        --pGrid->activeEdgeIndex;

      if (canBreak)
        break;

      canBreak = true;
      --i;
    }
  }
}

void vcBPA_MarkAsBoundary(vcBPAGrid *pGrid, vcBPAEdge edge)
{
  for (size_t i = 0; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].edge.x == edge.edge.x && pGrid->edges[i].edge.y == edge.edge.y)
    {
      pGrid->edges[i].status = vcBPAES_Boundary;
      if (pGrid->activeEdgeIndex > 0)
        --pGrid->activeEdgeIndex;
      break;
    }
  }
}

void vcBPA_UnfreezeEdges(vcBPAGrid *pGrid)
{
  for (size_t i = 0; i < pGrid->edges.length; ++i)
  {
    if (pGrid->edges[i].status != vcBPAES_Frozen)
      continue;

    for (int j = 0; j < 2; ++j)
    {
      udDouble3 point = pGrid->vertices[pGrid->edges[i].edge[j]].position;
      if (udPointInAABB(point, pGrid->minPos, pGrid->maxPos))
        pGrid->edges[i].status = vcBPAES_Active;
    }
  }
}

void vcBPA_DoGrid(vcBPAGrid *pGrid, double ballRadius)
{
  if (pGrid->vertices.length == 0)
    return;

  while (true)
  {
    vcBPAEdge edge = {};
    while (vcBPA_GetActiveEdge(pGrid, &edge))
    {
      uint64_t vertex = 0;
      udDouble3 ballCenter = {};
      if (vcBPA_BallPivot(pGrid, ballRadius, edge, &vertex, &ballCenter))
      {
        if (vcBPA_NotUsed(pGrid, vertex) || vcBPA_OnFront(pGrid, vertex))
        {
          if (!vcBPA_NotUsed(pGrid, vertex))
          {
            size_t i = 0;
            for (; i < pGrid->triangles.length; ++i)
            {
              udULong3 vertices = pGrid->triangles[i].vertices;
              if ((vertices.x == edge.edge.x || vertices.y == edge.edge.x || vertices.z == edge.edge.x) &&
                (vertices.x == vertex || vertices.y == vertex || vertices.z == vertex) &&
                (vertices.x == edge.edge.y || vertices.y == edge.edge.y || vertices.z == edge.edge.y))
              {
                break;
              }
            }

            // Remove the current edge, and skip adding the duplicate triangle
            if (i != pGrid->triangles.length)
            {
              pGrid->edges.RemoveAt(pGrid->activeEdgeIndex);
              continue;
            }
          }

          vcBPATriangle *pTriangle = vcBPA_OutputTriangle(pGrid, vcBPATriangle::create(udULong3::create(edge.edge.x, vertex, edge.edge.y), ballCenter));
          vcBPA_Join(pGrid, edge.edge, vertex, pTriangle);

          udULong2 edges[2] = { udULong2::create(vertex, edge.edge.x), udULong2::create(edge.edge.y, vertex) };
          if (vcBPA_FrontContains(pGrid, edges[0]))
            vcBPA_Glue(pGrid, edges[0]);

          if (vcBPA_FrontContains(pGrid, edges[1]))
            vcBPA_Glue(pGrid, edges[1]);
        }
        else
        {
          vcBPA_MarkAsBoundary(pGrid, edge);
        }
      }
      else
      {
        vcBPA_MarkAsBoundary(pGrid, edge);
      }
    }

    vcBPATriangle triangle = vcBPA_FindSeedTriangle(pGrid, ballRadius);
    if (triangle.vertices == udULong3::zero())
      break;

    vcBPATriangle *pTriangle = vcBPA_OutputTriangle(pGrid, triangle);
    vcBPA_InsertEdge(pGrid, vcBPAEdge::create({ triangle.vertices.z, triangle.vertices.x }, vcBPAES_Active, pTriangle));
    vcBPA_InsertEdge(pGrid, vcBPAEdge::create({ triangle.vertices.y, triangle.vertices.z }, vcBPAES_Active, pTriangle));
    vcBPA_InsertEdge(pGrid, vcBPAEdge::create({ triangle.vertices.x, triangle.vertices.y }, vcBPAES_Active, pTriangle));
  }
}

double vcBPA_DistanceToTriangle(vcBPAGrid *pOldGrid, size_t triangleIndex, udDouble3 position, udDouble3 *pPoint)
{
  udDouble3 vertices[3] = {
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.x].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.y].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.z].position,
  };

  return udDistanceToTriangle(vertices[0], vertices[1], vertices[2], position, pPoint);
}

double vcBPA_SignedDistanceToTriangle(vcBPAGrid *pOldGrid, size_t triangleIndex, udDouble3 position, udDouble3 *pPoint)
{
  udDouble3 vertices[3] = {
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.x].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.y].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.z].position,
  };

  double distance = udDistanceToTriangle(vertices[0], vertices[1], vertices[2], position, pPoint);
  udDouble3 normal = vcBPA_GetTriangleNormal(vertices[0], vertices[1], vertices[2]);
  udDouble3 p0_to_position = position - vertices[0];
  if (udDot3(p0_to_position, normal) < 0.0)
    distance = -distance;
  return distance;
}

size_t vcBPA_FindClosestTriangle(vcBPAGrid *pOldGrid, udDouble3 position)
{
  size_t closest = 0;
  double closestDistance = FLT_MAX;

  for (size_t i = 0; i < pOldGrid->triangles.length; ++i)
  {
    double distance = vcBPA_DistanceToTriangle(pOldGrid, i, position, nullptr);
    if (distance < closestDistance)
    {
      closest = i;
      closestDistance = distance;
    }
  }

  return closest;
}

struct vcBPAConvertItemData
{
  vcBPAManifold *pManifold;

  vcBPAGrid *pGrid;
  uint32_t pointIndex;

  vdkPointCloud *pOldModel;
  vcBPAGrid oldGrid;
};

struct vcBPAConvertItem
{
  vcBPAManifold *pManifold;
  vdkContext *pContext;
  vdkPointCloud *pOldModel;
  vdkPointCloud *pNewModel;
  vcConvertItem *pConvertItem;
  double gridSize;
  double ballRadius;

  udSafeDeque<vcBPAConvertItemData> *pQueueItems;
  udSafeDeque<vcBPAConvertItemData> *pConvertItemData;
  vcBPAConvertItemData activeItem;
  udThread *pThread;
  udInterlockedBool running;
};

void vcBPA_GridPopulationThread(void *pDataPtr)
{
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pDataPtr;
  vcBPAConvertItemData itemData = {};
  if (udSafeDeque_PopFront(pData->pQueueItems, &itemData) != udR_Success)
    return;

  udDouble3 minPos = itemData.pGrid->center - udDouble3::create(itemData.pManifold->gridSize / 2.0);
  udDouble3 maxPos = itemData.pGrid->center + udDouble3::create(itemData.pManifold->gridSize / 2.0);
  itemData.oldGrid = vcBPAGrid::create(itemData.pGrid->center, minPos, maxPos);

  udDouble3 aabbCenter = itemData.oldGrid.center;
  udDouble3 aabbExtents = udDouble3::create(itemData.pManifold->gridSize / 2.0);
  aabbExtents += udDouble3::create(2 * itemData.pManifold->ballRadius); // Add overlap

  bool hasPoints = false;
  vcBPA_PopulateGrid(itemData.pManifold->pContext, itemData.pOldModel, nullptr, aabbCenter, aabbExtents, &itemData.oldGrid, &hasPoints);
  vcBPA_DoGrid(&itemData.oldGrid, itemData.pManifold->ballRadius);

  udSafeDeque_PushBack(pData->pConvertItemData, itemData);
}

uint32_t vcBPA_GridGeneratorThread(void *pDataPtr)
{
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pDataPtr;

  vcBPAGrid *pGrid = nullptr;
  int i = 0;

  vdkPointCloudHeader header;
  vdkPointCloud_GetHeader(pData->pNewModel, &header);

  while (vcBPA_GetGrid(pData->pManifold, pData->pNewModel, &header.attributes, &pGrid) && pData->running)
  {
    vcBPAConvertItemData data = {};
    data.pManifold = pData->pManifold;
    data.pGrid = pGrid;
    data.pointIndex = 0;
    data.pOldModel = pData->pOldModel;

    udSafeDeque_PushBack(pData->pQueueItems, data);
    udWorkerPool_AddTask(pData->pManifold->pPool, vcBPA_GridPopulationThread, pData, false);
    ++i;

    // Catch up to avoid running out of memory
    if (i >= 100)
    {
      do
      {
        udLockMutex(pData->pQueueItems->pMutex);
        i = (int)pData->pQueueItems->chunkedArray.length;
        udReleaseMutex(pData->pQueueItems->pMutex);

        if (i < 50)
          break;

        udSleep(500);
      } while (pData->running);
    }
  }

  pData->running = false;
  return 0;
}

vdkError vcBPA_ConvertOpen(vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, vdkConvertCustomItemFlags flags)
{
  udUnused(everyNth);
  udUnused(origin);
  udUnused(pointResolution);
  udUnused(flags);

  vcBPAConvertItem *pData = (vcBPAConvertItem*)pConvertInput->pData;
  pData->running = true;

  vcBPA_Init(&pData->pManifold, pData->pContext);
  pData->pManifold->ballRadius = pData->ballRadius;
  pData->pManifold->gridSize = pData->gridSize;

  vdkPointCloudHeader header = {};
  vdkPointCloud_GetHeader(pData->pNewModel, &header);
  udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
  udDouble3 startAABBCenter = (storedMatrix * udDouble4::create(header.pivot[0], header.pivot[1], header.pivot[2], 1.0)).toVector3();

  udSafeDeque_Create(&pData->pQueueItems, 32);
  udSafeDeque_Create(&pData->pConvertItemData, 128);

  udDouble4 boundingBoxExtents = udDouble4::create(header.boundingBoxExtents[0], header.boundingBoxExtents[1], header.boundingBoxExtents[2], 1.0);
  boundingBoxExtents = storedMatrix * boundingBoxExtents;
  double nextPow2 = (double)udPowerOfTwoAbove((int64_t)udMaxElement(boundingBoxExtents));

  pData->pManifold->nodes.PushBack(vcBPAOctNode::create(startAABBCenter, udDouble3::create(nextPow2)));

  udThread_Create(&pData->pThread, vcBPA_GridGeneratorThread, pData, udTCF_None, "BPAGridGeneratorThread");
  while (pData->running && udSafeDeque_PopFront(pData->pConvertItemData, &pData->activeItem) != udR_Success)
    continue;

  return (!pData->running && pData->activeItem.pointIndex == 0 && pData->activeItem.pGrid == nullptr) ? vE_InvalidConfiguration : vE_Success;
}

vdkError vcBPA_ConvertReadPoints(vdkConvertCustomItem *pConvertInput, vdkPointBufferF64 *pBuffer)
{
  // Reset point count to avoid infinite loop
  pBuffer->pointCount = 0;

  vcBPAConvertItem *pData = (vcBPAConvertItem *)pConvertInput->pData;
  bool getNextGrid = true;

  uint32_t displacementOffset = 0;
  udUInt3 displacementDistanceOffset = {};
  vdkError error = vE_Failure;

  static int gridCount = 0;
  if (pData->activeItem.pointIndex == 0)
    ++gridCount;

  if (pData->activeItem.pointIndex == 0 && pData->activeItem.pGrid == nullptr)
    goto epilogue;

  error = vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacement", &displacementOffset);
  if (error != vE_Success)
    return error;

  error = vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionX", &displacementDistanceOffset.x);
  if (error != vE_Success)
    return error;

  error = vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionY", &displacementDistanceOffset.y);
  if (error != vE_Success)
    return error;

  error = vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacementDirectionZ", &displacementDistanceOffset.z);
  if (error != vE_Success)
    return error;

  for (size_t i = pData->activeItem.pointIndex; i < pData->activeItem.pGrid->vertices.length; ++i)
  {
    if (pBuffer->pointCount == pBuffer->pointsAllocated)
    {
      getNextGrid = false;
      pData->activeItem.pointIndex = (uint32_t)i;
      break;
    }

    udDouble3 position = pData->activeItem.pGrid->vertices[i].position;
    size_t triangle = vcBPA_FindClosestTriangle(&pData->activeItem.oldGrid, position);

    udDouble3 trianglePoint = {};

    double distance = FLT_MAX;
    if (triangle < pData->activeItem.oldGrid.triangles.length)
      distance = vcBPA_SignedDistanceToTriangle(&pData->activeItem.oldGrid, triangle, position, &trianglePoint);

    // Position XYZ
    memcpy(&pBuffer->pPositions[pBuffer->pointCount * 3], &pData->activeItem.pGrid->pBuffer->pPositions[i * 3], sizeof(double) * 3);

    // Copy all of the original attributes
    ptrdiff_t pointAttrOffset = ptrdiff_t(pBuffer->pointCount) * pBuffer->attributeStride;
    for (uint32_t j = 0; j < pData->activeItem.pGrid->pBuffer->attributes.count; ++j)
    {
      vdkAttributeDescriptor &oldAttrDesc = pData->activeItem.pGrid->pBuffer->attributes.pDescriptors[j];
      uint32_t attributeSize = (oldAttrDesc.typeInfo & (vdkAttributeTypeInfo_SizeMask << vdkAttributeTypeInfo_SizeShift));

      // Get attribute old offset and pointer
      uint32_t attrOldOffset = 0;
      if (vdkAttributeSet_GetOffsetOfNamedAttribute(&pData->activeItem.pGrid->pBuffer->attributes, oldAttrDesc.name, &attrOldOffset) != vE_Success)
        continue;

      void *pOldAttr = udAddBytes(pData->activeItem.pGrid->pBuffer->pAttributes, i * pData->activeItem.pGrid->pBuffer->attributeStride + attrOldOffset);

      // Get attribute new offset and pointer
      uint32_t attrNewOffset = 0;
      if (vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, oldAttrDesc.name, &attrNewOffset) != vE_Success)
        continue;

      void *pNewAttr = udAddBytes(pBuffer->pAttributes, pointAttrOffset + attrNewOffset);

      // Copy attribute data
      memcpy(pNewAttr, pOldAttr, attributeSize);
    }

    // Displacement
    float *pDisplacement = (float*)udAddBytes(pBuffer->pAttributes, pointAttrOffset + displacementOffset);
    *pDisplacement = (float)distance;

    for (int elementIndex = 0; elementIndex < 3; ++elementIndex) //X,Y,Z
    {
      float *pDisplacementDistance = (float*)udAddBytes(pBuffer->pAttributes, pointAttrOffset + displacementDistanceOffset[elementIndex]);
      if (distance != FLT_MAX)
        *pDisplacementDistance = (float)((trianglePoint[elementIndex] - position[elementIndex]) / distance);
      else
        *pDisplacementDistance = 0.0;
    }

    ++pBuffer->pointCount;
  }

  if (getNextGrid)
  {
    pData->activeItem.pGrid->Deinit();
    pData->activeItem.oldGrid.Deinit();
    pData->activeItem.pGrid = nullptr;
    pData->activeItem.pointIndex = 0;

    do
    {
      if (udSafeDeque_PopFront(pData->pConvertItemData, &pData->activeItem) == udR_Success)
        break;
      udSleep(100);
    } while (pData->running);
  }

epilogue:
  return vE_Success;
}

void vcBPA_ConvertClose(vdkConvertCustomItem *pConvertInput)
{
  vcBPAConvertItem *pBPA = (vcBPAConvertItem *)pConvertInput->pData;
  pBPA->running = false;
  udThread_Join(pBPA->pThread);
  udThread_Destroy(&pBPA->pThread);
  pBPA->activeItem.oldGrid.Deinit();
  pBPA->activeItem = {};

  vcBPAConvertItemData itemData;
  while (udSafeDeque_PopFront(pBPA->pQueueItems, &itemData) == udR_Success)
    itemData.oldGrid.Deinit();
  udSafeDeque_Destroy(&pBPA->pQueueItems);

  while (udSafeDeque_PopFront(pBPA->pConvertItemData, &itemData) == udR_Success)
    itemData.oldGrid.Deinit();
  udSafeDeque_Destroy(&pBPA->pConvertItemData);

  vcBPA_Deinit(&pBPA->pManifold);
}

void vcBPA_ConvertDestroy(vdkConvertCustomItem *pConvertInput)
{
  vcBPAConvertItem *pBPA = (vcBPAConvertItem*)pConvertInput->pData;
  vdkPointCloud_Unload(&pBPA->pOldModel);
  vdkPointCloud_Unload(&pBPA->pNewModel);
  vdkAttributeSet_Free(&pConvertInput->attributes);
  udFree(pConvertInput->pData);
}

void vcBPA_CompareExport(vcState *pProgramState, const char *pOldModelPath, const char *pNewModelPath, double ballRadius, double gridSize, const char *pName)
{
  vcConvertItem *pConvertItem = nullptr;
  vcConvert_AddEmptyJob(pProgramState, &pConvertItem);

  udLockMutex(pConvertItem->pMutex);

  vdkPointCloudHeader header = {};

  vcBPAConvertItem *pBPA = udAllocType(vcBPAConvertItem, 1, udAF_Zero);
  pBPA->pContext = pProgramState->pVDKContext;
  vdkPointCloud_Load(pBPA->pContext, &pBPA->pOldModel, pOldModelPath, nullptr);
  vdkPointCloud_Load(pBPA->pContext, &pBPA->pNewModel, pNewModelPath, &header);
  pBPA->pConvertItem = pConvertItem;
  pBPA->running = true;
  pBPA->ballRadius = ballRadius;
  pBPA->gridSize = gridSize; // metres

  udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
  udDouble3 boundingBoxCenter = udDouble3::create(header.boundingBoxCenter[0], header.boundingBoxCenter[1], header.boundingBoxCenter[2]);
  udDouble3 boundingBoxExtents = udDouble3::create(header.boundingBoxExtents[0], header.boundingBoxExtents[1], header.boundingBoxExtents[2]);

  const char *pMetadata = nullptr;
  vdkPointCloud_GetMetadata(pBPA->pNewModel, &pMetadata);
  udJSON metadata = {};
  metadata.Parse(pMetadata);

  for (uint32_t i = 0; i < metadata.MemberCount() ; ++i)
  {
    const udJSON *pElement = metadata.GetMember(i);
    // Removed error checking because convertInfo metadata triggers vE_NotSupported
    vdkConvert_SetMetadata(pConvertItem->pConvertContext, metadata.GetMemberName(i), pElement->AsString());
  }

  vdkConvertCustomItem item = {};
  item.pName = pName;

  uint32_t displacementOffset = 0;
  bool addDisplacement = true;

  uint32_t displacementDirectionOffset = 0;
  bool addDisplacementDirection = true;

  if (vdkAttributeSet_GetOffsetOfNamedAttribute(&header.attributes, "udDisplacement", &displacementOffset) == vE_Success)
    addDisplacement = false;

  if (vdkAttributeSet_GetOffsetOfNamedAttribute(&header.attributes, "udDisplacementDirectionX", &displacementDirectionOffset) == vE_Success)
    addDisplacementDirection = false;

  vdkAttributeSet_Generate(&item.attributes, vdkSAC_None, header.attributes.count + (addDisplacement ? 1 : 0) + (addDisplacementDirection ? 3 : 0));

  for (uint32_t i = 0; i < header.attributes.count; ++i)
  {
    item.attributes.pDescriptors[i].blendMode = header.attributes.pDescriptors[i].blendMode;
    item.attributes.pDescriptors[i].typeInfo = header.attributes.pDescriptors[i].typeInfo;
    udStrcpy(item.attributes.pDescriptors[i].name, header.attributes.pDescriptors[i].name);
    ++item.attributes.count;
  }

  if (addDisplacement)
  {
    item.attributes.pDescriptors[item.attributes.count].blendMode = vdkABM_SingleValue;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = vdkAttributeTypeInfo_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacement");
    ++item.attributes.count;
  }

  if (addDisplacementDirection)
  {
    item.attributes.pDescriptors[item.attributes.count].blendMode = vdkABM_SingleValue;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = vdkAttributeTypeInfo_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacementDirectionX");
    ++item.attributes.count;

    item.attributes.pDescriptors[item.attributes.count].blendMode = vdkABM_SingleValue;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = vdkAttributeTypeInfo_float32;
    udStrcpy(item.attributes.pDescriptors[item.attributes.count].name, "udDisplacementDirectionY");
    ++item.attributes.count;

    item.attributes.pDescriptors[item.attributes.count].blendMode = vdkABM_SingleValue;
    item.attributes.pDescriptors[item.attributes.count].typeInfo = vdkAttributeTypeInfo_float32;
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
  vdkConvert_AddCustomItem(pConvertItem->pConvertContext, &item);

  udFree(item.pName);
  udReleaseMutex(pBPA->pConvertItem->pMutex);
}

#endif //VC_HASCONVERT
