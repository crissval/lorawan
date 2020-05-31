/*
 * Sensor class for rain gauges that use a contact to count rain amounts,
 * like tipping buckets rain gauges.
 *
 *  Created on: 29 mai 2018
 *      Author: Jérôme FUCHET
 */
#include "raingaugecontact.hpp"
#ifdef USE_SENSOR_RAIN_GAUGE_CONTACT
#include "sdcard.h"
#include "rtc.h"
#include "cnssrf-dt_rain.h"


#define STATE_VERSION  1


  CREATE_LOGGER(raingaugecontact);
#undef  logger
#define logger  raingaugecontact


const char *RainGaugeContact::TYPE_NAME = "RainGaugeContact";

const char *RainGaugeContact::_CSV_HEADER_VALUES[] =
{
    "rainMM", "rainPeriodSec", NULL
};


RainGaugeContact::RainGaugeContact() : Sensor(POWER_EXTERNAL_INT, POWER_EXTERNAL_INT)
{
  this->_mmPerTick          = 0.0;
  this->_readingMM          = 0.0;
  this->_readingDurationSec = 0;
  this->_tickIntFlag        = CNSSInt::INT_FLAG_NONE;

  this->_hasAlarmSet               = false;
  this->_thresholdSetMMPerMinute   = 0.0;
  this->_thresholdClearMMPerMinute = 0.0;
}

Sensor *RainGaugeContact::getNewInstance()
{
  return new RainGaugeContact();
}

const char *RainGaugeContact::type()
{
  return TYPE_NAME;
}


Sensor::State *RainGaugeContact::defaultState(uint32_t *pu32_size)
{
  ts2000_t tsNow = rtc_get_date_as_secs_since_2000();

  this->_state.base.version = STATE_VERSION;
  this->_state.tsLastRead   = this->_state.tsLastTick = tsNow;
  this->_state.nbTicks      = 0;

  *pu32_size = sizeof(this->_state);
  return &this->_state.base;
}

/**
 * Check if the current amount of rain is in the alarm range or not.
 *
 * @param[in,out] state the state object containing the informations we need.
 *                      The state object is updated to match the current alarm status.
 * @param[in] tsNow     the timestamp for now. If set to 0 the get it from the RTC.
 *
 * @return true  if the current rain amount or rate is in the alarm range.
 * @return false otherwise.
 */
bool RainGaugeContact::checkIfIsInAlarmRange(StateSpecific &state, ts2000_t tsNow)
{
  bool alarm;

  if(!this->_hasAlarmSet) { return false; }

  // Get now timestamp if need and check it against last tick.
  if(!tsNow) { tsNow =  rtc_get_date_as_secs_since_2000(); }
  if(tsNow <= state.tsLastTick)
  {
    // Well keep the current state
    return state.base.isInAlarm;
  }

  // Compute rain rate and compare it to the alarm thresholds
  float   rateMMPerMinute = (this->_mmPerTick * 60) / (tsNow - state.tsLastTick);
  alarm = rateMMPerMinute >  this->_thresholdSetMMPerMinute ||
      (   rateMMPerMinute >  this->_thresholdClearMMPerMinute && state.base.isInAlarm);

  setIsInAlarm(alarm);
  return alarm;
}


bool RainGaugeContact::openSpecific()
{
  // Do nothing
  return true;
}

void RainGaugeContact::closeSpecific()
{
  // Do nothing
}

bool RainGaugeContact::readSpecific()
{
  // Get the current timestamp
  ts2000_t tsNow = rtc_get_date_as_secs_since_2000();

  // Get the sensor's state
  StateSpecific *pvState;
  if(!(pvState = (StateSpecific *)state())) { return false; }

  // Update the reading values and the alarm
  bool res;
  if((res = updateReadings(*pvState, tsNow)))
  {
    checkIfIsInAlarmRange( *pvState, tsNow);
  }

  // Update the state object
  pvState->tsLastRead  = tsNow;
  pvState->nbTicks     = 0;
  saveState();

  return res;
}

/**
 * Update the reading values using a State object contents.
 *
 * @param[in] state the State object to use.
 * @param[in] tsNow the timestamp for now. If set to 0 then we'll get it from RTC.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool RainGaugeContact::updateReadings(const StateSpecific& state, ts2000_t tsNow)
{
  bool res = true;
  if(state.tsLastRead == tsNow)
  {
    // Well there is nothing to read
    setHasNewData(false);
    return true;
  }
  if(state.tsLastRead > tsNow)
  {
    // The timestamp is in the future; not good.
    log_error_sensor(logger, "Timestamp from sensor's state file is in the future.");
    res = false;
  }
  else { this->_readingDurationSec = tsNow - state.tsLastRead; }

  // Get the amount of rain
  this->_readingMM  = state.nbTicks * this->_mmPerTick;

  setHasNewData(res);
  return res;
}


bool RainGaugeContact::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  float thSet   = 0.0;
  float thClear = 0.0;
  if(json["thresholdSetMMPerMinute"]  .success()) { thSet   = json["thresholdSetMMPerMinute"];                  }
  if(json["thresholdSetMMPerHour"]    .success()) { thSet   = json["thresholdSetMMPerHour"];   thSet   /= 60.0; }
  if(json["thresholdClearMMPerMinute"].success()) { thClear = json["thresholdClearMMPerMinute"];                }
  if(json["thresholdClearMMPerHour"]  .success()) { thClear = json["thresholdClearMMPerHour"]; thClear /= 60.0; }
  if((thSet && !thClear) || (thClear && !thSet) || thSet < 0 || thClear < 0)
  {
    log_error_sensor(logger, "To set up an alarm, parameters 'thresholdSetMMPerMinute' and 'thresholdClearMMPerMinute' are mandatory.");
    return false;
  }
  this->_thresholdSetMMPerMinute   = thSet;
  this->_thresholdClearMMPerMinute = thClear;
  this->_hasAlarmSet               = this->_thresholdSetMMPerMinute && this->_thresholdClearMMPerMinute;

  *pvAlarmInts = CNSSInt::INT_FLAG_NONE;
  return true;
}

bool RainGaugeContact::jsonSpecificHandler(const JsonObject& json)
{
  if(!json.success())
  {
    // We need a specific part
    log_error_sensor(logger, "This kind of sensor requires a specific configuration; we can't find it.");
    return false;
  }

  // Get the required parameters
  // The tick interruption name
  const char *psTickItName = json["tickInterrupt"].as<const char *>();
  if(!psTickItName || !*psTickItName)
  {
    log_error_sensor(logger, "The sensor specific parameter 'tickInterrupt' is required.");
    return false;
  }
  CNSSInt::IntFlag tickFlag = CNSSInt::getFlagUsingName(psTickItName);
  if(tickFlag == CNSSInt::INT_FLAG_NONE)
  {
    log_error_sensor(logger, "Unknown interrupt '%s' for parameter 'tickInterrupt'.", psTickItName);
    return false;
  }
  if(isTriggerableBy(tickFlag))
  {
    log_error_sensor(logger, "The interruption '%s' set for 'tickInterrupt' cannot be one of these set by 'interruptChannel' or 'interruptChannels'.", psTickItName);
    return false;
  }

  // The debounce time
  if(!json["tickDebounceMs"].success())
  {
    log_error_sensor(logger, "Sensor specific parameter 'tickDebounceMs' is mandatory.");
    return false;
  }
  uint32_t debounceMs = json["tickDebounceMs"].as<uint32_t>();

  // The mm per tick
  if(!json["rainMMPerTick"].success())
  {
    log_error_sensor(logger, "Sensor specific parameter 'rainMMPerTick' is mandatory.");
    return false;
  }
  float mmPerTick = json["rainMMPerTick"].as<float>();
  if(mmPerTick <= 0.0)
  {
    log_error_sensor(logger, "Sensor specific parameter 'rainMMPerTick' is mandatory.");
    return false;
  }

  // We have all the required parameters
  // Set up the object.
  this->_mmPerTick   = mmPerTick;
  this->_tickIntFlag = tickFlag;
  CNSSInt::instance()->setDebounce(CNSSInt::getIdUsingName(psTickItName), debounceMs);
  setSpecificIntSensitivity(this->_tickIntFlag);

  return true;
}

void RainGaugeContact::processInterruptionSpecific(CNSSInt::Interruptions ints)
{
  if(!(ints & this->_tickIntFlag)) { return; }

  // Get the sensor's state
  StateSpecific *pvState;
  if(!(pvState = (StateSpecific *)state()))
  {
    log_error_sensor(logger, "Failed to open sensor's state file.");
    return;
  }

  // Add a tick
  pvState->nbTicks++;

  // Check if we have to set up the alarm
  ts2000_t tsNow = rtc_get_date_as_secs_since_2000();
  checkIfIsInAlarmRange(*pvState, tsNow);
  if(requestToSendData(false))
  {
    // Well, we have to do the reading here since the read() function will not be called.
    if(updateReadings(*pvState, tsNow))
    {
      pvState->tsLastRead = tsNow;
      pvState->nbTicks    = 0;
    }
  }

  // Save the state object
  pvState->tsLastTick = tsNow;
  saveState();
}

bool RainGaugeContact::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_rainamount_write_mm_to_frame(pvFrame,
						this->_readingDurationSec,
						this->_readingMM,
						isInAlarm());
}


const char **RainGaugeContact::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t RainGaugeContact::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.2f%c%u",
		 this->_readingMM, OUTPUT_DATA_CSV_SEP,
		 (unsigned int)this->_readingDurationSec);

  return len >= size ? -1 : (int32_t)len;
}


#endif // USE_SENSOR_RAIN_GAUGE_CONTACT
