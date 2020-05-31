/*
 * Sensor class for rain gauges that use a contact to count rain amounts,
 * like tipping buckets rain gauges.
 *
 *  Created on: 29 mai 2018
 *      Author: Jérôme FUCHET
 */
#ifndef SENSORS_EXTERNAL_RAINGAUGECONTACT_HPP_
#define SENSORS_EXTERNAL_RAINGAUGECONTACT_HPP_

#include "config.h"
#ifdef USE_SENSOR_RAIN_GAUGE_CONTACT
#include "sensor.hpp"

class RainGaugeContact : public Sensor
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


private:
  /**
   * Defines the data type stored in the sensor's state file
   */
  typedef struct StateSpecific
  {
    State    base;        ///< The state base.

    ts2000_t tsLastRead;  ///< Timestamp of the last sensor's read.
    ts2000_t tsLastTick;  ///< Timestamp of the last tick.
    uint32_t nbTicks;     ///< The number of ticks since last read.
  }
  StateSpecific;


public:
  RainGaugeContact();
  static Sensor *getNewInstance();
  const char    *type();


private:
  State *defaultState(uint32_t *pu32_size);
  bool   checkIfIsInAlarmRange(StateSpecific& state, ts2000_t tsNow = 0);
  bool   updateReadings( const StateSpecific& state, ts2000_t tsNow = 0);

  bool openSpecific();
  void closeSpecific();
  bool readSpecific();
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  bool jsonSpecificHandler(               const JsonObject&       json);
  void processInterruptionSpecific(       CNSSInt::Interruptions  ints);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  float            _mmPerTick;          ///< The number of rain millimeters to count for each contact tick.
  float            _readingMM;          ///< The current reading, in millimeters of rain.
  uint32_t         _readingDurationSec; ///< The amount of time, in seconds, the rain amount corresponds to.
  CNSSInt::IntFlag _tickIntFlag;        ///< The tick interruption flag.

  bool  _hasAlarmSet;                ///< Indicate if an alarm is configured or not.
  float _thresholdSetMMPerMinute;    ///< The alarm threshold set rain intensity in millimeters per minute. Set to 0.0 if no threshold.
  float _thresholdClearMMPerMinute;  ///< The alarm threshold clear rain intensity in millimeters per minute. Set to 0.0 if no threshold.

  StateSpecific _state;  ///< The sensor's state.

  static const char   *_CSV_HEADER_VALUES[];
};

#endif // USE_SENSOR_RAIN_GAUGE_CONTACT
#endif /* SENSORS_EXTERNAL_RAINGAUGECONTACT_HPP_ */
