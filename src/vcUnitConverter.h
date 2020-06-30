#ifndef vcUnitConverter_h__
#define vcUnitConverter_h__

#include "vcUnitConversion.h"
#include "udResult.h"

struct vcUnitItem
{
  int unit;
  double maxValue;
};

struct vcUnitConversionData
{
  static const int MaxElements = 4;

  vcUnitItem distance[MaxElements];
  vcUnitItem area[MaxElements];
  vcUnitItem volume[MaxElements];
  vcSpeedUnit speed;
  vcTemperatureUnit temperature;
  vcTimeReference time;

  int distanceSigFigs;
  int areaSigFigs;
  int volumeSigFigs;
  int speedSigFigs;
  int teamperatureSigFigs;
  int timeSigFigs;
};

//Functions to set up some quick defaults...
udResult vcUnitConversion_SetMetric(vcUnitConversionData *pData);
udResult vcUnitConversion_SetImperial(vcUnitConversionData *pData);

//Converts a value to a desired unit and builds a string.
udResult vcUnitConversion_ConvertAndFormatDistance(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatArea(char *pBuffer, size_t bufferSize, double metersSq, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatVolume(char *pBuffer, size_t bufferSize, double metersCub, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatSpeed(char *pBuffer, size_t bufferSize, double metersPsec, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatTemperature(char *pBuffer, size_t bufferSize, double kelvin, const vcUnitConversionData *pData);

#endif
