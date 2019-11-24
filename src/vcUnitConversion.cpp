#include "vcUnitConversion.h"

double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit)
{
  return sourceValue;
}

double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit) 
{
  return sourceValue;
}

double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit)
{
  return sourceValue;
}

double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit)
{
  double scalar = 1.0;

  if (sourceUnit == vcSpeed_MetresPerSecond)
    scalar = 1.0;
  else if (sourceUnit == vcSpeed_KilometresPerHour)
    scalar = 1.0 / 3.6;

  // There are more units here

  if (requiredUnit == vcSpeed_MetresPerSecond)
    scalar *= 1.0;
  else if (requiredUnit == vcSpeed_KilometresPerHour)
    scalar *= 3.6;

  return sourceValue * scalar;
}

double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit)
{
  return sourceValue;
}
