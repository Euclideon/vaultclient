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

  static vcBPAGrid *Create(udDouble3 center, udDouble3 minPos, udDouble3 maxPos)
  {
    vcBPAGrid *pGrid = udAllocType(vcBPAGrid, 1, udAF_Zero);
    pGrid->center = center;
    pGrid->activeEdgeIndex = 0;
    pGrid->minPos = minPos;
    pGrid->maxPos = maxPos;
    return pGrid;
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

struct vcBPAManifold
{
  udChunkedArray<vcBPAGrid*> grids;
  udWorkerPool *pPool;

  bool foundFirstGrid;
  double gridSize;
  double ballRadius;
  vdkContext *pContext;
};

void vcBPA_Init(vcBPAManifold **ppManifold, vdkContext *pContext)
{
  vcBPAManifold *pManifold = udAllocType(vcBPAManifold, 1, udAF_Zero);
  pManifold->grids.Init(1024);
  udWorkerPool_Create(&pManifold->pPool, 8, "vcBPAPool");
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
  for (size_t i = 0; i < pManifold->grids.length; ++i)
    pManifold->grids[i]->Deinit();

  pManifold->grids.Deinit();
  udFree(pManifold);
}

void vcBPA_AddGrid(vcBPAManifold *pManifold, vdkPointCloud *pModel, udDouble3 center)
{
  if (!pManifold->foundFirstGrid)
  {
    vdkPointCloudHeader header = {};
    if (vdkPointCloud_GetHeader(pModel, &header) != vE_Success)
      return;

    udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
    udDouble3 boundingBoxCenter = udDouble3::create(header.boundingBoxCenter[0], header.boundingBoxCenter[1], header.boundingBoxCenter[2]);
    udDouble3 boundingBoxExtents = udDouble3::create(header.boundingBoxExtents[0], header.boundingBoxExtents[1], header.boundingBoxExtents[2]);
    udDouble3 modelMin = (storedMatrix * udDouble4::create(boundingBoxCenter - boundingBoxExtents, 1.0)).toVector3();
    udDouble3 modelMax = (storedMatrix * udDouble4::create(boundingBoxCenter + boundingBoxExtents, 1.0)).toVector3();

    if (center.x < modelMin.x || center.y < modelMin.y || center.z < modelMin.z || center.x > modelMax.x || center.y > modelMax.y || center.z > modelMax.z)
      return;
  }

  size_t i = 0;
  for (; i < pManifold->grids.length; ++i)
  {
    if (pManifold->grids[i]->center == center)
      break;
  }

  if (i == pManifold->grids.length)
  {
    udDouble3 minPos = center - udDouble3::create(pManifold->gridSize / 2.0);
    udDouble3 maxPos = center + udDouble3::create(pManifold->gridSize / 2.0);
    pManifold->grids.PushBack(vcBPAGrid::Create(center, minPos, maxPos));
  }
}

void vcBPA_PopulateGrid(vdkContext *pContext, vdkPointCloud *pModel, vdkAttributeSet *pAttributes, udDouble3 aabbCenter, udDouble3 aabbExtents, vcBPAGrid *pGrid, bool *pHasPoints, bool *pHasNeighbours)
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
    else
      *pHasNeighbours = true;

    pGrid->vertices.PushBack({ position, false });
  }
}

bool vcBPA_GetGrid(vcBPAManifold *pManifold, vdkPointCloud *pModel, vdkAttributeSet *pAttributes, vcBPAGrid **ppGrid, bool addOverlap)
{
  for (size_t i = 0; i < pManifold->grids.length; ++i)
  {
    if (pManifold->grids[i]->visited)
      continue;

    pManifold->grids[i]->visited = true;

    udDouble3 aabbCenter = pManifold->grids[i]->center;
    udDouble3 aabbExtents = udDouble3::create(pManifold->gridSize / 2.0);

    if (addOverlap)
      aabbExtents += udDouble3::create(2 * pManifold->ballRadius); // Add overlap

    bool hasPoints = false;
    bool hasNeighbours = false;
    vcBPA_PopulateGrid(pManifold->pContext, pModel, pAttributes, aabbCenter, aabbExtents, *(pManifold->grids.GetElement(i)), &hasPoints, &hasNeighbours);

    if (!hasPoints && !pManifold->foundFirstGrid)
    {
      // Head in one direction first (prioritize up/down)
      for (int j = 2; j >= 0; --j)
      {
        size_t gridCount = pManifold->grids.length;
        udDouble3 offset = udDouble3::zero();
        offset[j] = pManifold->gridSize;
        vcBPA_AddGrid(pManifold, pModel, aabbCenter + offset);
        vcBPA_AddGrid(pManifold, pModel, aabbCenter - offset);

        // If any new grids were added break
        if (gridCount != pManifold->grids.length)
          break;
      }
    }
    else if (hasNeighbours || (!addOverlap && hasPoints))
    {
      // TODO: Consider calculating which neighbours to add
      for (int j = 0; j < 3; j++)
      {
        udDouble3 offset = udDouble3::zero();
        offset[j] = pManifold->gridSize;
        vcBPA_AddGrid(pManifold, pModel, aabbCenter + offset);
        vcBPA_AddGrid(pManifold, pModel, aabbCenter - offset);
      }
    }

    if (!hasPoints)
    {
      pManifold->grids[i]->Deinit();
      continue;
    }

    if (pManifold->grids[i]->vertices.length == 0)
      continue;

    pManifold->foundFirstGrid = true;

    *ppGrid = *(pManifold->grids.GetElement(i));
    return true;
  }

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
      udDouble3 p2 = pGrid->vertices[j].position;

      if (udMagSq3(p0 - p2) > d01)
        break;
    }
    pPoints->Insert(j, &i);
  }
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
        udDouble3 a = p1 - p0;
        udDouble3 b = p2 - p0;
        udDouble3 triangleNormal = udNormalize(udCross(a, b));
        if (triangleNormal.z < 0) // Normal is pointing down, that's bad!
        {
          triangle.x = (uint64_t)nearbyPoints[i];
          triangle.y = (uint64_t)pointIndex;
          a = p0 - p1;
          b = p2 - p1;
          triangleNormal = udNormalize(udCross(a, b));
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
    udDouble3 a = vertices[1] - vertices[0];
    udDouble3 b = vertices[2] - vertices[0];
    udDouble3 triangleNormal = udNormalize(udCross(a, b));
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

double vcBPA_DistanceToTriangle(vcBPAGrid *pOldGrid, size_t triangleIndex, udDouble3 position)
{
  udDouble3 vertices[3] = {
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.x].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.y].position,
    pOldGrid->vertices[pOldGrid->triangles[triangleIndex].vertices.z].position,
  };

  return udDistanceToTriangle(vertices[0], vertices[1], vertices[2], position);
}

size_t vcBPA_FindClosestTriangle(vcBPAGrid *pOldGrid, udDouble3 position)
{
  size_t closest = 0;
  double closestDistance = FLT_MAX;

  for (size_t i = 0; i < pOldGrid->triangles.length; ++i)
  {
    double distance = udAbs(vcBPA_DistanceToTriangle(pOldGrid, i, position));
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
  vcBPAGrid *pOldGrid;

  uint32_t pointIndex;
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
  itemData.pOldGrid = vcBPAGrid::Create(itemData.pGrid->center, minPos, maxPos);

  udDouble3 aabbCenter = itemData.pOldGrid->center;
  udDouble3 aabbExtents = udDouble3::create(itemData.pManifold->gridSize / 2.0);
  aabbExtents += udDouble3::create(2 * itemData.pManifold->ballRadius); // Add overlap

  bool hasPoints = false;
  bool hasNeighbours = false;
  vcBPA_PopulateGrid(itemData.pManifold->pContext, pData->pOldModel, nullptr, aabbCenter, aabbExtents, itemData.pOldGrid, &hasPoints, &hasNeighbours);
  vcBPA_DoGrid(itemData.pOldGrid, itemData.pManifold->ballRadius);

  udSafeDeque_PushBack(pData->pConvertItemData, itemData);
}

uint32_t vcBPA_GridGeneratorThread(void *pDataPtr)
{
  vcBPAConvertItem *pData = (vcBPAConvertItem *)pDataPtr;

  vcBPAGrid *pGrid = nullptr;
  int i = 0;

  vdkPointCloudHeader header;
  vdkPointCloud_GetHeader(pData->pNewModel, &header);

  while (vcBPA_GetGrid(pData->pManifold, pData->pNewModel, &header.attributes, &pGrid, false) && pData->running)
  {
    vcBPAConvertItemData data = {};
    data.pManifold = pData->pManifold;
    data.pGrid = pGrid;
    data.pointIndex = 0;

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

  udDouble3 halfGrid = udDouble3::create(pData->pManifold->gridSize / 2.0);
  pData->pManifold->grids.PushBack(vcBPAGrid::Create(startAABBCenter, startAABBCenter - halfGrid, startAABBCenter + halfGrid));

  udSafeDeque_Create(&pData->pQueueItems, 32);
  udSafeDeque_Create(&pData->pConvertItemData, 128);

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
  vdkError error = vE_Failure;

  static int gridCount = 0;
  if (pData->activeItem.pointIndex == 0)
    ++gridCount;

  if (pData->activeItem.pointIndex == 0 && pData->activeItem.pGrid == nullptr)
    goto epilogue;

  error = vdkAttributeSet_GetOffsetOfNamedAttribute(&pBuffer->attributes, "udDisplacement", &displacementOffset);
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
    size_t triangle = vcBPA_FindClosestTriangle(pData->activeItem.pOldGrid, position);

    double distance = FLT_MAX;
    if (triangle < pData->activeItem.pOldGrid->triangles.length)
      distance = udAbs(vcBPA_DistanceToTriangle(pData->activeItem.pOldGrid, triangle, position));

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

    ++pBuffer->pointCount;
  }

  if (getNextGrid)
  {
    pData->activeItem.pGrid->Deinit();
    pData->activeItem.pOldGrid->Deinit();
    pData->activeItem.pGrid = nullptr;
    pData->activeItem.pOldGrid = nullptr;
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

  if (pBPA->activeItem.pOldGrid)
  {
    pBPA->activeItem.pOldGrid->Deinit();
    udFree(pBPA->activeItem.pOldGrid);
  }

  if (pBPA->activeItem.pGrid)
  {
    pBPA->activeItem.pGrid->Deinit();
    udFree(pBPA->activeItem.pGrid);
  }

  vcBPAConvertItemData itemData;

  while (udSafeDeque_PopFront(pBPA->pConvertItemData, &itemData) == udR_Success || udSafeDeque_PopFront(pBPA->pQueueItems, &itemData) == udR_Success)
  {
    if (itemData.pOldGrid)
    {
      itemData.pOldGrid->Deinit();
      udFree(itemData.pOldGrid);
    }

    if (itemData.pGrid)
    {
      itemData.pGrid->Deinit();
      udFree(itemData.pGrid);
    }
  }
  udSafeDeque_Destroy(&pBPA->pConvertItemData);

  vcBPA_Deinit(&pBPA->pManifold);
}

void vcBPA_ConvertDestroy(vdkConvertCustomItem *pConvertInput)
{
  vdkAttributeSet_Free(&pConvertInput->attributes);
  udFree(pConvertInput->pData);
}

void vcBPA_CompareExport(vcState *pProgramState, vdkPointCloud *pOldModel, vdkPointCloud *pNewModel, double ballRadius, const char *pName)
{
  vcConvertItem *pConvertItem = nullptr;
  vcConvert_AddEmptyJob(pProgramState, &pConvertItem);

  udLockMutex(pConvertItem->pMutex);

  vcBPAConvertItem *pBPA = udAllocType(vcBPAConvertItem, 1, udAF_Zero);
  pBPA->pContext = pProgramState->pVDKContext;
  pBPA->pOldModel = pOldModel;
  pBPA->pNewModel = pNewModel;
  pBPA->pConvertItem = pConvertItem;
  pBPA->running = true;
  pBPA->ballRadius = ballRadius;
  pBPA->gridSize = 1; // metres

  vdkPointCloudHeader header = {};
  vdkPointCloud_GetHeader(pNewModel, &header);
  udDouble4x4 storedMatrix = udDouble4x4::create(header.storedMatrix);
  udDouble3 boundingBoxCenter = udDouble3::create(header.boundingBoxCenter[0], header.boundingBoxCenter[1], header.boundingBoxCenter[2]);
  udDouble3 boundingBoxExtents = udDouble3::create(header.boundingBoxExtents[0], header.boundingBoxExtents[1], header.boundingBoxExtents[2]);

  const char *pMetadata = nullptr;
  vdkPointCloud_GetMetadata(pNewModel, &pMetadata);
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
  if (vdkAttributeSet_GetOffsetOfNamedAttribute(&header.attributes, "udDisplacement", &displacementOffset) == vE_Success)
    addDisplacement = false;

  vdkAttributeSet_Generate(&item.attributes, vdkSAC_None, header.attributes.count + (addDisplacement ? 1 : 0));

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

  item.sourceResolution = header.convertedResolution;
  item.pointCount = metadata.Get("SourcePointCount").AsInt64();
  item.pointCountIsEstimate = false;
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
