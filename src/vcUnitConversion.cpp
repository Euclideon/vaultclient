#include "vcUnitConversion.h"

double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double metreTable[vcDistance_Count] = { 1.0, 1000.0, 0.01, 0.001, 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0, 1.0 / 39.37, 1852.0 };

  return sourceValue * metreTable[sourceUnit] / metreTable[requiredUnit];
}

double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit) 
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double sqmTable[vcArea_Count] = { 1.0, 1000000.0, 1000.0, 1200.0 / 3937.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0 * 1.0 / 640.0 };

  return sourceValue * sqmTable[sourceUnit] / sqmTable[requiredUnit];
}
double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double m3Table[vcVolume_Count] = { 1.0, 1000.0, 1.0 / 1000.0, 1.0 / 61023.744094732297526, 1 / 35.31466621266132221, 1 / 264.17203728418462560, 1 / 1056.6882607957347, 1 / 1.30795061586555094734 };

  return sourceValue * m3Table[sourceUnit] / m3Table[requiredUnit];
}

double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double mpsTable[vcSpeed_Count] = { 1.0, 1.0 / 3.6, 1.0 / 2.23693629205440203122634, 1.0 / 3.2808398950131225646487109, 1.0 / 1.9438444924406046432352, 340.29 };

  return sourceValue * mpsTable[sourceUnit] / mpsTable[requiredUnit];
}

double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  double scalar = 1.0;

  //Source Unit 
  if (sourceUnit == vcTemperature_Celcius)
    scalar *= 1.0;
  else if (sourceUnit == vcTemperature_Kelvin)
    scalar *= sourceValue - 273.15;
  else if (sourceUnit == vcTemperature_Farenheit)
    scalar = (9.0 / 5.0) * sourceValue + 32;

  //Required Unit
  if (requiredUnit == vcTemperature_Celcius)
    scalar *= 1.0;
  else if (requiredUnit == vcTemperature_Kelvin)
    scalar *= sourceValue + 273.15;
  else if (requiredUnit == vcTemperature_Farenheit)
    scalar *= (9.0 / 5.0) * sourceValue + 32;
  return scalar;
}
