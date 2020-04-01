#include "vcTesting.h"
#include "vcGIS.h"

TEST(vcGIS, LocalAxis)
{
  vcGISSpace space = {};
  udGeoZone zone = {};

  udGeoZone_SetFromSRID(&zone, 4978); // ECEF

  EXPECT_TRUE(vcGIS_ChangeSpace(&space, zone));

  {
    udDouble3 northPole = udGeoZone_LatLongToCartesian(zone, { 90.0, 0.0, 0.0 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, 1), vcGIS_GetWorldLocalUp(space, northPole)));
  }

  {
    udDouble3 southPole = udGeoZone_LatLongToCartesian(zone, { -90.0, 0.0, 0.0 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), vcGIS_GetWorldLocalUp(space, southPole)));
  }

  {
    udDouble3 primeMeridian = udGeoZone_LatLongToCartesian(zone, { 0.0, 0.0, 0.0 });
    udDouble3 up = vcGIS_GetWorldLocalUp(space, primeMeridian);

    EXPECT_TRUE(udEqualApprox(udDouble3::create(1, 0, 0), up));

    const double rt2o2 = udSqrt(2.0) / 2.0;

    // Facing North
    udDouble2 headingPitch = udDouble2::zero();
    udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(space, primeMeridian, headingPitch);
    udDouble2 calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(space, primeMeridian, orientation);
    udDouble3 localForward = orientation.apply({ 0, 1, 0 });
    udDouble3 localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, 1), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y;

    // Facing south
    headingPitch = { -UD_PI, 0 }; // This is negative because the boundary is slightly more stable on the negative side
    orientation = vcGIS_HeadingPitchToQuaternion(space, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(space, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y;

    // Facing East
    headingPitch = { UD_HALF_PI, 0 };
    orientation = vcGIS_HeadingPitchToQuaternion(space, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(space, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 1, 0), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y << " / EXPECTED: " << headingPitch.x << ", " << headingPitch.y;

    // Facing West & 45° down
    headingPitch = { -UD_HALF_PI, -UD_PI / 4.0 };
    orientation = vcGIS_HeadingPitchToQuaternion(space, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(space, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create( rt2o2,-rt2o2, 0), localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(-rt2o2,-rt2o2, 0), localForward)) << localForward.x << ", " << localForward.y << ", " << localForward.z;
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y << " / EXPECTED: " << headingPitch.x << ", " << headingPitch.y;
  }

  {
    udDouble3 antiMeridian = udGeoZone_LatLongToCartesian(zone, { 180.0, 0.0, 0.0 });
    udDouble3 up = vcGIS_GetWorldLocalUp(space, antiMeridian);

    EXPECT_TRUE(udEqualApprox(udDouble3::create(-1, 0, 0), up));
    udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(space, antiMeridian, udDouble2::zero());

    EXPECT_TRUE(udEqualApprox(up, orientation.apply({ 0, 0, 1 })));
  }
}
