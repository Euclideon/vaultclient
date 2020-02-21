#include "vcMath.h"
#include "vcTesting.h"

#define EPSILON_DOUBLE 0.0000000001
#define TRI_DIST udDistanceToTriangle(p0, p1, p2, testPoint, &out)
#define EXPECT_VEC_NEAR(v0, v1) EXPECT_NEAR(v0.x, v1.x, EPSILON_DOUBLE);\
EXPECT_NEAR(v0.y, v1.y, EPSILON_DOUBLE);\
EXPECT_NEAR(v0.z, v1.z, EPSILON_DOUBLE)

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
