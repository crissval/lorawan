/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Driver LIS3DH for ConnecSenS Device ********************/
/* Datasheet : http://bit.ly/2maWpQI                      */
/*             http://bit.ly/2h9IrNA                      */
/**********************************************************/
#pragma once

#include "sensor_internal.hpp"


class LIS3DH : public SensorInternal
{
private:
  /**
   * Define the scale values.
   */
  typedef enum Scale
  {
    SCALE_2G,
    SCALE_4G,
    SCALE_8G,
    SCALE_16G
  }
  Scale;


public :
  LIS3DH();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool openSpecific();
  void closeSpecific();
  bool readSpecific();
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);

  bool iamLIS3DH();
  void readIntRegister();
  float convertValueToAcceleration(uint16_t v);


public:
  static const char *TYPE_NAME;

private:
  Scale    _scale;    ///< The sensing scale 0 for +/-2g, 1 for 1 for +/-4g, 2 for
  float    _x_accel;
  float    _y_accel;
  float    _z_accel;
  bool     _motionDetection;
  uint16_t _alarmThresholdMg; ///< The alarm threshold in mg
  I2C_HandleTypeDef hi2c;

  static const uint8_t _ctrlNoAlarm[8];
  static const uint8_t _ctrlAlarm[8];
  static const uint8_t _scaleToTHSStep[4];

  static const char   *_CSV_HEADER_VALUES[];
};

