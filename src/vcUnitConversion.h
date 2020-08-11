#ifndef vcUnitConversion_h__
#define vcUnitConversion_h__

#include <stdint.h>
#include "udMath.h"

enum vcDistanceUnit
{
  vcDistance_Millimetres,
  vcDistance_Centimetres,
  vcDistance_Metres,
  vcDistance_Kilometres,

  vcDistance_USSurveyInches,
  vcDistance_USSurveyFeet,
  vcDistance_USSurveyMiles,

  vcDistance_NauticalMiles,

  vcDistance_Count
};

enum vcAreaUnit
{
  vcArea_SquareMetres,
  vcArea_SquareKilometres,
  vcArea_Hectare,

  vcArea_SquareFoot,
  vcArea_SquareMiles,
  vcArea_Acre,

  vcArea_Count
};

enum vcVolumeUnit
{
  vcVolume_CubicMetre,
  vcVolume_Litre,
  vcVolume_MegaLitre,

  vcVolume_USSurveyCubicInch,
  vcVolume_USSurveyCubicFoot,
  vcVolume_USSurveyCubicYard,

  vcVolume_USQuart,
  vcVolume_USGallons,

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
  vcTimeReference_UTC,

  vcTimeReference_Count
};

enum vcAngleUnit
{
  vcAngle_Degree,
  vcAngle_Radian,
  vcAngle_Gradian,

  vcAngle_Count
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
    struct
    {
      double seconds;
      uint16_t year;
      uint8_t month;
      uint8_t day;
      uint8_t hour;
      uint8_t minute;
    } UTC;
  };
};

struct vcUnitConversionData
{
  struct vcUnitConversionItem
  {
    uint32_t unit;
    double upperLimit;
  };

  static const int MaxPromotions = 4;

  vcUnitConversionItem distanceUnit[MaxPromotions];
  vcUnitConversionItem areaUnit[MaxPromotions];
  vcUnitConversionItem volumeUnit[MaxPromotions];
  vcSpeedUnit speedUnit;
  vcTemperatureUnit temperatureUnit;
  vcTimeReference timeReference;
  vcAngleUnit angleUnit;

  uint32_t distanceSigFigs;
  uint32_t areaSigFigs;
  uint32_t volumeSigFigs;
  uint32_t speedSigFigs;
  uint32_t temperatureSigFigs;
  uint32_t timeSigFigs;
  uint32_t angleSigFigs;
};

void vcUnitConversion_SetUTC(vcTimeReferenceData *pData, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, double seconds);

// Trying to convert to or from a Unix time before the 1970 epoch will fail.
// Trying to convert to or from GPSWeek time using a negative value of seconds will fail.
vcTimeReferenceData vcUnitConversion_ConvertTimeReference(vcTimeReferenceData sourceValue, vcTimeReference sourceReference, vcTimeReference requiredReference);
double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit);
double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit);
double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit);
double vcUnitConversion_ConvertSpeed(double sourceValue, vcSpeedUnit sourceUnit, vcSpeedUnit requiredUnit);
double vcUnitConversion_ConvertTemperature(double sourceValue, vcTemperatureUnit sourceUnit, vcTemperatureUnit requiredUnit);
double vcUnitConversion_ConvertAngle(double sourceValue, vcAngleUnit sourceUnit, vcAngleUnit requiredUnit);

int vcUnitConversion_ConvertTimeToString(char *pBuffer, size_t bufferSize, const vcTimeReferenceData &value, vcTimeReference reference, const char *pSecondsFormat = nullptr);
int vcUnitConversion_ConvertDistanceToString(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const char *pFormat = nullptr);
int vcUnitConversion_ConvertAreaToString(char *pBuffer, size_t bufferSize, double value, vcAreaUnit unit, const char *pFormat = nullptr);
int vcUnitConversion_ConvertVolumeToString(char *pBuffer, size_t bufferSize, double value, vcVolumeUnit unit, const char *pFormat = nullptr);
int vcUnitConversion_ConvertSpeedToString(char *pBuffer, size_t bufferSize, double value, vcSpeedUnit unit, const char *pFormat = nullptr);
int vcUnitConversion_ConvertTemperatureToString(char *pBuffer, size_t bufferSize, double value, vcTemperatureUnit unit, const char *pFormat = nullptr);
int vcUnitConversion_ConvertAngleToString(char *pBuffer, size_t bufferSize, double value, vcAngleUnit unit, const char *pFormat = nullptr);

//Functions to set up some quick defaults.
udResult vcUnitConversion_SetMetric(vcUnitConversionData *pData);
udResult vcUnitConversion_SetUSSurvey(vcUnitConversionData *pData);

udResult vcUnitConversion_ConvertAndFormatDistance(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatArea(char *pBuffer, size_t bufferSize, double value, vcAreaUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatVolume(char *pBuffer, size_t bufferSize, double value, vcVolumeUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatSpeed(char *pBuffer, size_t bufferSize, double value, vcSpeedUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatTemperature(char *pBuffer, size_t bufferSize, double value, vcTemperatureUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatTimeReference(char *pBuffer, size_t bufferSize, vcTimeReferenceData timeRefData, vcTimeReference unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatAngle(char *pBuffer, size_t bufferSize, double value, vcAngleUnit unit, const vcUnitConversionData *pData);

#endif //vcUnitConversion_h__
