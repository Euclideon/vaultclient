#include "vcMath.h"
#include "vcTesting.h"

#define EPSILON_DOUBLE 0.0000000001
#define TRI_DIST udDistanceToTriangle(p0, p1, p2, testPoint, &out)
#define EXPECT_VEC_NEAR(v0, v1) EXPECT_NEAR(v0.x, v1.x, EPSILON_DOUBLE);\
EXPECT_NEAR(v0.y, v1.y, EPSILON_DOUBLE);\
EXPECT_NEAR(v0.z, v1.z, EPSILON_DOUBLE)

TEST(vcMath, TriangleTest_NoOutput)
{
  udDouble3 p0{ 0.0, 1.0, 0.0 };
  udDouble3 p1{ -1.0, 0.0, 0.0 };
  udDouble3 p2{ 1.0, 0.0, 0.0 };
  udDouble3 testPoint = {};

  testPoint = udDouble3::create(0.0, 0.5, 0.0);
  EXPECT_DOUBLE_EQ(udDistanceToTriangle(p0, p1, p2, testPoint), 0.0);
}

TEST(vcMath, TriangleTest_PointOnTrianglePlane)
{
  udDouble3 p0{ 0.0, 1.0, 0.0 };
  udDouble3 p1{ -1.0, 0.0, 0.0 };
  udDouble3 p2{ 1.0, 0.0, 0.0 };
  udDouble3 out;
  udDouble3 testPoint = {};
  double rt2on2 = 0.70710678118654752440084436210485;

  //------------------------------------------------------------------------
  // Point inside triangle
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 0.5, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 0.0);
  EXPECT_VEC_NEAR(out, testPoint);

  //------------------------------------------------------------------------
  // Point outside triangle - closest point is a vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 2.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p0);

  testPoint = udDouble3::create(-2.0, 0.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p1);

  testPoint = udDouble3::create(2.0, 0.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p2);
  
  //------------------------------------------------------------------------
  // Point outside triangle - closest point is an edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(1.0, 1.0, 0.0);
  EXPECT_NEAR(TRI_DIST, rt2on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));

  testPoint = udDouble3::create(-1.0, 1.0, 0.0);
  EXPECT_NEAR(TRI_DIST, rt2on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, -1.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));


  //------------------------------------------------------------------------
  // Point on vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 1.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 0.0);
  EXPECT_VEC_NEAR(out, p0);

  testPoint = udDouble3::create(-1.0, 0.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 0.0);
  EXPECT_VEC_NEAR(out, p1);

  testPoint = udDouble3::create(1.0, 0.0, 0.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 0.0);
  EXPECT_VEC_NEAR(out, p2);

  //------------------------------------------------------------------------
  // Point on edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.5, 0.5, 0.0);
  EXPECT_NEAR(TRI_DIST, 0.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));

  testPoint = udDouble3::create(-0.5, 0.5, 0.0);
  EXPECT_NEAR(TRI_DIST, 0.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, 0.0, 0.0);
  EXPECT_NEAR(TRI_DIST, 0.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));
}

TEST(vcMath, TriangleTest_PointAboveTrianglePlane)
{
  udDouble3 p0{ 0.0, 1.0, 0.0 };
  udDouble3 p1{ -1.0, 0.0, 0.0 };
  udDouble3 p2{ 1.0, 0.0, 0.0 };
  udDouble3 out;
  udDouble3 testPoint = {};
  double rt2 = 1.4142135623730950488016887242097;
  double rt6on2 = 1.2247448713915890490986420373529;

  //------------------------------------------------------------------------
  // Point inside triangle
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 0.5, 1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.5, 0.0));

  //------------------------------------------------------------------------
  // Point outside triangle - closest point is a vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 2.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p0);

  testPoint = udDouble3::create(-2.0, 0.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p1);

  testPoint = udDouble3::create(2.0, 0.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p2);

  //------------------------------------------------------------------------
  // Point outside triangle - closest point is an edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(1.0, 1.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt6on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));

  
  testPoint = udDouble3::create(-1.0, 1.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt6on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, -1.0, 1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));


  //------------------------------------------------------------------------
  // Point on vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 1.0, 1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p0);
  
  testPoint = udDouble3::create(-1.0, 0.0, 1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p1);
  
  testPoint = udDouble3::create(1.0, 0.0, 1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p2);

  //------------------------------------------------------------------------
  // Point on edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.5, 0.5, 1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));

  testPoint = udDouble3::create(-0.5, 0.5, 1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, 0.0, 1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));
}

TEST(vcMath, TriangleTest_PointBelowTrianglePlane)
{
  udDouble3 p0{ 0.0, 1.0, 0.0 };
  udDouble3 p1{ -1.0, 0.0, 0.0 };
  udDouble3 p2{ 1.0, 0.0, 0.0 };
  udDouble3 out;
  udDouble3 testPoint = {};
  double rt2 = 1.4142135623730950488016887242097;
  double rt6on2 = 1.2247448713915890490986420373529;

  //------------------------------------------------------------------------
  // Point inside triangle
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 0.5, -1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.5, 0.0));

  //------------------------------------------------------------------------
  // Point outside triangle - closest point is a vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 2.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p0);

  testPoint = udDouble3::create(-2.0, 0.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p1);

  testPoint = udDouble3::create(2.0, 0.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, p2);

  //------------------------------------------------------------------------
  // Point outside triangle - closest point is an edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(1.0, 1.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt6on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));


  testPoint = udDouble3::create(-1.0, 1.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt6on2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, -1.0, -1.0);
  EXPECT_NEAR(TRI_DIST, rt2, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));


  //------------------------------------------------------------------------
  // Point on vertex
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.0, 1.0, -1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p0);

  testPoint = udDouble3::create(-1.0, 0.0, -1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p1);

  testPoint = udDouble3::create(1.0, 0.0, -1.0);
  EXPECT_DOUBLE_EQ(TRI_DIST, 1.0);
  EXPECT_VEC_NEAR(out, p2);

  //------------------------------------------------------------------------
  // Point on edge
  //------------------------------------------------------------------------

  testPoint = udDouble3::create(0.5, 0.5, -1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.5, 0.5, 0.0));

  testPoint = udDouble3::create(-0.5, 0.5, -1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(-0.5, 0.5, 0.0));

  testPoint = udDouble3::create(0.0, 0.0, -1.0);
  EXPECT_NEAR(TRI_DIST, 1.0, EPSILON_DOUBLE);
  EXPECT_VEC_NEAR(out, udDouble3::create(0.0, 0.0, 0.0));
}

TEST(vcMath, LineSegment_General)
{
  udDouble3 p0 = udDouble3::create(-1.0, 1.0, 0.0);
  udDouble3 p1 = udDouble3::create(1.0, 1.0, 0.0);
  udLineSegment<double> seg(p0, p1);

  EXPECT_EQ(seg.origin(), p0);
  EXPECT_EQ(seg.direction(), p1 - p0);
  EXPECT_EQ(seg.P0(), p0);
  EXPECT_EQ(seg.P1(), p1);

  EXPECT_EQ(seg.length(), 2.0);
  EXPECT_EQ(seg.lengthSquared(), 4.0);
}

TEST(vcMath, LineSegmentPointQuery)
{
  udDouble3 p0 = udDouble3::create(-1.0, 1.0, 0.0);
  udDouble3 p1 = udDouble3::create(1.0, 1.0, 0.0);
  udLineSegment<double> seg(p0, p1);

  double u;
  udDouble3 point;

  //Point on the segment
  point = udDouble3::create(-1.0, 1.0, 0.0);
  EXPECT_EQ(0.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(0.0, u);

  point = udDouble3::create(1.0, 1.0, 0.0);
  EXPECT_EQ(0.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(1.0, u);

  point = udDouble3::create(0.0, 1.0, 0.0);
  EXPECT_EQ(0.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(0.5, u);

  //Point on line, not on segment
  point = udDouble3::create(-9.0, 1.0, 0.0);
  EXPECT_EQ(64.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(0.0, u);

  point = udDouble3::create(10.0, 1.0, 0.0);
  EXPECT_EQ(81.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(1.0, u);

  //Point of line and segment
  point = udDouble3::create(-2.0, 1.0, 1.0);
  EXPECT_EQ(2.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(0.0, u);

  point = udDouble3::create(0.0, 2.0, 1.0);
  EXPECT_EQ(2.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(0.5, u);

  point = udDouble3::create(2.0, 1.0, 1.0);
  EXPECT_EQ(2.0, udDistanceSqLineSegmentPoint(seg, point, &u));
  EXPECT_EQ(1.0, u);
}

TEST(vcMath, PolygonArea)
{
  udDouble2 points[4]; 
  points[0] = udDouble2::create(1.0, 1.0);
  points[1] = udDouble2::create(4.0, 1.0);
  points[2] = udDouble2::create(4.0, 4.0);
  points[3] = udDouble2::create(1.0, 4.0);

  EXPECT_EQ(udSignedSimplePolygonArea2(points, 4), 9.0);

  points[0] = udDouble2::create(1.0, 1.0);
  points[1] = udDouble2::create(1.0, 5.0);
  points[2] = udDouble2::create(5.0, 5.0);
  points[3] = udDouble2::create(5.0, 1.0);

  EXPECT_EQ(udSignedSimplePolygonArea2(points, 4), -16.0);
}
