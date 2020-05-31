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
/* Driver OPT3001 for ConnecSenS Device *******************/
/* Datasheet : http://bit.ly/1VC5aAC                      */
/**********************************************************/
#pragma once

#include "sensor_internal.hpp"


class OPT3001 : public SensorInternal
{
public:
  static const char *TYPE_NAME;


public :
  OPT3001();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool openSpecific();
  bool readSpecific();
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  void processAlarmInterruption(          CNSSInt::Interruptions  ints);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);

  bool     currentValuesAreInAlarmRange();
  void     clearAlarmParameters();
  bool     configureNextAlarm(float valueLux);
  bool     iAmOPT3001();
  float    rawToLux(uint16_t raw);
  uint16_t luxToRaw(float    lux);
  State *  defaultState(uint32_t *pu32_size);


private:
  float     _illuminanceLux;      ///< The sensor's last reading, in Lux.
  uint16_t  _alarmThLowSetRaw;    ///< The low alarm threshold that sets the alarm, in raw units.
  uint16_t  _alarmThLowClearRaw;  ///< The low alarm threshold that clears the alarm, in raw units.
  uint16_t  _alarmThHighSetRaw;   ///< The high alarm threshold that sets the alarm, in raw units.
  uint16_t  _alarmThHighClearRaw; ///< The high alarm threshold that clears the alarm, in raw units.
  bool      _useAlarm;            ///< Do we use the alarm functionality or not?
  bool      _useLowAlarm;         ///< Do we have low alarm parameters set?
  bool      _useHighAlarm;        ///< Do we have high alarm parameters set?
  uint32_t  _regCFGBase;          ///< The base value for the OPT3001's configuration register.
  State     _state;               ///< The sensor's state object.

  static const char *_CSV_HEADER_VALUES[];
};
