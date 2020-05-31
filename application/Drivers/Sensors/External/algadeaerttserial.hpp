/*
 * Driver for the Alagade AER TT radon sensor with serial interface.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef SENSORS_EXTERNAL_ALGADEAERTTSERIAL_HPP_
#define SENSORS_EXTERNAL_ALGADEAERTTSERIAL_HPP_

#include "config.h"
#ifdef USE_SENSOR_ALGADE_AERTT_SERIAL
#include "sensor_uart.hpp"


#define ALGADEAERTTSERIAL_UART_MSG_TIME_LEN        11
#define ALGADEAERTTSERIAL_UART_MSG_TIME_LEN_MAX    ALGADEAERTTSERIAL_UART_MSG_TIME_LEN
#define ALGADEAERTTSERIAL_UART_MSG_BQM3_LEN_MAX    8   // I actually don't know what is the maximum value we can get so I saw big
#define ALGADEAERTTSERIAL_UART_MSG_TEMP_LEN_MAX    4
#define ALGADEAERTTSERIAL_UART_MSG_HUMI_LEN_MAX    2
#define ALGADEAERTTSERIAL_UART_MSG_STATUS_LEN      1
#define ALGADEAERTTSERIAL_UART_MSG_STATUS_LEN_MAX  ALGADEAERTTSERIAL_UART_MSG_STATUS_LEN
#define ALGADEAERTTSERIAL_UART_MSG_BATTV_LEN_MAX   4
#define ALGADEAERTTSERIAL_UART_MSG_SEP_COUNT       5
#define ALGADEAERTTSERIAL_UART_MSG_LEN_MAX  ( \
  ALGADEAERTTSERIAL_UART_MSG_TIME_LEN_MAX   + \
  ALGADEAERTTSERIAL_UART_MSG_BQM3_LEN_MAX   + \
  ALGADEAERTTSERIAL_UART_MSG_TEMP_LEN_MAX   + \
  ALGADEAERTTSERIAL_UART_MSG_HUMI_LEN_MAX   + \
  ALGADEAERTTSERIAL_UART_MSG_STATUS_LEN_MAX + \
  ALGADEAERTTSERIAL_UART_MSG_BATTV_LEN_MAX  + \
  ALGADEAERTTSERIAL_UART_MSG_SEP_COUNT)


class AlgadeAERTTSerial : public SensorUART
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


public:
  AlgadeAERTTSerial();
  ~AlgadeAERTTSerial();
  static Sensor *getNewInstance();
  const char    *type();

  bool process();


private:
  bool   openSpecific();
  bool   jsonAlarmSpecific(const JsonObject&       json,
			   CNSSInt::Interruptions *pvAlarmInts);
  void   clearAlarms();
  State *defaultState(uint32_t *pu32_size);
  void   clearReadings();
  bool   readSpecific();
  void   updateAlarmStatus();
  bool   writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);

  void receivedData(  USART *pv_usart, uint8_t *pu8_data, uint32_t size);
  bool parseDataFrame(           const uint8_t *pu8_data, uint32_t size);


private:
  /// Where the data received from the sensor are written to. This is a working buffer.
  uint8_t  _workingBuffer[ALGADEAERTTSERIAL_UART_MSG_LEN_MAX];
  /// Where the complete received message is saved to.
  uint8_t  _dataBuffer[   ALGADEAERTTSERIAL_UART_MSG_LEN_MAX];
  uint32_t _nbDataBytes;            ///< The number of data bytes in _dataBuffer.
  uint32_t _radioactivityBqm3;      ///< The air radioactivity, in Becquerels per cubic meter.
  float    _airTemperatureDegC;     ///< The air temperature, in Celsius degrees.
  uint8_t  _airRelHumidityPercent;  ///< The air relative air humidity, in percents.
  float    _voltageV;               ///< The sensor's power voltage, in Volts.
  bool     _battIsLow;              ///< Indicate if the battery voltage is low or not.
  bool     _heaterIsOn;             ///< Indicate if the heater is on or not.
  State    _state;                  ///< The state object.
  uint32_t _alarmLowSetBqm3;        ///< The radioactivty low alarm set threshold.
  uint32_t _alarmLowClearBqm3;      ///< The radioactivty low alarm clear threshold.
  uint32_t _alarmHighSetBqm3;       ///< The radioactivty high alarm set threshold.
  uint32_t _alarmHighClearBqm3;     ///< The radioactivty high alarm clear threshold.
  bool     _alarmIsSet;             ///< Indicate if an alarm has been set or not.
  bool     _hasDataToProcess;       ///< Indicate if we have received data to process.

  static const char   *_CSV_HEADER_VALUES[];
};


#endif // USE_SENSOR_ALGADE_AERTT_SERIAL
#endif /* SENSORS_EXTERNAL_ALGADEAERTTSERIAL_HPP_ */
