#include "vcUnitConversion.h"

double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;
  double scalar = 1.0;
  // Metric
  if (sourceUnit == vcDistance_Metres)
    scalar = 1.0;
  else if (sourceUnit == vcDistance_Kilometres)
    scalar = 1000.0;
  else if (sourceUnit == vcDistance_Centimetres) 
    scalar = 1 / 100.0;
  else if (sourceUnit == vcDistance_Millimetres) 
    scalar = 1 / 1000.0;
  // Imperial
  else if (sourceUnit == vcDistance_USSurveyFeet) 
    scalar = 1 / 3.2808398950131225646;
  else if (sourceUnit == vcDistance_USSurveyMiles)  
    scalar = 1609.3440000000000509;
  else if (sourceUnit == vcDistance_USSurveyInches)   
    scalar = 1 / 39.370078740157481433;
  // Nautical
  else if (sourceUnit == vcDistance_NauticalMiles) 
    scalar = 1852.0;
  // Metric
  if (requiredUnit == vcDistance_Metres)
    scalar *= 1.0;
  else if (requiredUnit == vcDistance_Kilometres)   
    scalar *= 1 / 1000.0;
  else if (requiredUnit == vcDistance_Centimetres)  
    scalar *= 100.0;
  else if (requiredUnit == vcDistance_Millimetres) 
    scalar *= 1000.0;
  // Imperial_US
  else if (requiredUnit == vcDistance_USSurveyFeet) 
    scalar *= 3.2808398950131225646;
  else if (requiredUnit == vcDistance_USSurveyMiles)   
    scalar *= 1 / 1609.3440000000000509;
  else if (requiredUnit == vcDistance_USSurveyInches)
    scalar *= 39.370078740157481433;
  // Nautical
  else if (requiredUnit == vcDistance_NauticalMiles)
    scalar *= 1 / 1852.0;
  return sourceValue * scalar;
}
double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit) 
{
  if (sourceUnit == requiredUnit)
    return sourceValue;
  double scalar = 1.0;
  //Source Unit 
  if (sourceUnit == vcAreaUnit_SquareMetres)
    scalar = 1.0;
  else if (sourceUnit == vcAreaUnit_SquareFoot)
    scalar = 1 / 10.763910416709721928;
  else if (sourceUnit == vcAreaUnit_SquareKilometers)
    scalar = 1000000;
  else if (sourceUnit == vcAreaUnit_SquareMiles)
    scalar = 2589988.1000000000931;
  else if (sourceUnit == vcAreaUnit_Acre)
    scalar = 4046.8564000000001215;
  else if (sourceUnit == vcAreaUnit_Hectare)
    scalar = 1000;
  //Required Unit
  if (requiredUnit == vcAreaUnit_SquareMetres)
    scalar *= 1.0;
  else if (requiredUnit == vcAreaUnit_SquareFoot)
    scalar *= 10.763910416709721928;
  else if (requiredUnit == vcAreaUnit_SquareKilometers)
    scalar *= 1 / 1000000.0;
  else if (requiredUnit == vcAreaUnit_SquareMiles)
    scalar *= 1 / 2589988.1000000000931;
  else if (requiredUnit == vcAreaUnit_Acre)
    scalar *= 1 / 4046.8564000000001215;
  else if (requiredUnit == vcAreaUnit_Hectare)
    scalar *= 1 / 10000.0;
  return sourceValue * scalar;
}
double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;
  double scalar = 1.0;
  //Source Unit
  if (sourceUnit == vcVolumeUnit_CubicMeter)
    scalar = 1.0;
  else if (sourceUnit == vcVolumeUnit_USSGallons)
    scalar = 1 / 264.17203728418462560;
  else if (sourceUnit == vcVolumeUnit_USSQuart)
    scalar = 1 / 1056.6882607957347;
  else if (sourceUnit == vcVolumeUnit_Liter)
    scalar = 1 / 1000.0;
  else if (sourceUnit == vcVolumeUnit_CubicInch)
    scalar = 1.0 / 61023.744094732297526;
  else if (sourceUnit == vcVolumeUnit_CubicFoot)
    scalar = 1 / 35.31466621266132221;
  else if (sourceUnit == vcVolumeUnit_CubicYard)
    scalar = 1 / 1.30795061586555094734;
  else if (sourceUnit == vcVolumeUnit_MegaLiter)
    scalar = 1000.0;
  // Required Unit
  if (requiredUnit == vcVolumeUnit_CubicMeter)
    scalar *= 1.0;
  else if (requiredUnit == vcVolumeUnit_USSGallons)
    scalar *= 264.17203728418462560;
  else if (requiredUnit == vcVolumeUnit_USSQuart)
    scalar *= 1056.6882607957347;
  else if (requiredUnit == vcVolumeUnit_Liter)
    scalar *= 1000.0;
  else if (requiredUnit == vcVolumeUnit_CubicInch)
    scalar *= 61023.744094732297526;
  else if (requiredUnit == vcVolumeUnit_CubicFoot)
    scalar *= 35.31466621266132221;
  else if (requiredUnit == vcVolumeUnit_MegaLiter)
    scalar *= 1 / 1000.0;
  else if (requiredUnit == vcVolumeUnit_CubicYard)
    scalar *= 1.30795061586555094734;
  return sourceValue * scalar;
}

double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;
double scalar = 1.0;
//Source Unit
  if (sourceUnit == vcSpeed_MetresPerSecond)
    scalar = 1.0;
  else if (sourceUnit == vcSpeed_KilometresPerHour)
    scalar = 1.0 / 3.599999999999999644;
  else if (sourceUnit == vcSpeed_USSurveyMilesPerHour)
    scalar = 1.0 / 2.23693629205440203122634;
  else if (sourceUnit == vcSpeed_NauticalMilesPerHour)
    scalar = 1.0 / 1.9438444924406046432352;
  else if (sourceUnit == vcSpeed_FootPerSecound)
    scalar = 1.0 / 3.2808398950131225646487109;
  // There are more units here
  if (requiredUnit == vcSpeed_MetresPerSecond)
    scalar *= 1.0;
  else if (requiredUnit == vcSpeed_KilometresPerHour)
    scalar *= 3.599999999999999644;
  else if (requiredUnit == vcSpeed_USSurveyMilesPerHour)
    scalar *= 2.23693629205440203122634;
  else if (requiredUnit == vcSpeed_NauticalMilesPerHour)
    scalar *= 1.9438444924406046432352;
  else if (requiredUnit == vcSpeed_FootPerSecound)
    scalar *= 3.2808398950131225646;
  return sourceValue * scalar;
}

double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;
  double scalar = 1.0;
  //Source uNit 
  if (sourceUnit == vcTemperature_Celcius)
    scalar * 1.0;
  else if (sourceUnit == vcTemperature_Kelvin)
    scalar *= sourceValue - 273.15;
  else if (sourceUnit == vcTemperature_Farenheit)
    scalar = (9.0 / 5.0) * sourceValue + 32;
  //Required Unit
  if (requiredUnit == vcTemperature_Celcius)
    scalar * 1.0;
  else if (requiredUnit == vcTemperature_Kelvin)
    scalar *= sourceValue + 273.15;
  else if (requiredUnit == vcTemperature_Farenheit)
    scalar *= (9.0 / 5.0) * sourceValue + 32;
  return scalar;
}
