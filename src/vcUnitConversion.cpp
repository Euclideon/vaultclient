#include "vcUnitConversion.h"
#include "udMath.h"
#include "udStringUtil.h"


//I think we need this so we are using static strings. Otherwise the vcUnitConversion_Convert* API will need to change to allow for a significant figure param.
static const char *gSigFigsFormats[] = {"%0.0f", "%0.1f", "%0.2f", "%0.3f", "%0.4f", "%0.5f", "%0.6f", "%0.7f", "%0.8f", "%0.9f", "%0.10f"};
static const char *gTimeSigFigsFormats[] = {"%02.0f", "%04.1f", "%05.2f", "%06.3f", "%07.4f", "%08.5f", "%09.6f", "%010.7f", "%011.8f", "%012.9f", "%013.10f"};
static const uint32_t gSigFigCount = (uint32_t)udLengthOf(gSigFigsFormats);

#define USSINCHES_IN_METRE      39.37
#define USSINCHES_IN_METRE_SQ  (39.37 * 39.37)
#define USSINCHES_IN_METRE_CB  (39.37 * 39.37 * 39.37)

double vcUnitConversion_ConvertDistance(double sourceValue, vcDistanceUnit sourceUnit, vcDistanceUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double metreTable[vcDistance_Count] = { 0.001, 0.01, 1.0, 1000.0, 1.0 / USSINCHES_IN_METRE, 1.0 / USSINCHES_IN_METRE * 12.0, 1.0 / USSINCHES_IN_METRE *12.0 * 5280.0, 1852.0};

  return sourceValue * metreTable[sourceUnit] / metreTable[requiredUnit];
}

double vcUnitConversion_ConvertArea(double sourceValue, vcAreaUnit sourceUnit, vcAreaUnit requiredUnit) 
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double sqmTable[vcArea_Count] = { 1.0, 1000.0 * 1000.0, 100.0 * 100.0, 1200.0 / 3937.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0, 5280.0 * 1200.0 / 3937.0 * 5280.0 * 1200.0 / 3937.0 * 1.0 / 640.0 };

  return sourceValue * sqmTable[sourceUnit] / sqmTable[requiredUnit];
}

double vcUnitConversion_ConvertVolume(double sourceValue, vcVolumeUnit sourceUnit, vcVolumeUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  const double m3Table[vcVolume_Count] = { 1.0, 1.0 / 1000.0, 1000.0, 1.0 / USSINCHES_IN_METRE_CB, 1 / (USSINCHES_IN_METRE_CB / 1728), 1 / (USSINCHES_IN_METRE_CB / 1728.0 / 27.0), 1.0 / 1056.6882607957347, 1.0 / 264.17203728418462560};

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

double vcUnitConversion_ConvertAngle(double sourceValue, vcAngleUnit sourceUnit, vcAngleUnit requiredUnit)
{
  if (sourceUnit == requiredUnit)
    return sourceValue;

  static const double angleTable[vcSpeed_Count] = {1.0, 180.0 / UD_PI, 18.0 / 20.0};

  return sourceValue * angleTable[sourceUnit] / angleTable[requiredUnit];
}

// Algorithm: http: //howardhinnant.github.io/date_algorithms.html
static int vcUnitConversion_DaysFromCivil(int y, int m, int d)
{
  y -= m <= 2 ? 1 : 0;
  int era = y / 400;
  int yoe = y - era * 400;                                   // [0, 399]
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           // [0, 146096]
  return era * 146097 + doe - 719468;
}

// Algorithm: http: //howardhinnant.github.io/date_algorithms.html
static vcTimeReferenceData vcUnitConversion_UnixToUTC(double unixTime)
{
  vcTimeReferenceData result = {};
  int days = (int)unixTime / (60 * 60 * 24);
  double rem = unixTime - ((double)days * 60.0 * 60.0 * 24.0);
  result.UTC.hour = (uint8_t)(rem / (60.0 * 60.0));
  rem -= 60.0 * 60.0 * result.UTC.hour;
  result.UTC.minute = (uint8_t)(rem / (60.0));
  rem -= 60.0 * result.UTC.minute;
  result.UTC.seconds = rem;

  days += 719468;
  int era = (days >= 0 ? days : days - 146096) / 146097;
  uint32_t doe = (uint32_t)(days - era * 146097);                         // [0, 146096]
  uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;   // [0, 399]
  int y = (int)(yoe)+era * 400;
  uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                 // [0, 365]
  uint32_t mp = (5 * doy + 2) / 153;                                      // [0, 11]
  uint32_t d = doy - (153 * mp + 2) / 5 + 1;                              // [1, 31]
  uint32_t m = mp + (mp < 10 ? 3 : -9);                                   // [1, 12]

  result.UTC.year = (uint16_t)y + (m <= 2 ? 1 : 0);
  result.UTC.month = (uint8_t)m;
  result.UTC.day = (uint8_t)d;

  return result;
}

static double vcUnitConversion_ConvertUTCtoUNIX(vcTimeReferenceData timeData)
{
  int year = (int)timeData.UTC.year;
  uint16_t month = (uint16_t)timeData.UTC.month;          // 0-11
  if (month > 11)
  {
    year += month / 12;
    month %= 12;
  }
  int days_since_1970 = vcUnitConversion_DaysFromCivil(year, month, (int)timeData.UTC.day);

  return (double)60 * (60 * (24L * days_since_1970 + (int)timeData.UTC.hour) + (int)timeData.UTC.minute) + timeData.UTC.seconds;
}

vcTimeReferenceData vcUnitConversion_ConvertTimeReference(vcTimeReferenceData sourceValue, vcTimeReference sourceReference, vcTimeReference requiredReference)
{
  vcTimeReferenceData result = {false, {0.0}};
  double TAI_seconds = 0.0;
  double currentLeapSeconds = 0.0;

  static const double s_weekSeconds = 60.0 * 60.0 * 24.0 * 7.0;
  static const double s_secondsBetweenEpochs_TAI_Unix = 378691200.0;
  static const double s_secondsBetweenEpochs_TAI_GPS = 694656009.0;

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
      TAI_seconds = sourceValue.seconds;
      break;
    }
    case vcTimeReference_Unix:
    {
      if (sourceValue.seconds < 0.0)
        goto epilogue;

      currentLeapSeconds = 0.0;
      for (size_t i = 0; i < udLengthOf(s_leapSeconds); ++i)
      {
        if (sourceValue.seconds < s_leapSeconds[i])
          break;
        currentLeapSeconds++;
      }
      TAI_seconds = s_secondsBetweenEpochs_TAI_Unix + sourceValue.seconds + currentLeapSeconds;
      break;
    }
    case vcTimeReference_GPS:
    {
      TAI_seconds = sourceValue.seconds + s_secondsBetweenEpochs_TAI_GPS;
      break;
    }
    case vcTimeReference_GPSAdjusted:
    {
      TAI_seconds = sourceValue.seconds  + 1.0e9 + s_secondsBetweenEpochs_TAI_GPS;
      break;
    }
    case vcTimeReference_GPSWeek:
    {
      if (sourceValue.GPSWeek.secondsOfTheWeek < 0.0)
        goto epilogue;

      TAI_seconds = s_secondsBetweenEpochs_TAI_GPS + s_weekSeconds * double(sourceValue.GPSWeek.weeks) + sourceValue.GPSWeek.secondsOfTheWeek;
      break;
    }
    case vcTimeReference_UTC:
    {
      vcTimeReferenceData tempIn;
      tempIn.seconds = vcUnitConversion_ConvertUTCtoUNIX(sourceValue);
      vcTimeReferenceData tempOut = vcUnitConversion_ConvertTimeReference(tempIn, vcTimeReference_Unix, vcTimeReference_TAI);
      TAI_seconds = tempOut.seconds;
      break;
    }
    default:
    {
      goto epilogue;
    }
  }

  //Required Reference
  switch (requiredReference)
  {
    case vcTimeReference_TAI:
    {
      result.seconds = TAI_seconds;
      break;
    }
    case vcTimeReference_Unix:
    {
      if (TAI_seconds < s_secondsBetweenEpochs_TAI_Unix)
        goto epilogue;

      result.seconds = TAI_seconds - s_secondsBetweenEpochs_TAI_Unix;
      currentLeapSeconds = 0.0;
      for (size_t i = 0; i < udLengthOf(s_leapSeconds); ++i)
      {
        if (result.seconds < s_leapSeconds[i])
          break;
        currentLeapSeconds++;
      }
      result.seconds -= currentLeapSeconds;
      break;
    }
    case vcTimeReference_GPS:
    {
      result.seconds = TAI_seconds - s_secondsBetweenEpochs_TAI_GPS;
      break;
    }
    case vcTimeReference_GPSAdjusted:
    {
      result.seconds = TAI_seconds - s_secondsBetweenEpochs_TAI_GPS - 1.0e9;
      break;
    }
    case vcTimeReference_GPSWeek:
    {
      if (TAI_seconds < s_secondsBetweenEpochs_TAI_GPS)
        goto epilogue;

      TAI_seconds -= s_secondsBetweenEpochs_TAI_GPS;
      result.GPSWeek.weeks = (uint32_t)udFloor(TAI_seconds / s_weekSeconds);
      result.GPSWeek.secondsOfTheWeek = TAI_seconds - (s_weekSeconds * result.GPSWeek.weeks);
      break;
    }
    case vcTimeReference_UTC:
    {
      vcTimeReferenceData tempIn;
      tempIn.seconds = TAI_seconds;
      vcTimeReferenceData tempOut = vcUnitConversion_ConvertTimeReference(tempIn, vcTimeReference_TAI, vcTimeReference_Unix);
      result = vcUnitConversion_UnixToUTC(tempOut.seconds);
      break;
    }
    default:
    {
      goto epilogue;
    }
  }

  result.success = true;

epilogue:
  return result;
}

int vcUnitConversion_ConvertTimeToString(char *pBuffer, size_t bufferSize, const vcTimeReferenceData &value, vcTimeReference reference, const char *pSecondsFormat)
{
  if (pBuffer == nullptr)
    return -1;

  if (pSecondsFormat == nullptr)
  {
    if (reference == vcTimeReference_GPSWeek)
      return udSprintf(pBuffer, bufferSize, "%i weeks, %fs", value.GPSWeek.weeks, value.GPSWeek.secondsOfTheWeek);
    else if (reference == vcTimeReference_UTC)
      return udSprintf(pBuffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%02uZ", value.UTC.year, value.UTC.month, value.UTC.day, value.UTC.hour, value.UTC.minute, (uint32_t)value.UTC.seconds);
    else
      return udSprintf(pBuffer, bufferSize, "%fs", value.seconds);
  }
  else
  {
    if (reference == vcTimeReference_GPSWeek)
      return udSprintf(pBuffer, bufferSize, "%i weeks, %ss", value.GPSWeek.weeks, udTempStr(pSecondsFormat, value.GPSWeek.secondsOfTheWeek));
    else if (reference == vcTimeReference_UTC)
      return udSprintf(pBuffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%sZ", value.UTC.year, value.UTC.month, value.UTC.day, value.UTC.hour, value.UTC.minute, udTempStr(pSecondsFormat, value.UTC.seconds));
    else
      return udSprintf(pBuffer, bufferSize, "%ss", udTempStr(pSecondsFormat, value.seconds));
  }
}

int vcUnitConversion_ConvertDistanceToString(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"mm", "cm", "m", "km", "in", "ft", "mi", "nmi"};

  if (pBuffer == nullptr || unit == vcDistance_Count)
    return -1;

  if (pFormat == nullptr)
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  else
    return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

int vcUnitConversion_ConvertAreaToString(char *pBuffer, size_t bufferSize, double value, vcAreaUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"sqm", "sqkm", "ha", "sqft", "sqmi", "ac"};

  if (pBuffer == nullptr || unit == vcArea_Count)
    return -1;

  if (pFormat == nullptr)
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  else
    return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

int vcUnitConversion_ConvertVolumeToString(char *pBuffer, size_t bufferSize, double value, vcVolumeUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"cbm", "L", "ML", "cbin", "cbft", "cbyd", "qt US", "gal US"};

  if (pBuffer == nullptr || unit == vcVolume_Count)
    return -1;

  if (pFormat == nullptr)
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  else
    return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

int vcUnitConversion_ConvertSpeedToString(char *pBuffer, size_t bufferSize, double value, vcSpeedUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"m/s", "km/h", "mi/h", "ft/s", "nmi/h", "Mach"};

  if (pBuffer == nullptr || unit == vcSpeed_Count)
    return -1;

  if (pFormat == nullptr)
  {
    if (unit == vcSpeed_Mach)
      return udSprintf(pBuffer, bufferSize, "%s %f", Suffixes[unit], value);
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  }

  if (unit == vcSpeed_Mach)
    return udSprintf(pBuffer, bufferSize, "%s %s", Suffixes[unit], udTempStr(pFormat, value));
  return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

int vcUnitConversion_ConvertTemperatureToString(char *pBuffer, size_t bufferSize, double value, vcTemperatureUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"C", "K", "F"};

  if (pBuffer == nullptr || unit == vcTemperature_Count)
    return -1;

  if (pFormat == nullptr)
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  else
    return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

int vcUnitConversion_ConvertAngleToString(char *pBuffer, size_t bufferSize, double value, vcAngleUnit unit, const char *pFormat)
{
  static const char *Suffixes[] = {"deg", "rad", "grad"};

  if (pBuffer == nullptr || unit == vcAngle_Count)
    return -1;

  if (pFormat == nullptr)
    return udSprintf(pBuffer, bufferSize, "%f%s", value, Suffixes[unit]);
  else
    return udSprintf(pBuffer, bufferSize, "%s%s", udTempStr(pFormat, value), Suffixes[unit]);
}

udResult vcUnitConversion_SetMetric(vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;

  if (pData == nullptr)
    UD_ERROR_SET(udR_InvalidParameter_);

  pData->distanceUnit[0] = {vcDistance_Millimetres, 10.0};
  pData->distanceUnit[1] = {vcDistance_Centimetres, 100.0};
  pData->distanceUnit[2] = {vcDistance_Metres, 1000.0};
  pData->distanceUnit[3] = {vcDistance_Kilometres, 0.0};

  pData->areaUnit[0] = {vcArea_SquareMetres, 1'000'000.0};
  pData->areaUnit[1] = {vcArea_SquareKilometres, 0.0};
  pData->areaUnit[2] = {vcArea_SquareKilometres, 0.0};
  pData->areaUnit[3] = {vcArea_SquareKilometres, 0.0};

  pData->volumeUnit[0] = {vcVolume_Litre, 1'000'000.0};
  pData->volumeUnit[1] = {vcVolume_MegaLitre, 0.0};
  pData->volumeUnit[2] = {vcVolume_MegaLitre, 0.0};
  pData->volumeUnit[3] = {vcVolume_MegaLitre, 0.0};

  pData->speedUnit = vcSpeed_MetresPerSecond;
  pData->temperatureUnit = vcTemperature_Celcius;
  pData->timeReference = vcTimeReference_UTC;
  pData->angleUnit = vcAngle_Degree;

  pData->distanceSigFigs = 3;
  pData->areaSigFigs = 3;
  pData->volumeSigFigs = 3;
  pData->speedSigFigs = 3;
  pData->temperatureSigFigs = 3;
  pData->timeSigFigs = 0;
  pData->angleSigFigs = 2;

  result = udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_SetUSSurvey(vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;

  if (pData == nullptr)
    UD_ERROR_SET(udR_InvalidParameter_);

  pData->distanceUnit[0] = {vcDistance_USSurveyInches, 12.0};
  pData->distanceUnit[1] = {vcDistance_USSurveyFeet, 5280.0};   //Do we say 1.33ft or 1 foot, 4 inches?
  pData->distanceUnit[2] = {vcDistance_USSurveyMiles, 0.0};     //Yards?
  pData->distanceUnit[3] = {vcDistance_USSurveyMiles, 0.0};

  pData->areaUnit[0] = {vcArea_SquareFoot, 5280.0 * 5280.0};
  pData->areaUnit[1] = {vcArea_SquareMiles, 0.0};
  pData->areaUnit[2] = {vcArea_SquareMiles, 0.0};
  pData->areaUnit[3] = {vcArea_SquareMiles, 0.0};

  pData->volumeUnit[0] = {vcVolume_USGallons, 0.0};
  pData->volumeUnit[1] = {vcVolume_USGallons, 0.0};
  pData->volumeUnit[2] = {vcVolume_USGallons, 0.0};
  pData->volumeUnit[3] = {vcVolume_USGallons, 0.0};

  pData->speedUnit = vcSpeed_FeetPerSecond;
  pData->temperatureUnit = vcTemperature_Farenheit;
  pData->timeReference = vcTimeReference_UTC;
  pData->angleUnit = vcAngle_Degree;

  pData->distanceSigFigs = 3;
  pData->areaSigFigs = 3;
  pData->volumeSigFigs = 3;
  pData->speedSigFigs = 3;
  pData->temperatureSigFigs = 3;
  pData->timeSigFigs = 0;
  pData->angleSigFigs = 2;

  result = udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatDistance(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  vcDistanceUnit finalUnit;
  double finalValue;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcDistance_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  for (int i = 0; i < vcUnitConversionData::MaxPromotions; ++i)
  {
    finalUnit = (vcDistanceUnit)pData->distanceUnit[i].unit;
    finalValue = vcUnitConversion_ConvertDistance(value, unit, finalUnit);

    if (udAbs(finalValue) < pData->distanceUnit[i].upperLimit)
      break;
  }

  sigFigs = udClamp<uint32_t>(pData->distanceSigFigs, 0, gSigFigCount - 1);
  result = vcUnitConversion_ConvertDistanceToString(pBuffer, bufferSize, finalValue, finalUnit, gSigFigsFormats[sigFigs]) == -1 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}


udResult vcUnitConversion_ConvertAndFormatArea(char *pBuffer, size_t bufferSize, double value, vcAreaUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  vcAreaUnit finalUnit;
  double finalValue;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcArea_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  for (int i = 0; i < vcUnitConversionData::MaxPromotions; ++i)
  {
    finalUnit = (vcAreaUnit)pData->areaUnit[i].unit;
    finalValue = vcUnitConversion_ConvertArea(value, unit, finalUnit);

    if (udAbs(finalValue) < pData->areaUnit[i].upperLimit)
      break;
  }

  sigFigs = udClamp<uint32_t>(pData->areaSigFigs, 0, gSigFigCount - 1);
  result = vcUnitConversion_ConvertAreaToString(pBuffer, bufferSize, finalValue, finalUnit, gSigFigsFormats[sigFigs]) == -1 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatVolume(char *pBuffer, size_t bufferSize, double value, vcVolumeUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  vcVolumeUnit finalUnit;
  double finalValue;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcVolume_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  for (int i = 0; i < vcUnitConversionData::MaxPromotions; ++i)
  {
    finalUnit = (vcVolumeUnit)pData->volumeUnit[i].unit;
    finalValue = vcUnitConversion_ConvertVolume(value, unit, finalUnit);

    if (udAbs(finalValue) < pData->volumeUnit[i].upperLimit)
      break;
  }

  sigFigs = udClamp<uint32_t>(pData->volumeSigFigs, 0, gSigFigCount - 1);
  result = vcUnitConversion_ConvertVolumeToString(pBuffer, bufferSize, finalValue, finalUnit, gSigFigsFormats[sigFigs]) == -1 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatSpeed(char *pBuffer, size_t bufferSize, double value, vcSpeedUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  double finalValue;
  int convertResult;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcSpeed_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  finalValue = vcUnitConversion_ConvertSpeed(value, unit, pData->speedUnit);

  sigFigs = udClamp<uint32_t>(pData->speedSigFigs, 0, gSigFigCount - 1);
  convertResult = vcUnitConversion_ConvertSpeedToString(pBuffer, bufferSize, finalValue, pData->speedUnit, gSigFigsFormats[sigFigs]);

  result = convertResult < 0 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatTemperature(char *pBuffer, size_t bufferSize, double value, vcTemperatureUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  double finalValue;
  int convertResult;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcTemperature_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  finalValue = vcUnitConversion_ConvertTemperature(value, unit, pData->temperatureUnit);

  sigFigs = udClamp<uint32_t>(pData->temperatureSigFigs, 0, gSigFigCount - 1);
  convertResult = vcUnitConversion_ConvertTemperatureToString(pBuffer, bufferSize, finalValue, pData->temperatureUnit, gSigFigsFormats[sigFigs]);

  result = convertResult < 0 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatTimeReference(char *pBuffer, size_t bufferSize, vcTimeReferenceData timeRefData, vcTimeReference unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  vcTimeReferenceData finalValue;
  int convertResult;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcTimeReference_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  finalValue = vcUnitConversion_ConvertTimeReference(timeRefData, unit, pData->timeReference);

  sigFigs = udClamp<uint32_t>(pData->timeSigFigs, 0, gSigFigCount - 1);
  convertResult = vcUnitConversion_ConvertTimeToString(pBuffer, bufferSize, finalValue, pData->timeReference, gTimeSigFigsFormats[sigFigs]);

  result = convertResult < 0 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatAngle(char *pBuffer, size_t bufferSize, double value, vcAngleUnit unit, const vcUnitConversionData *pData)
{
  udResult result = udR_Failure_;
  int sigFigs;
  double finalValue;
  int convertResult;

  if (pData == nullptr || pBuffer == nullptr || unit < 0 || unit >= vcAngle_Count)
    UD_ERROR_SET(udR_InvalidParameter_);

  finalValue = vcUnitConversion_ConvertAngle(value, unit, pData->angleUnit);

  sigFigs = udClamp<uint32_t>(pData->angleSigFigs, 0, gSigFigCount - 1);
  convertResult = vcUnitConversion_ConvertAngleToString(pBuffer, bufferSize, finalValue, pData->angleUnit, gSigFigsFormats[sigFigs]);

  result = convertResult < 0 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

void vcUnitConversion_SetUTC(vcTimeReferenceData *pData, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, double seconds)
{
  if (pData == nullptr)
    return;

  pData->UTC.year = year;
  pData->UTC.month = month;
  pData->UTC.day = day;
  pData->UTC.hour = hour;
  pData->UTC.minute = minute;
  pData->UTC.seconds = seconds;
}
