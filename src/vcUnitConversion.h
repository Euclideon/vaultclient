#ifndef vcUnitConversion_h__
#define vcUnitConversion_h__

enum vcDistanceUnit
{
  vcDistance_Metres,
  vcDistance_Kilometres,
  vcDistance_Centimetres,
  vcDistance_Millimetres,

  vcDistance_USSurveyFeet,
  vcDistance_USSurveyMiles,
  vcDistance_USSurveyInches,

  vcDistance_NauticalMiles,

  vcDistance_Count
};

enum vcAreaUnit
{
  vcArea_SquareMetres,
  vcArea_SquareKilometers,
  vcArea_Hectare,

  vcArea_SquareFoot,
  vcArea_SquareMiles,
  vcArea_Acre,

  vcArea_Count
};

enum vcVolumeUnit
{
  vcVolume_CubicMeter,
  vcVolume_MegaLiter,
  vcVolume_Litre,

  vcVolume_CubicInch,
  vcVolume_CubicFoot,

  vcVolume_USSGallons,
  vcVolume_USSQuart,
  vcVolume_CubicYard,

  vcVolume_Count
};

enum vcSpeedUnit
{
  vcSpeed_MetresPerSecond,
  vcSpeed_KilometresPerHour,

  vcSpeed_USSurveyMilesPerHour,
  vcSpeed_FeetPerSecond,

  vcSpeed_NauticalMilesPerHour, //Knots
  vcSpeed_Mach,

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
