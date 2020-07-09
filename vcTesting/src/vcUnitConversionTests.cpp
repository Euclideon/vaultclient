#include "vcUnitConversion.h"
#include "vcTesting.h"
#include "udStringUtil.h"

void VerifyDistance(vcDistanceUnit sourceType, double in, vcDistanceUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertDistance(in, sourceType, destType), 0.01) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertDistance(out, destType, sourceType), 0.01) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, Distance)
{
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_Millimetres, 1000.0);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_Centimetres, 100.0);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_Metres, 1.0);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_Kilometres, 0.001);

  VerifyDistance(vcDistance_Millimetres, 1.0, vcDistance_Metres, 0.001);
  VerifyDistance(vcDistance_Centimetres, 1.0, vcDistance_Metres, 0.01);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_Metres, 1.0);
  VerifyDistance(vcDistance_Kilometres, 1.0, vcDistance_Metres, 1000.0);

  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_USSurveyInches, 39.37);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_USSurveyFeet, 39.37 / 12);
  VerifyDistance(vcDistance_Metres, 1.0, vcDistance_USSurveyMiles, 39.37 / 12 / 5280);

  VerifyDistance(vcDistance_Kilometres, 100.0, vcDistance_NauticalMiles, 53.995680);
  VerifyDistance(vcDistance_Metres, 100.0, vcDistance_Centimetres, 10000.0);
  VerifyDistance(vcDistance_USSurveyFeet, 1.0, vcDistance_Millimetres, 304.800610);
  VerifyDistance(vcDistance_USSurveyInches, 100000.0, vcDistance_USSurveyMiles, 1.57828282);
  VerifyDistance(vcDistance_USSurveyInches, 0.5, vcDistance_Centimetres, 1.27);
  VerifyDistance(vcDistance_Centimetres, 100000.0, vcDistance_Kilometres, 1.0);

  for (int i = 0; i < vcDistance_Count; ++i)
  {
    // Round trips
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertDistance(vcUnitConversion_ConvertDistance(10000.0, vcDistance_Metres, (vcDistanceUnit)i), (vcDistanceUnit)i, vcDistance_Metres));

    // All should be 0 from 0
    EXPECT_DOUBLE_EQ(0.0, vcUnitConversion_ConvertDistance(0.0, vcDistance_Metres, (vcDistanceUnit)i));
  }
}

void VerifyArea(vcAreaUnit sourceType, double in, vcAreaUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertArea(in, sourceType, destType), 0.01) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertArea(out, destType, sourceType), 0.01) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, Area)
{
  VerifyArea(vcArea_SquareMetres, 100.0, vcArea_SquareKilometres, 0.0001);
  VerifyArea(vcArea_SquareMetres, 100.0, vcArea_Hectare, 0.01);
  VerifyArea(vcArea_SquareMetres, 100.0, vcArea_SquareFoot, 1076.391041670972072097356431186199188232421875);
  VerifyArea(vcArea_SquareMetres, 100.0, vcArea_SquareMiles, 0.0000386102160083283757);
  VerifyArea(vcArea_SquareMetres, 100.0, vcArea_Acre, 0.0247105382834933301472446487423);

  for (int i = 0; i < vcArea_Count; ++i)
  {
    // Round trips
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertArea(vcUnitConversion_ConvertArea(10000.0, vcArea_SquareMetres, (vcAreaUnit)i), (vcAreaUnit)i, vcArea_SquareMetres));

    // All should be 0 from 0
    EXPECT_DOUBLE_EQ(0.0, vcUnitConversion_ConvertArea(0.0, vcArea_SquareMetres, (vcAreaUnit)i));
  }
}

void VerifyVolume(vcVolumeUnit sourceType, double in, vcVolumeUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertVolume(in, sourceType, destType), 0.01) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertVolume(out, destType, sourceType), 0.01) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, Volume)
{
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_CubicMetre, 1.0);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_Litre, 1000.0);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_MegaLitre, 1.0 / 1000.0);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USSurveyCubicInch, 39.37 * 39.37 * 39.37);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USSurveyCubicFoot, 35.31466621266132221990);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USSurveyCubicYard, 1.30795061586555);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USQuart, 1056.6882607957347);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USGallons, 264.17203728418462);

  VerifyVolume(vcVolume_CubicMetre, 159.0, vcVolume_USSurveyCubicInch, 9702717.0945269987);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_MegaLitre, 0.001);
  VerifyVolume(vcVolume_USSurveyCubicInch, 159.0, vcVolume_CubicMetre, 0.00260554317599999983);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USSurveyCubicYard, 1.30795061586555);
  VerifyVolume(vcVolume_CubicMetre, 1.0, vcVolume_USSurveyCubicFoot, 35.31466621266132221990);
  VerifyVolume(vcVolume_USSurveyCubicYard, 1.0, vcVolume_CubicMetre, 0.764554859999999995);
  
  for (int i = 0; i < vcVolume_Count; ++i)
  {
    // Round trips
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertVolume(vcUnitConversion_ConvertVolume(10000.0, vcVolume_CubicMetre, (vcVolumeUnit)i), (vcVolumeUnit)i, vcVolume_CubicMetre));
  
    // All should be 0 from 0
    EXPECT_DOUBLE_EQ(0.0, vcUnitConversion_ConvertVolume(0.0, vcVolume_CubicMetre, (vcVolumeUnit)i));
  }
}

void VerifySpeed(vcSpeedUnit sourceType, double in, vcSpeedUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertSpeed(in, sourceType, destType), 0.01) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertSpeed(out, destType, sourceType), 0.01) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, Speed)
{
  VerifySpeed(vcSpeed_MetresPerSecond, 100, vcSpeed_KilometresPerHour, 360.0);
  VerifySpeed(vcSpeed_MetresPerSecond, 100, vcSpeed_USSurveyMilesPerHour, 223.69362920544023154);
  VerifySpeed(vcSpeed_MetresPerSecond, 100, vcSpeed_NauticalMilesPerHour, 194.38444924406047675);
  VerifySpeed(vcSpeed_MetresPerSecond, 100, vcSpeed_FeetPerSecond, 328.08398950131231686100363);
  VerifySpeed(vcSpeed_Mach, 1.0, vcSpeed_MetresPerSecond, 340.29);

  for (int i = 0; i < vcSpeed_Count; ++i)
  {
    // Round trips
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertSpeed(vcUnitConversion_ConvertSpeed(10000.0, vcSpeed_MetresPerSecond, (vcSpeedUnit)i), (vcSpeedUnit)i, vcSpeed_MetresPerSecond));

    // All should be 0 from 0
    EXPECT_DOUBLE_EQ(0.0, vcUnitConversion_ConvertSpeed(0.0, vcSpeed_MetresPerSecond, (vcSpeedUnit)i));
  }
}

void VerifyTemperature(vcTemperatureUnit sourceType, double in, vcTemperatureUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertTemperature(in, sourceType, destType), 0.01) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertTemperature(out, destType, sourceType), 0.01) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, Temperature)
{
  VerifyTemperature(vcTemperature_Celcius, 0.0, vcTemperature_Farenheit, 32.0);
  VerifyTemperature(vcTemperature_Celcius, 0.0, vcTemperature_Kelvin, 273.15);

  VerifyTemperature(vcTemperature_Celcius, 100.0, vcTemperature_Farenheit, 212.0);
  VerifyTemperature(vcTemperature_Celcius, 100.0, vcTemperature_Kelvin, 373.15);

  // Round trips
  for (int i = 0; i < vcTemperature_Count; ++i)
    EXPECT_DOUBLE_EQ(10000.0, vcUnitConversion_ConvertTemperature(vcUnitConversion_ConvertTemperature(10000.0, vcTemperature_Celcius, (vcTemperatureUnit)i), (vcTemperatureUnit)i, vcTemperature_Celcius));
}

void VerifyTimeReference(vcTimeReference sourceType, double in, vcTimeReference destType, double out)
{
  vcTimeReferenceData inData, outData;

  inData.seconds = in;
  outData = vcUnitConversion_ConvertTimeReference(inData, sourceType, destType);
  EXPECT_TRUE(outData.success) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(outData.seconds, out, 0.0001) << "fwd-" << sourceType << " to " << destType;

  inData.seconds = out;
  outData = vcUnitConversion_ConvertTimeReference(inData, destType, sourceType);
  EXPECT_TRUE(outData.success) << "back-" << sourceType << " to " << destType;
  EXPECT_NEAR(outData.seconds, in, 0.0001) << "back-" << sourceType << " to " << destType;
}

void VerifyTimeReferenceWeek(double seconds, unsigned weeks, double out)
{
  vcTimeReferenceData inData, outData;

  inData.GPSWeek.secondsOfTheWeek = seconds;
  inData.GPSWeek.weeks =weeks;
  outData = vcUnitConversion_ConvertTimeReference(inData, vcTimeReference_GPSWeek, vcTimeReference_TAI);
  EXPECT_TRUE(outData.success) << "fwd-" << vcTimeReference_GPSWeek << " to " << vcTimeReference_TAI;
  EXPECT_NEAR(outData.seconds, out, 0.0001) << "fwd-" << vcTimeReference_GPSWeek << " to " << vcTimeReference_TAI;

  inData.seconds = out;
  outData = vcUnitConversion_ConvertTimeReference(inData, vcTimeReference_TAI, vcTimeReference_GPSWeek);
  EXPECT_TRUE(outData.success) << "back-" << vcTimeReference_GPSWeek << " to " << vcTimeReference_TAI;
  EXPECT_EQ(outData.GPSWeek.weeks, weeks) << "back-" << vcTimeReference_GPSWeek << " to " << vcTimeReference_TAI;;
  EXPECT_EQ(outData.GPSWeek.secondsOfTheWeek, seconds) << "back-" << vcTimeReference_GPSWeek << " to " << vcTimeReference_TAI;;
}

void VerifyTimeReferenceUTC(double seconds, vcTimeReference reference, const vcTimeReferenceData &UTC)
{
  vcTimeReferenceData inData, outData;
  inData.seconds = seconds;

  outData = vcUnitConversion_ConvertTimeReference(inData, reference, vcTimeReference_UTC);
  EXPECT_TRUE(outData.success) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_EQ(outData.UTC.year, UTC.UTC.year) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_EQ(outData.UTC.month, UTC.UTC.month) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_EQ(outData.UTC.day, UTC.UTC.day) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_EQ(outData.UTC.hour, UTC.UTC.hour) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_EQ(outData.UTC.minute, UTC.UTC.minute) << "fwd-" << reference << " to " << vcTimeReference_UTC;
  EXPECT_NEAR(outData.UTC.seconds, UTC.UTC.seconds, 0.0001) << "fwd-" << reference << " to " << vcTimeReference_UTC;

  outData = vcUnitConversion_ConvertTimeReference(UTC, vcTimeReference_UTC, reference);
  EXPECT_TRUE(outData.success) << "back-" << vcTimeReference_UTC << " to " << reference;
  EXPECT_NEAR(outData.seconds, seconds, 0.0001) << "back-" << vcTimeReference_UTC << " to " << reference;
}

TEST(UnitConversion, TimeReference)
{
  static double const s_seconds_TAI_Unix_epoch = 378691200.0;
  static double const s_seconds_TAI_GPS_epoch  = 694656009.0;
  static double const s_weekSeconds = 60.0 * 60.0 * 24.0 * 7.0;

  vcTimeReferenceData in, out;

  VerifyTimeReference(vcTimeReference_TAI, 0.0, vcTimeReference_TAI, 0.0);

  //TAI, Unix
  in.seconds = 0.0;
  out = vcUnitConversion_ConvertTimeReference(in, vcTimeReference_TAI, vcTimeReference_Unix);
  EXPECT_FALSE(out.success);

  in.seconds = s_seconds_TAI_Unix_epoch - 1.0;
  out = vcUnitConversion_ConvertTimeReference(in, vcTimeReference_TAI, vcTimeReference_Unix);
  EXPECT_FALSE(out.success);

  VerifyTimeReference(vcTimeReference_TAI, 378691200.0, vcTimeReference_Unix, 0.0);
  VerifyTimeReference(vcTimeReference_TAI, s_seconds_TAI_GPS_epoch, vcTimeReference_Unix, s_seconds_TAI_GPS_epoch - s_seconds_TAI_Unix_epoch - 9.0);

  //TAI, GPS
  VerifyTimeReference(vcTimeReference_TAI, 0.0, vcTimeReference_GPS, -s_seconds_TAI_GPS_epoch);

  //TAI, GPSAdjusted
  VerifyTimeReference(vcTimeReference_TAI, 0.0, vcTimeReference_GPSAdjusted, -s_seconds_TAI_GPS_epoch - 1.0e9);

  //TAI, GPSWeek
  VerifyTimeReferenceWeek(0.0, 0, s_seconds_TAI_GPS_epoch);
  VerifyTimeReferenceWeek(0.0, 42, s_seconds_TAI_GPS_epoch + 42.0 * s_weekSeconds);
  VerifyTimeReferenceWeek(1234.0, 42, s_seconds_TAI_GPS_epoch + 42.0 * s_weekSeconds + 1234.0);

  in.GPSWeek.secondsOfTheWeek = -42.0;
  in.GPSWeek.weeks = 0;
  out = vcUnitConversion_ConvertTimeReference(in, vcTimeReference_GPSWeek, vcTimeReference_TAI);
  EXPECT_FALSE(out.success);

  //Unix, UTC
  vcTimeReferenceData utcData;
  vcUnitConversion_SetUTC(&utcData, 1970, 1, 1, 0, 0, 0.0);
  VerifyTimeReferenceUTC(0.0, vcTimeReference_Unix, utcData);

  vcUnitConversion_SetUTC(&utcData, 1970, 1, 1, 2, 42, 34.567);
  VerifyTimeReferenceUTC(3600.0 * 2 + 42.0 * 60 + 34.567, vcTimeReference_Unix, utcData);

  vcUnitConversion_SetUTC(&utcData, 2020, 7, 3, 0, 4, 13.7);
  VerifyTimeReferenceUTC(1593734653.7, vcTimeReference_Unix, utcData); //Retrieved from https: //www.epochconverter.com
}

void VerifyAngle(vcAngleUnit sourceType, double in, vcAngleUnit destType, double out)
{
  EXPECT_NEAR(out, vcUnitConversion_ConvertAngle(in, sourceType, destType), 0.00001) << "fwd-" << sourceType << " to " << destType;
  EXPECT_NEAR(in, vcUnitConversion_ConvertAngle(out, destType, sourceType), 0.00001) << "back-" << sourceType << " to " << destType;
}

TEST(UnitConversion, AngleUnit)
{
  double oneDegInRad = 1.0 / 180.0 * UD_PI;
  double oneDegInGrad = 20.0 / 18.0;
  double oneRadInGrad = 200.0 / UD_PI;

  VerifyAngle(vcAngle_Degree, 1, vcAngle_Radian, oneDegInRad);
  VerifyAngle(vcAngle_Degree, 1, vcAngle_Gradian, oneDegInGrad);
  VerifyAngle(vcAngle_Radian, 1, vcAngle_Gradian, oneRadInGrad);

  double val = 42;
  val = vcUnitConversion_ConvertAngle(val, vcAngle_Degree, vcAngle_Radian);
  val = vcUnitConversion_ConvertAngle(val, vcAngle_Radian, vcAngle_Gradian);
  val = vcUnitConversion_ConvertAngle(val, vcAngle_Gradian, vcAngle_Degree);
  EXPECT_NEAR(val, 42.0, 0.000001);
}

TEST(UnitConversion, StringFormating)
{
  double value = 42.0;
  const size_t bufSze = 64;
  char buffer[bufSze] = {};
  vcTimeReferenceData timeData;

  {
    timeData.GPSWeek.weeks = 42;
    timeData.GPSWeek.secondsOfTheWeek = 13.0;
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPSWeek) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42 weeks, 13.000000s"), 0);

    vcUnitConversion_SetUTC(&timeData, 2020, 7, 3, 15, 29, 42);
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_UTC) != -1);
    EXPECT_EQ(udStrcmp(buffer, "2020-07-03T15:29:42Z"), 0);

    timeData.seconds= 42.0;
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_TAI) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_Unix) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPS) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPSAdjusted) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000s"), 0);

    timeData.GPSWeek.weeks = 42;
    timeData.GPSWeek.secondsOfTheWeek = 13.0;
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPSWeek, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42 weeks, 13s"), 0);

    vcUnitConversion_SetUTC(&timeData, 2020, 7, 3, 15, 29, 42);
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_UTC, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "2020-07-03T15:29:42Z"), 0);

    timeData.seconds= 42.0;
    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_TAI, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_Unix, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPS, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTimeToString(buffer, bufSze, timeData, vcTimeReference_GPSAdjusted, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42s"), 0);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Metres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000m"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Kilometres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000km"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Centimetres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000cm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Millimetres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000mm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyFeet) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000ft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyMiles) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000mi"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyInches) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000in"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_NauticalMiles) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000nmi"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Metres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42m"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Kilometres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42km"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Centimetres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42cm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Millimetres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42mm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyFeet, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42ft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyMiles, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42mi"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_USSurveyInches, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42in"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_NauticalMiles, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42nmi"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertDistanceToString(buffer, bufSze, value, vcDistance_Count), -1);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareMetres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000sqm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareKilometres) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000sqkm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_Hectare) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000ha"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareFoot) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000sqft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareMiles) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000sqmi"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_Acre) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000ac"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareMetres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42sqm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareKilometres, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42sqkm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_Hectare, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42ha"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareFoot, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42sqft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_SquareMiles, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42sqmi"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_Acre, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42ac"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAreaToString(buffer, bufSze, value, vcArea_Count), -1);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_CubicMetre) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000cbm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_MegaLitre) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000ML"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_Litre) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000L"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicInch) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000cbin"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicFoot) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000cbft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USGallons) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000gal US"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USQuart) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000qt US"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicYard) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000cbyd"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_CubicMetre, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42cbm"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_MegaLitre, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42ML"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_Litre, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42L"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicInch, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42cbin"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicFoot, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42cbft"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USGallons, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42gal US"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USQuart, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42qt US"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_USSurveyCubicYard, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42cbyd"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertVolumeToString(buffer, bufSze, value, vcVolume_Count), -1);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_MetresPerSecond) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000m/s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_KilometresPerHour) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000km/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_USSurveyMilesPerHour) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000mi/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_FeetPerSecond) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000ft/s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_NauticalMilesPerHour) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000nmi/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_Mach) != -1);
    EXPECT_EQ(udStrcmp(buffer, "Mach 42.000000"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_MetresPerSecond, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42m/s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_KilometresPerHour, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42km/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_USSurveyMilesPerHour, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42mi/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_FeetPerSecond, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42ft/s"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_NauticalMilesPerHour, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42nmi/h"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_Mach, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "Mach 42"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertSpeedToString(buffer, bufSze, value, vcSpeed_Count), -1);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Celcius) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000C"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Kelvin) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000K"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Farenheit) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000F"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Celcius, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42C"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Kelvin, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42K"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Farenheit, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42F"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertTemperatureToString(buffer, bufSze, value, vcTemperature_Count), -1);
  }

  {
    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Degree) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000deg"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Radian) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000rad"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Gradian) != -1);
    EXPECT_EQ(udStrcmp(buffer, "42.000000grad"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Degree, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42deg"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Radian, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42rad"), 0);

    EXPECT_TRUE(vcUnitConversion_ConvertAngleToString(buffer, bufSze, value, vcAngle_Gradian, "%0.0f") != -1);
    EXPECT_EQ(udStrcmp(buffer, "42grad"), 0);
  }
}

TEST(UnitConversion, ConvertAndStringifyMetric)
{
  const size_t bufSze = 64;
  char buffer[bufSze] = {};
  double metricVal = 5.732987423;

  vcUnitConversionData conversionData;

  vcUnitConversion_SetMetric(&conversionData);
  {
    vcTimeReferenceData timeData;
    timeData.GPSWeek.weeks = 0;
    timeData.GPSWeek.secondsOfTheWeek = 0.0;
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTimeReference(buffer, bufSze, timeData, vcTimeReference_GPSWeek, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "1980-01-06T00:00:00Z"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal, vcDistance_Millimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 10.0, vcDistance_Millimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733cm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 10.0 * 100.0, vcDistance_Millimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 10.0 * 100.0 * 1000.0, vcDistance_Millimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733km"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 10.0, vcDistance_Centimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal, vcDistance_Centimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733cm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 100.0, vcDistance_Centimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 100.0 * 1000.0, vcDistance_Centimetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733km"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 100.0 / 10.0, vcDistance_Metres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 100.0, vcDistance_Metres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733cm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal, vcDistance_Metres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal * 1000.0, vcDistance_Metres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733km"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 1000.0 / 100.0 / 10.0, vcDistance_Kilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 1000.0 / 100.0, vcDistance_Kilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733cm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal / 1000.0, vcDistance_Kilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, metricVal, vcDistance_Kilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733km"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, metricVal, vcArea_SquareMetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, metricVal * 1000.0 * 1000.0, vcArea_SquareMetres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqkm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, metricVal / 1000.0 / 1000.0, vcArea_SquareKilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqm"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, metricVal, vcArea_SquareKilometres, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqkm"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatVolume(buffer, bufSze, metricVal, vcVolume_Litre, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733L"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatVolume(buffer, bufSze, metricVal * 1000.0 * 1000.0, vcVolume_Litre, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ML"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatVolume(buffer, bufSze, metricVal / 1000.0 / 1000.0, vcVolume_MegaLitre, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733L"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatVolume(buffer, bufSze, metricVal, vcVolume_MegaLitre, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ML"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatSpeed(buffer, bufSze, metricVal, vcSpeed_MetresPerSecond, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m/s"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatSpeed(buffer, bufSze, metricVal / 1000.0 * 3600.0, vcSpeed_KilometresPerHour, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733m/s"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTemperature(buffer, bufSze, metricVal, vcTemperature_Celcius, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733C"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTemperature(buffer, bufSze, metricVal + 273.15, vcTemperature_Kelvin, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733C"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatAngle(buffer, bufSze, metricVal, vcAngle_Degree, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.73deg"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatAngle(buffer, bufSze, metricVal /180.0 * UD_PI, vcAngle_Radian, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.73deg"), 0);
  }
}

TEST(UnitConversion, ConvertAndStringifyImperial)
{
  const size_t bufSze = 64;
  char buffer[bufSze] = {};
  double imperialVal = 5.732987423;

  vcUnitConversionData conversionData;

  vcUnitConversion_SetUSSurvey(&conversionData);
  {
    vcTimeReferenceData timeData;
    timeData.GPSWeek.weeks = 0;
    timeData.GPSWeek.secondsOfTheWeek = 0.0;
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTimeReference(buffer, bufSze, timeData, vcTimeReference_GPSWeek, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "1980-01-06T00:00:00Z"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal, vcDistance_USSurveyInches, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733in"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal * 12.0, vcDistance_USSurveyInches, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ft"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal * 12.0 * 5280.0, vcDistance_USSurveyInches, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mi"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal / 12.0, vcDistance_USSurveyFeet, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733in"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal, vcDistance_USSurveyFeet, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ft"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal * 5280.0, vcDistance_USSurveyFeet, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mi"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal / 12.0 / 5280.0, vcDistance_USSurveyMiles, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733in"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal / 5280.0, vcDistance_USSurveyMiles, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ft"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatDistance(buffer, bufSze, imperialVal, vcDistance_USSurveyMiles, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733mi"), 0);

  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, imperialVal, vcArea_SquareFoot, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqft"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, imperialVal * 5280.0 * 5280.0, vcArea_SquareFoot, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqmi"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, imperialVal / 5280.0 / 5280.0, vcArea_SquareMiles, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqft"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatArea(buffer, bufSze, imperialVal, vcArea_SquareMiles, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733sqmi"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatVolume(buffer, bufSze, imperialVal, vcVolume_USGallons, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733gal US"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatSpeed(buffer, bufSze, imperialVal, vcSpeed_FeetPerSecond, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ft/s"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatSpeed(buffer, bufSze, imperialVal / 5280.0 * 3600.0, vcSpeed_USSurveyMilesPerHour, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733ft/s"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTemperature(buffer, bufSze, imperialVal, vcTemperature_Farenheit, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733F"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatTemperature(buffer, bufSze, (imperialVal - 32.0) * 5.0/9.0 + 273.15, vcTemperature_Kelvin, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.733F"), 0);
  }

  {
    EXPECT_EQ(vcUnitConversion_ConvertAndFormatAngle(buffer, bufSze, imperialVal, vcAngle_Degree, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.73deg"), 0);

    EXPECT_EQ(vcUnitConversion_ConvertAndFormatAngle(buffer, bufSze, imperialVal / 180.0 * UD_PI, vcAngle_Radian, &conversionData), udR_Success);
    EXPECT_EQ(udStrcmp(buffer, "5.73deg"), 0);
  }
}
