#include "vcUnitConversion.h"
#include "vcTesting.h"

TEST(UnitConversion, Distance)
{
  EXPECT_DOUBLE_EQ(32.0, vcUnitConversion_ConvertTemperature(0.0, vcTemperature_Celcius, vcTemperature_Farenheit));
}

TEST(UnitConversion, Area)
{
  EXPECT_DOUBLE_EQ(32.0, vcUnitConversion_ConvertTemperature(0.0, vcTemperature_Celcius, vcTemperature_Farenheit));
}

TEST(UnitConversion, Volume)
{
  EXPECT_DOUBLE_EQ(32.0, vcUnitConversion_ConvertTemperature(0.0, vcTemperature_Celcius, vcTemperature_Farenheit));
}

TEST(UnitConversion, Speed)
{
  EXPECT_DOUBLE_EQ(32.0, vcUnitConversion_ConvertTemperature(0.0, vcTemperature_Celcius, vcTemperature_Farenheit));
}

TEST(UnitConversion, Temperature)
{
  EXPECT_DOUBLE_EQ(32.0, vcUnitConversion_ConvertTemperature(0.0, vcTemperature_Celcius, vcTemperature_Farenheit));
}
