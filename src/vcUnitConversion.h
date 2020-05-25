#ifndef vcUnitConversion_h__
#define vcUnitConversion_h__

#include <stdint.h>

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

enum vcTimeReference
{
  vcTimeReference_TAI,          //seconds since UTC 00:00:00, 1/1/1958
  vcTimeReference_Unix,         //seconds since UTC 00:00:00, 1/1/1970, not including leap seconds
  vcTimeReference_GPS,          //seconds since UTC 00:00:00, 6/1/1980
  vcTimeReference_GPSAdjusted,  //seconds since UTC 00:00:00, 6/1/1980 - 1'000'000'000
  vcTimeReference_GPSWeek,      //seconds of the week + week number since UTC 00:00:00, 6/1/1980 (sunday)

  vcTimeReference_Count
};

struct vcTimeReferenceData
{
  bool success;
  union
  {
    double seconds;
    struct
    {
      double secondsOfTheWeek;
      uint32_t weeks;
    } GPSWeek;
  };
};

// Trying to convert to or from a Unix time before the 1970 epoch will fail.
// Trying to convert to or from GPSWeek time using a negative value of seconds will fail.
vcTimeReferenceData vcUnitConversion_ConvertTimeReference(vcTimeReferenceData sourceValue, vcTimeReference sourceReference, vcTimeReference requiredReference);
double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit);
double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit);
double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit);
double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit);
double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit);

#endif //vcUnitConversion_h__
