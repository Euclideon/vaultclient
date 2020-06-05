#include "vcTesting.h"
#include "vcGIS.h"

TEST(vcGIS, LocalAxis)
{
  udGeoZone lZone = {};
  udGeoZone rZone = {};

  udGeoZone_SetFromSRID(&lZone, 4978); // ECEF

  EXPECT_TRUE(vcGIS_ChangeSpace(&rZone, lZone));

  EXPECT_TRUE(memcmp(&lZone, &rZone, sizeof(udGeoZone)) == 0);

  {
    udDouble3 northPole = udGeoZone_LatLongToCartesian(rZone, { 90.0, 0.0, 0.0 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, 1), vcGIS_GetWorldLocalUp(rZone, northPole)));
  }

  {
    udDouble3 southPole = udGeoZone_LatLongToCartesian(rZone, { -90.0, 0.0, 0.0 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), vcGIS_GetWorldLocalUp(rZone, southPole)));
  }

  {
    udDouble3 primeMeridian = udGeoZone_LatLongToCartesian(rZone, { 0.0, 0.0, 0.0 });
    udDouble3 up = vcGIS_GetWorldLocalUp(rZone, primeMeridian);

    EXPECT_TRUE(udEqualApprox(udDouble3::create(1, 0, 0), up));

    const double rt2o2 = udSqrt(2.0) / 2.0;

    // Facing North
    udDouble2 headingPitch = udDouble2::zero();
    udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(rZone, primeMeridian, headingPitch);
    udDouble2 calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(rZone, primeMeridian, orientation);
    udDouble3 localForward = orientation.apply({ 0, 1, 0 });
    udDouble3 localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, 1), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y;

    // Facing south
    headingPitch = { -UD_PI, 0 }; // This is negative because the boundary is slightly more stable on the negative side
    orientation = vcGIS_HeadingPitchToQuaternion(rZone, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(rZone, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y;

    // Facing East
    headingPitch = { UD_HALF_PI, 0 };
    orientation = vcGIS_HeadingPitchToQuaternion(rZone, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(rZone, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(up, localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 1, 0), localForward));
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y << " / EXPECTED: " << headingPitch.x << ", " << headingPitch.y;

    // Facing West & 45° down
    headingPitch = { -UD_HALF_PI, -UD_PI / 4.0 };
    orientation = vcGIS_HeadingPitchToQuaternion(rZone, primeMeridian, headingPitch);
    calcHeadingPitch = vcGIS_QuaternionToHeadingPitch(rZone, primeMeridian, orientation);
    localForward = orientation.apply({ 0, 1, 0 });
    localUp = orientation.apply({ 0, 0, 1 });
    EXPECT_TRUE(udEqualApprox(udDouble3::create( rt2o2,-rt2o2, 0), localUp)) << localUp.x << ", " << localUp.y << ", " << localUp.z;
    EXPECT_TRUE(udEqualApprox(udDouble3::create(-rt2o2,-rt2o2, 0), localForward)) << localForward.x << ", " << localForward.y << ", " << localForward.z;
    EXPECT_TRUE(udEqualApprox(headingPitch, calcHeadingPitch)) << calcHeadingPitch.x << ", " << calcHeadingPitch.y << " / EXPECTED: " << headingPitch.x << ", " << headingPitch.y;
  }

  {
    udDouble3 antiMeridian = udGeoZone_LatLongToCartesian(rZone, { 180.0, 0.0, 0.0 });
    udDouble3 up = vcGIS_GetWorldLocalUp(rZone, antiMeridian);

    EXPECT_TRUE(udEqualApprox(udDouble3::create(-1, 0, 0), up));
    udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(rZone, antiMeridian, udDouble2::zero());

    EXPECT_TRUE(udEqualApprox(up, orientation.apply({ 0, 0, 1 })));
  }

  {
    udDouble2 headingNorth = vcGIS_GetHeadingPitchFromLatLong(rZone, udDouble3::create(152.96376, -27.206142, 0), udDouble3::create(152.96376, -27.206142 + 0.0005, 0));
    EXPECT_TRUE(udEqualApprox(headingNorth, udDouble2::create(UD_DEG2RAD(0.0), 0.0)));
    udDouble2 headingEast = vcGIS_GetHeadingPitchFromLatLong(rZone, udDouble3::create(152.96376, -27.206142, 0), udDouble3::create(152.96376 + 0.0005, -27.206142, 0));
    EXPECT_TRUE(udEqualApprox(headingEast, udDouble2::create(UD_DEG2RAD(90.0), 0.0)));
    udDouble2 headingSouth = vcGIS_GetHeadingPitchFromLatLong(rZone, udDouble3::create(152.96376, -27.206142, 0), udDouble3::create(152.96376, -27.206142 - 0.0005, 0));
    EXPECT_TRUE(udEqualApprox(headingSouth, udDouble2::create(UD_DEG2RAD(180.0), 0.0)));
    udDouble2 headingWest = vcGIS_GetHeadingPitchFromLatLong(rZone, udDouble3::create(152.96376, -27.206142, 0), udDouble3::create(152.96376 - 0.0005, -27.206142, 0));
    EXPECT_TRUE(udEqualApprox(headingWest, udDouble2::create(UD_DEG2RAD(-90.0), 0.0)));

    udDouble3 kansasCityLatLong = udDouble3::create(-94.581213, 39.099912, 0.0);
    udDouble3 stLouisLatLong = udDouble3::create(-90.200203, 38.627089, 0.0);
    udDouble2 headingPitchKanasStLouis = vcGIS_GetHeadingPitchFromLatLong(rZone, kansasCityLatLong, stLouisLatLong);
    EXPECT_TRUE(udEqualApprox(headingPitchKanasStLouis, udDouble2::create(UD_DEG2RAD(96.51), 0.0)));
  }

  {
    udDouble3 localPosition = udGeoZone_LatLongToCartesian(lZone, { 0.0, 0.0, 6371000.0 });
    udDoubleQuat q = vcGIS_GetQuaternion(lZone, localPosition);
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), q.apply({ 1, 0, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 1, 0), q.apply({ 0, 1, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(1, 0, 0), q.apply({ 0, 0, 1 })));

    localPosition = udGeoZone_LatLongToCartesian(lZone, { 0.0, 90.0, 6371000.0 });
    q = vcGIS_GetQuaternion(lZone, localPosition);
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), q.apply({ 1, 0, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(-1, 0, 0), q.apply({ 0, 1, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 1, 0), q.apply({ 0, 0, 1 })));

    localPosition = udGeoZone_LatLongToCartesian(lZone, { 0.0, 180.0, 6371000.0 });
    q = vcGIS_GetQuaternion(lZone, localPosition);
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), q.apply({ 1, 0, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, -1, 0), q.apply({ 0, 1, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(-1, 0, 0), q.apply({ 0, 0, 1 })));

    localPosition = udGeoZone_LatLongToCartesian(lZone, { 0.0, -90.0, 6371000.0 });
    q = vcGIS_GetQuaternion(lZone, localPosition);
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, 0, -1), q.apply({ 1, 0, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(1, 0, 0), q.apply({ 0, 1, 0 })));
    EXPECT_TRUE(udEqualApprox(udDouble3::create(0, -1, 0), q.apply({ 0, 0, 1 })));
  }
}
