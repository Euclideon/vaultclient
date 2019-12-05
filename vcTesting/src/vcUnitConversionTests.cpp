#include "vcUnitConversion.h"
#include "vcTesting.h"

TEST(UnitConversion, Distance)
{
  EXPECT_NEAR(53.995680, vcUnitConversion_ConvertDistance(100.0, vcDistance_Kilometres, vcDistance_NauticalMiles), 0.00001);
  EXPECT_NEAR(10000.0, vcUnitConversion_ConvertDistance(100.0, vcDistance_Metres, vcDistance_Centimetres), 0.000001);
  EXPECT_NEAR(304.800610, vcUnitConversion_ConvertDistance(1.0, vcDistance_USSurveyFeet, vcDistance_Millimetres), 0.00001);
  EXPECT_NEAR(1.578283, vcUnitConversion_ConvertDistance(100000.0, vcDistance_USSurveyInches, vcDistance_USSurveyMiles), 0.00001);
  EXPECT_NEAR(1.27, vcUnitConversion_ConvertDistance(0.5, vcDistance_USSurveyInches, vcDistance_Centimetres), 0.00001);
  EXPECT_NEAR(1, vcUnitConversion_ConvertDistance(100000, vcDistance_Centimetres, vcDistance_Kilometres), 0.00001);

  // Round trips
  for (int i = 0; i < vcDistance_Count; ++i)
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertDistance(vcUnitConversion_ConvertDistance(10000.0, vcDistance_Metres, (vcDistanceUnit)i), (vcDistanceUnit)i, vcDistance_Metres));
}

TEST(UnitConversion, Area)
{
  EXPECT_DOUBLE_EQ(0.0001, vcUnitConversion_ConvertArea(100.0, vcArea_SquareMetres, vcArea_SquareKilometers));
  EXPECT_DOUBLE_EQ(0.01, vcUnitConversion_ConvertArea(100.0, vcArea_SquareMetres, vcArea_Hectare));
  EXPECT_DOUBLE_EQ(1076.391041670972072097356431186199188232421875, vcUnitConversion_ConvertArea(100.0, vcArea_SquareMetres, vcArea_SquareFoot));
  EXPECT_DOUBLE_EQ(0.0000386102160083283757, vcUnitConversion_ConvertArea(100.0, vcArea_SquareMetres, vcArea_SquareMiles));
  EXPECT_DOUBLE_EQ(0.0247105382834933301472446487423, vcUnitConversion_ConvertArea(100.0, vcArea_SquareMetres, vcArea_Acre));
}

TEST(UnitConversion, Volume)
{
  EXPECT_DOUBLE_EQ(1056.6882607957347, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_USSQuart));
  EXPECT_DOUBLE_EQ(264.17203728418462, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_USSGallons));
  EXPECT_DOUBLE_EQ(9702775.311062432825565, vcUnitConversion_ConvertVolume(159, vcVolume_CubicMeter, vcVolume_CubicInch));
  EXPECT_DOUBLE_EQ(1, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_CubicMeter));
  EXPECT_DOUBLE_EQ(0.001, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_MegaLiter));
  EXPECT_DOUBLE_EQ(0.00260554317599999983, vcUnitConversion_ConvertVolume(159, vcVolume_CubicInch, vcVolume_CubicMeter));
  EXPECT_DOUBLE_EQ(1.30795061586555, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_CubicYard));
  EXPECT_DOUBLE_EQ(35.31466621266132221990, vcUnitConversion_ConvertVolume(1, vcVolume_CubicMeter, vcVolume_CubicFoot));
  EXPECT_DOUBLE_EQ(0.764554859999999995, vcUnitConversion_ConvertVolume(1,vcVolume_CubicYard, vcVolume_CubicMeter));
}

TEST(UnitConversion, Speed)
{
  EXPECT_DOUBLE_EQ(360.0, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_KilometresPerHour));
  EXPECT_DOUBLE_EQ(223.69362920544023154, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_USSurveyMilesPerHour));
  EXPECT_DOUBLE_EQ(194.38444924406047675, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_NauticalMilesPerHour));
  EXPECT_DOUBLE_EQ(328.08398950131231686100363, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_FeetPerSecond));
  EXPECT_DOUBLE_EQ(340.29, vcUnitConversion_ConvertSpeed(1.0, vcSpeed_Mach, vcSpeed_MetresPerSecond));
}

TEST(UnitConversion, Temperature)
{
  EXPECT_DOUBLE_EQ(212, vcUnitConversion_ConvertTemperature(100.0, vcTemperature_Celcius, vcTemperature_Farenheit));
  EXPECT_DOUBLE_EQ(373.15, vcUnitConversion_ConvertTemperature(100.0, vcTemperature_Celcius, vcTemperature_Kelvin));
}
