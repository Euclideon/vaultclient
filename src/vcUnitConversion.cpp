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
  vcTimeReferenceData result = {false};

  static double const s_weekSeconds = 60.0 * 60.0 * 24.0 * 7.0;
  static double const s_secondsBetweenEpochs_TAI_Unix = 378691200.0;
  static double const s_secondsBetweenEpochs_TAI_GPS = 694656000.0;

  //TODO this will have to be updated every time a leap second is introduced.
  //The next leap second date may be December 2020.
  static const double s_leapSeconds[] = {
    78796811.0,   // 1972-06-01
    94694412.0,   // 1972-12-01
    126230413.0,  // 1973-12-01
    157766414.0,  // 1974-12-01
    189302415.0,  // 1975-12-01
    220924816.0,  // 1976-12-01
    252460817.0,  // 1977-12-01
    283996818.0,  // 1978-12-01
    315532819.0,  // 1979-12-01
    362793620.0,  // 1981-06-01
    394329621.0,  // 1982-06-01
    425865622.0,  // 1983-06-01
    489024023.0,  // 1985-06-01
    567993624.0,  // 1987-12-01
    631152025.0,  // 1988-12-01
    662688026.0,  // 1990-12-01
    709948827.0,  // 1992-06-01
    741484828.0,  // 1993-06-01
    773020829.0,  // 1994-06-01
    820454430.0,  // 1995-12-01
    867715231.0,  // 1997-06-01
    915148832.0,  // 1998-12-01
    1136073633.0, // 2005-12-01
    1230768034.0, // 2008-12-01
    1341100835.0, // 2012-06-01
    1435708836.0, // 2015-06-01
    1483228837.0  // 2016-12-01
  };

  //Convert sourceValue to TAI
  switch (sourceReference)
  {
    case vcTimeReference_TAI:
    {
      result.seconds = sourceValue.seconds;
      break;
    }
    case vcTimeReference_Unix:
    {
      if (sourceValue.seconds < 0.0)
        goto epilogue;

      double leapSeconds = 0.0;
      for (size_t i = 0; i < udLengthOf(s_leapSeconds); ++i)
      {
        if (sourceValue.seconds < s_leapSeconds[i])
          break;
        leapSeconds++;
      }
      result.seconds = s_secondsBetweenEpochs_TAI_Unix + sourceValue.seconds + leapSeconds;
      break;
    }
    case vcTimeReference_GPS:
    {
      result.seconds = sourceValue.seconds + s_secondsBetweenEpochs_TAI_GPS;
      break;
    }
    case vcTimeReference_GPSAdjusted:
    {
      result.seconds = sourceValue.seconds + (s_secondsBetweenEpochs_TAI_GPS + 1.0e9);
      break;
    }
    case vcTimeReference_GPSWeek:
    {
      if (sourceValue.GPSWeek.secondsOfTheWeek < 0.0)
        goto epilogue;

      result.seconds = s_secondsBetweenEpochs_TAI_GPS + sourceValue.GPSWeek.secondsOfTheWeek + s_weekSeconds * double(sourceValue.GPSWeek.weeks);
      break;
    }
  }

  //Required Reference
  switch (requiredReference)
  {
    case vcTimeReference_TAI:
    {
      break;
    }
    case vcTimeReference_Unix:
    {
      if (result.seconds < s_secondsBetweenEpochs_TAI_Unix)
        goto epilogue;

      result.seconds -= s_secondsBetweenEpochs_TAI_Unix;
      double leapSeconds = 0.0;
      for (size_t i = 0; i < udLengthOf(s_leapSeconds); ++i)
      {
        if (result.seconds < s_leapSeconds[i])
          break;
        leapSeconds++;
      }
      result.seconds = result.seconds - leapSeconds;
      break;
    }
    case vcTimeReference_GPS:
    {
      result.seconds -= s_secondsBetweenEpochs_TAI_GPS;
      break;
    }
    case vcTimeReference_GPSAdjusted:
    {
      result.seconds -= (s_secondsBetweenEpochs_TAI_GPS + 1.0e9);
      break;
    }
    case vcTimeReference_GPSWeek:
    {
      if (result.seconds < s_secondsBetweenEpochs_TAI_GPS)
        goto epilogue;

      result.seconds -= s_secondsBetweenEpochs_TAI_GPS;
      result.GPSWeek.weeks = (uint32_t)udFloor(result.seconds / s_weekSeconds);
      result.GPSWeek.secondsOfTheWeek = result.seconds - (s_weekSeconds * result.GPSWeek.weeks);
      break;
    }
  }

  result.success = true;
  epilogue:

  return result;
}
