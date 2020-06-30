#include "vcUnitConverter.h"

udResult vcUnitConversion_SetMetric(vcUnitConversionData *pData)
{
  udResult result;

  if (pData == nullptr)
    UD_ERROR_SET(udR_InvalidParameter_);

  pData->distance[0] = {vcDistance_Millimetres, 10.0};
  pData->distance[1] = {vcDistance_Centimetres, 100.0};
  pData->distance[2] = {vcDistance_Metres, 1000.0};
  pData->distance[3] = {vcDistance_Kilometres, 0.0};

  pData->area[0] = {vcArea_SquareMetres, 10000.0};
  pData->area[1] = {vcArea_Hectare, 100.0};
  pData->area[2] = {vcArea_SquareKilometers, 0.0};
  pData->area[3] = {vcArea_SquareKilometers, 0.0};

  pData->volume[0] = {vcVolume_Litre, 1'000'000.0};
  pData->volume[1] = {vcVolume_MegaLiter, 0.0};
  pData->volume[2] = {vcVolume_MegaLiter, 0.0};
  pData->volume[3] = {vcVolume_MegaLiter, 0.0};

  pData->speed = vcSpeed_MetresPerSecond;
  pData->temperature = vcTemperature_Celcius;
  pData->time = vcTimeReference_Unix;

  pData->distanceSigFigs = 3;
  pData->areaSigFigs = 3;
  pData->volumeSigFigs = 3;
  pData->speedSigFigs = 3;
  pData->teamperatureSigFigs = 3;
  pData->timeSigFigs = 1;

  result = udR_Success;
   
epilogue:
  return result;
}

udResult vcUnitConversion_SetImperial(vcUnitConversionData *pData)
{
  udResult result;

  if (pData == nullptr)
    UD_ERROR_SET(udR_InvalidParameter_);

  pData->distance[0] = {vcDistance_USSurveyInches, 12.0}; 
  pData->distance[1] = {vcDistance_USSurveyFeet, 5280.0};   //Do we say 1.33ft or 1 foot, 4 inches?
  pData->distance[2] = {vcDistance_USSurveyMiles, 0.0};     //Yards?
  pData->distance[3] = {vcDistance_USSurveyMiles, 0.0};

  pData->area[0] = {vcArea_SquareFoot, 43560.0};
  pData->area[1] = {vcArea_Acre, 640.0};
  pData->area[2] = {vcArea_SquareMiles, 0.0};
  pData->area[3] = {vcArea_SquareMiles, 0.0};

  pData->volume[0] = {vcVolume_USSGallons, 0.0};
  pData->volume[1] = {vcVolume_USSGallons, 0.0};
  pData->volume[2] = {vcVolume_USSGallons, 0.0};
  pData->volume[3] = {vcVolume_USSGallons, 0.0};

  pData->speed = vcSpeed_FeetPerSecond;
  pData->temperature = vcTemperature_Farenheit;
  pData->time = vcTimeReference_Unix;

  pData->distanceSigFigs = 3;
  pData->areaSigFigs = 3;
  pData->volumeSigFigs = 3;
  pData->speedSigFigs = 3;
  pData->teamperatureSigFigs = 3;
  pData->timeSigFigs = 1;

  result = udR_Success;

epilogue:
  return result;
}

//I think we need this so we are using static strings. Otherwise the vcUnitConversion_Convert* API will need to change to allow for a sig fig param.
static const char * sigFigsFormats[11] = {"%0.0f", "%0.1f", "%0.2f", "%0.3f", "%0.4f", "%0.5f", "%0.6f", "%0.7f", "%0.8f", "%0.9f", "%0.10f"};

udResult vcUnitConversion_ConvertAndFormatDistance(char *pBuffer, size_t bufferSize, double value, vcDistanceUnit unit, const vcUnitConversionData *pData)
{
  udResult result;
  int sigFigs;

  if (pData == nullptr)
    UD_ERROR_SET(udR_InvalidParameter_);

  vcDistanceUnit finalUnit;
  double finalValue;

  for (int i = 0; i < vcUnitConversionData::MaxElements; ++i)
  {
    finalUnit = (vcDistanceUnit)pData->distance[i].unit;
    finalValue = vcUnitConversion_ConvertDistance(value, unit, finalUnit);

    if (finalValue < pData->distance[i].maxValue)
      break;
  }

  sigFigs = udClamp(pData->distanceSigFigs, 0, 10);

  result = vcUnitConversion_ConvertDistanceToString(pBuffer, bufferSize, finalValue, finalUnit, sigFigsFormats[sigFigs]) == -1 ? udR_WriteFailure : udR_Success;

epilogue:
  return result;
}

udResult vcUnitConversion_ConvertAndFormatArea(char *pBuffer, size_t bufferSize, double metersSq, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatVolume(char *pBuffer, size_t bufferSize, double metersCub, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatSpeed(char *pBuffer, size_t bufferSize, double metersPsec, const vcUnitConversionData *pData);
udResult vcUnitConversion_ConvertAndFormatTemperature(char *pBuffer, size_t bufferSize, double kelvin, const vcUnitConversionData *pData);
