#include "vcUnitConversion.h"
#include "udMath.h"

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

  const double sqmTable[vcArea_Count] = { 1.0, 1000000.0, 10000.0, 1200.0 / 3937.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0 * 1.0 / 640.0 };

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

  double celciusVal = sourceValue;

  //Source Unit 
  if (sourceUnit == vcTemperature_Kelvin)
    celciusVal = sourceValue - 273.15;
  else if (sourceUnit == vcTemperature_Farenheit)
    celciusVal = (5.0 / 9.0) * (sourceValue - 32);

  //Required Unit
  if (requiredUnit == vcTemperature_Kelvin)
    return (celciusVal + 273.15);
  else if (requiredUnit == vcTemperature_Farenheit)
    return (9.0 / 5.0) * celciusVal + 32;

  return celciusVal;
}

vcTimeReferenceData vcUnitConversion_ConvertTimeReference(vcTimeReferenceData sourceValue, vcTimeReference sourceReference, vcTimeReference requiredReference)
{
  if (sourceReference == requiredReference)
    return sourceValue;

  vcTimeReferenceData result = sourceValue;
  result.success = true;

  static double const s_weekSeconds = 60.0 * 60.0 * 24.0 * 7.0;
  static double const s_secondsBetweenEpochs_TAI_Unix = 378691200.0;
  static double const s_secondsBetweenEpochs_TAI_GPS = 694656000.0;

  //TODO this will have to be updated every time a leap second is introduced.
  //The next leap second date may be December 2020.
  static const double s_leapSeconds[] = {
    78796811.0,   // 1 - 6  - 1972
    94694412.0,   // 1 - 12 - 1972
    126230413.0,  // 1 - 12 - 1973
    157766414.0,  // 1 - 12 - 1974
    189302415.0,  // 1 - 12 - 1975
    220924816.0,  // 1 - 12 - 1976
    252460817.0,  // 1 - 12 - 1977
    283996818.0,  // 1 - 12 - 1978
    315532819.0,  // 1 - 12 - 1979
    362793620.0,  // 1 - 6  - 1981
    394329621.0,  // 1 - 6  - 1982
    425865622.0,  // 1 - 6  - 1983
    489024023.0,  // 1 - 6  - 1985
    567993624.0,  // 1 - 12 - 1987
    631152025.0,  // 1 - 12 - 1988
    662688026.0,  // 1 - 12 - 1990
    709948827.0,  // 1 - 6  - 1992
    741484828.0,  // 1 - 6  - 1993
    773020829.0,  // 1 - 6  - 1994
    820454430.0,  // 1 - 12 - 1995
    867715231.0,  // 1 - 6  - 1997
    915148832.0,  // 1 - 12 - 1998
    1136073633.0, // 1 - 12 - 2005
    1230768034.0, // 1 - 12 - 2008
    1341100835.0, // 1 - 6  - 2012
    1435708836.0, // 1 - 6  - 2015
    1483228837.0  // 1 - 12 - 2016
  };

  static const size_t s_leapSecondsCount = sizeof(s_leapSeconds) / sizeof(s_leapSeconds[0]);

  //Convert sourceValue to TAI
  if (sourceReference == vcTimeReference_Unix)
  {
    if (sourceValue.seconds < 0.0)
    {
      result.success = false;
      return result;
    }

    double leapSeconds = 0.0;
    for (size_t i = 0; i < s_leapSecondsCount; ++i)
    {
      if (result.seconds < s_leapSeconds[i])
        break;
      leapSeconds++;
    }
    result.seconds = s_secondsBetweenEpochs_TAI_Unix + result.seconds + leapSeconds;
  }
  else if (sourceReference == vcTimeReference_GPS)
  {
    result.seconds += s_secondsBetweenEpochs_TAI_GPS;
  }
  else if (sourceReference == vcTimeReference_GPSAdjusted)
  {
    result.seconds += s_secondsBetweenEpochs_TAI_GPS + 1.0e9;
  }
  else if (sourceReference == vcTimeReference_GPSWeek)
  {
    if (sourceValue.GPSWeek.seconds < 0.0)
    {
      result.success = false;
      return result;
    }

    result.seconds = s_secondsBetweenEpochs_TAI_GPS + sourceValue.GPSWeek.seconds + s_weekSeconds * double(sourceValue.GPSWeek.weeks);
  }

  //Required Reference
  if (requiredReference == vcTimeReference_Unix)
  {
    if (sourceValue.seconds < s_secondsBetweenEpochs_TAI_Unix)
    {
      result.success = false;
      return result;
    }

    result.seconds -= s_secondsBetweenEpochs_TAI_Unix;
    double leapSeconds = 0.0;
    for (size_t i = 0; i < s_leapSecondsCount; ++i)
    {
      if (result.seconds < s_leapSeconds[i])
        break;
      leapSeconds++;
    }
    result.seconds = result.seconds - leapSeconds;
  }
  else if (requiredReference == vcTimeReference_GPS)
  {
    result.seconds -= s_secondsBetweenEpochs_TAI_GPS;
  }
  else if (requiredReference == vcTimeReference_GPSAdjusted)
  {
    result.seconds -= (s_secondsBetweenEpochs_TAI_GPS + 1.0e9);
  }
  else if (requiredReference == vcTimeReference_GPSWeek)
  {
    if (sourceValue.seconds < s_secondsBetweenEpochs_TAI_GPS)
    {
      result.success = false;
      return result;
    }

    result.seconds -= s_secondsBetweenEpochs_TAI_GPS;
    result.GPSWeek.weeks = (unsigned)udFloor(result.seconds / s_weekSeconds);
    result.GPSWeek.seconds = result.seconds - (s_weekSeconds * result.GPSWeek.weeks);
  }

  return result;
}
