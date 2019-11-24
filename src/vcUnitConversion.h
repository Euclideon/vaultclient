#ifndef vcUnitConversion_h__
#define vcUnitConversion_h__

enum vcDistanceUnit
{
  // Metric
  vcDistance_Metres,
  vcDistance_Kilometres,
  vcDistance_Centimetres,
  vcDistance_Millimetres,

  // Imperial
  vcDistance_USSurveyFeet,
  vcDistance_USSurveyMiles,
  vcDistance_USSurveyInches,

  // Nautical
  vcDistance_NauticalMiles,

  vcDistance_Count
};

enum vcAreaUnit
{
  vcDistance_SquareMetres,

  vcDistance_Count
};

enum vcVolumeUnit
{
  vcVolume_CubicMetres,

  vcDistance_Count
};

enum vcSpeedUnit
{
  vcSpeed_MetresPerSecond,
  vcSpeed_KilometresPerHour,

  vcSpeed_USSurveyMilesPerHour,
  vcSpeed_NauticalMilesPerHour, //Knots

  vcSpeed_Count
};

enum vcTemperatureUnit
{
  vcTemperature_Celcius,
  vcTemperature_Kelvin,
  vcTemperature_Farenheit,

  vcTemperature_Count
};

double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit);
double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit);
double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit);
double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit);
double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit);

#endif //vcUnitConversion_h__
