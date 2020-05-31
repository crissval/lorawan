/*
 * Driver for the soil humidity sensor made by Laure
 *
 *  Created on: 9 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_EXTERNAL_SOILMOISTUREWATERMARKI2C_HPP_
#define SENSORS_EXTERNAL_SOILMOISTUREWATERMARKI2C_HPP_

#include "config.h"
#ifdef USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C
#include "sensor_i2c.hpp"


#define SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX  3


class SoilMoistureWatermakI2C : public SensorI2C
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


private:
  /**
   * Structure used to store a sensor's data
   */
  typedef struct SensorData
  {
    uint16_t depthCm;          ///< The measurement depth, in centimeters.
    uint8_t  centibars;        ///< The soil water tension, in centibars.
    float    tempDegC;         ///< The soil temperature, in Celcius degrees.
    uint32_t freqHz;           ///< The frequency returned by the sensor, in Hz.

    bool     alarmLowActive;   ///< Is the low alarm currently active?
    bool     alarmHighActive;  ///< Is the high alarm currently active?
  }
  SensorData;

  /**
   * Defines the data type stored in the sensor's state file
   */
  typedef struct StateSpecific
  {
    State   base;  ///< The base object from Sensor.

    uint8_t alarmLowSet  : SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX;  ///< are the low alarms set?
    uint8_t alarmHighSet : SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX;  ///< are the high alarms set?
  }
  StateSpecific;


public:
  SoilMoistureWatermakI2C();
  virtual ~SoilMoistureWatermakI2C();
  static Sensor *getNewInstance();
  const char    *type();

  bool readOnAlarmChange();
  bool readSpecific();

private:
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  bool jsonSpecificHandler(               const JsonObject&       json);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  void   clearAlarm();
  State *defaultState(uint32_t *pu32_size);


private:
  SensorData    _sensorData[SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX];  ///< The sensor's data
  bool          _alarmIsSet;        ///< Is there an alarm threshold set?
  uint8_t       _alarmLowSetCb;     ///< Alarm low threshold set value, in centibars.
  uint8_t       _alarmLowClearCb;   ///< Alarm low threshold clear value, in centibars.
  uint8_t       _alarmHighSetCb;    ///< Alarm high threshold set value, in centibars.
  uint8_t       _alarmHighClearCb;  ///< Alarm high threshold clear value, in centibars.
  StateSpecific _state;             ///< The state object.

  static const char *_CSV_HEADER_VALUES[];
};


#endif // USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C
#endif /* SENSORS_EXTERNAL_SOILMOISTUREWATERMARKI2C_HPP_ */
