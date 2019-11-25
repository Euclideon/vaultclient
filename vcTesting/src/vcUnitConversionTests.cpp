#include "vcUnitConversion.h"
#include "vcTesting.h"

TEST(UnitConversion, Distance)
{
  EXPECT_DOUBLE_EQ(53.99568034557236, vcUnitConversion_ConvertDistance(100, vcDistance_Kilometres,vcDistance_NauticalMiles));
  EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertDistance(100, vcDistance_Metres, vcDistance_Centimetres));
  EXPECT_DOUBLE_EQ(0.328083989501312345282713067717850208282470703125, vcUnitConversion_ConvertDistance(100, vcDistance_Millimetres, vcDistance_USSurveyFeet));
  EXPECT_DOUBLE_EQ(1578282.828282828, vcUnitConversion_ConvertDistance(100000000000, vcDistance_USSurveyInches, vcDistance_USSurveyMiles));
  EXPECT_DOUBLE_EQ(1.3714902807775378, vcUnitConversion_ConvertDistance(100000, vcDistance_USSurveyInches, vcDistance_NauticalMiles));
  EXPECT_DOUBLE_EQ(72913.385826771657, vcUnitConversion_ConvertDistance(1, vcDistance_NauticalMiles, vcDistance_USSurveyInches));
  EXPECT_DOUBLE_EQ(1, vcUnitConversion_ConvertDistance(100000, vcDistance_Centimetres, vcDistance_Kilometres));


}
TEST(UnitConversion, Area)
{
  EXPECT_DOUBLE_EQ(0.0001, vcUnitConversion_ConvertArea(100.0, vcAreaUnit_SquareMetres, vcAreaUnit_SquareKilometers));

  EXPECT_DOUBLE_EQ(0.01, vcUnitConversion_ConvertArea(100.0, vcAreaUnit_SquareMetres, vcAreaUnit_Hectare));

  EXPECT_DOUBLE_EQ(1076.391041670972072097356431186199188232421875, vcUnitConversion_ConvertArea(100.0, vcAreaUnit_SquareMetres, vcAreaUnit_SquareFoot));

  EXPECT_DOUBLE_EQ(0.0000386102160083283757, vcUnitConversion_ConvertArea(100.0, vcAreaUnit_SquareMetres, vcAreaUnit_SquareMiles));

  EXPECT_DOUBLE_EQ(0.0247105382834933301472446487423, vcUnitConversion_ConvertArea(100.0, vcAreaUnit_SquareMetres, vcAreaUnit_Acre));


}



TEST(UnitConversion, Volume)
{
  EXPECT_DOUBLE_EQ(1056.6882607957347, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_USSQuart));
  EXPECT_DOUBLE_EQ(264.17203728418462, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_USSGallons));
  EXPECT_DOUBLE_EQ(9702775.311062432825565, vcUnitConversion_ConvertVolume(159, vcVolumeUnit_CubicMeter, vcVolumeUnit_CubicInch));
  EXPECT_DOUBLE_EQ(1, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_CubicMeter));
  EXPECT_DOUBLE_EQ(0.001, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_MegaLiter));

  EXPECT_DOUBLE_EQ(0.00260554317599999983, vcUnitConversion_ConvertVolume(159, vcVolumeUnit_CubicInch, vcVolumeUnit_CubicMeter));

  EXPECT_DOUBLE_EQ(1.30795061586555, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_CubicYard));
  EXPECT_DOUBLE_EQ(35.31466621266132221990, vcUnitConversion_ConvertVolume(1, vcVolumeUnit_CubicMeter, vcVolumeUnit_CubicFoot));

  EXPECT_DOUBLE_EQ(0.764554859999999995, vcUnitConversion_ConvertVolume(1,vcVolumeUnit_CubicYard,vcVolumeUnit_CubicMeter));


}

TEST(UnitConversion, Speed)

  {
    EXPECT_DOUBLE_EQ(360.000000000000000000000, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_KilometresPerHour));
    EXPECT_DOUBLE_EQ(223.69362920544023154, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_USSurveyMilesPerHour));
    EXPECT_DOUBLE_EQ(194.38444924406047675, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_NauticalMilesPerHour));
    EXPECT_DOUBLE_EQ(328.08398950131231686100363, vcUnitConversion_ConvertSpeed(100, vcSpeed_MetresPerSecond, vcSpeed_FootPerSecound));
  }

TEST(UnitConversion, Temperature)
{
  EXPECT_DOUBLE_EQ(212, vcUnitConversion_ConvertTemperature(100.0, vcTemperature_Celcius, vcTemperature_Farenheit));
  EXPECT_DOUBLE_EQ(373.15, vcUnitConversion_ConvertTemperature(100.0, vcTemperature_Celcius, vcTemperature_Kelvin));


}
