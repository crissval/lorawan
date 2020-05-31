/*
 * sensor.cpp
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include <string.h>
#include <stdlib.h>
#include "sensor.hpp"
#include "cnssint.hpp"
#include "connecsens.hpp"
#include "sdcard.h"
#include "murmur3.h"
#include "cnssrf-dt_hash.h"
#include "board.h"
#include "rtc.h"


#ifndef SENSOR_CNSSRF_WRITE_SENSOR_TYPE_HASH
#define SENSOR_CNSSRF_WRITE_SENSOR_TYPE_HASH  true
#endif


#define SENSOR_STATE_FILENAME_SEP_STR  "-"
#define SENSOR_STATE_FILENAME_EXT      ".state"


  CREATE_LOGGER(sensor);
#undef  logger
#define logger  sensor


/**
 * Constructor.
 */
Sensor::Sensor(Power power, Power powerSleep, Features features)
{
  this->_features               = features;
  this->_name[0]                = '\0';
  this->_psStateFilename        = NULL;
  this->_uniqueId[0]            = '\0';
  this->_firmwareVersion[0]     = '\0';
  this->_periodType             = PERIOD_TYPE_FROM_NODE_REGULAR;
  this->_intSensitivity         = CNSSInt::INT_FLAG_NONE;
  this->_intSensitivityRead     = CNSSInt::INT_FLAG_NONE;
  this->_intSensitivityToSend   = CNSSInt::INT_FLAG_NONE;
  this->_intSensitivityToAlarm  = CNSSInt::INT_FLAG_NONE;
  this->_intSensitivitySpecific = CNSSInt::INT_FLAG_NONE;
  this->_sendOnInterrupt        = false;
  this->_sendOnAlarmSet         = false;
  this->_sendOnAlarmCleared     = false;
  this->_requestSendData        = false;
  this->_power                  = power;
  this->_powerSleep             = powerSleep;
  this->_hasNewData             = false;
  this->_dataChannel            = CNSSRF_DATA_CHANNEL_UNDEFINED;
  this->_isOpened               = false;
  this->_hasAlarmToSet          = false;
  this->_alarmStatusJustChanged = false;
  this->_pvState                = NULL;
  this->_stateSize              = 0;
  this->_pu8StateRef            = NULL;
  this->_periodNormalSec        = 0;
  this->_periodAlarmSec         = 0;
  this->_readingsTs2000         = 0;
  this->_interruptionTs2000     = 0;
  this->_typeHash               = 0;
  this->_writeTypeHashToCNSSRF  = SENSOR_CNSSRF_WRITE_SENSOR_TYPE_HASH;
  this->_typeHashComputed       = false;
  this->_csvNbValues            = 0;
}

/**
 * Destructor
 */
Sensor::~Sensor()
{
  close();
  if(this->_psStateFilename) { delete this->_psStateFilename; this->_psStateFilename = NULL; }
  if(this->_pu8StateRef)     { delete this->_pu8StateRef;     this->_pu8StateRef     = NULL; }
}


bool Sensor::process()
{
  // Default implementation
  // Do nothing
  return false;
}


/**
 * Set the sensor's name.
 *
 * @param[in] psName the name.
 */
void Sensor::setName(const char *psName)
{
  if(!psName || !*psName) { this->_name[0] = '\0'; }
  else
  {
    strncpy(this->_name, psName, SENSOR_NAME_MAX_SIZE);
    this->_name[SENSOR_NAME_MAX_SIZE - 1] = '\0'; // To be sure
  }
}


/**
 * Set the reading period as a regular one and at the node's pace.
 *
 * @param[in] secs the period, in seconds. Can be 0.
 */
void Sensor::setPeriodSec(uint32_t secs)
{
  this->_periodType      = PERIOD_TYPE_FROM_NODE_REGULAR;
  this->_periodNormalSec = secs;
  ClassPeriodic::setPeriodSec(secs);
}

/**
 * The reading period is set by the sensor's output flow.
 */
void Sensor::setPeriodAtSensorSFlow()
{
  this->_periodType      = PERIOD_TYPE_AT_SENSOR_S_FLOW;
  this->_periodNormalSec = this->_periodAlarmSec = 0;
  ClassPeriodic::setPeriodSec(0);
}

/**
 * Indicate if it's time to read the sensor or not.
 *
 * Overwrites the function from the parent class ClassPeriodic.
 */
bool Sensor::itsTime(bool updateNextTime)
{
  return (this->_periodType == PERIOD_TYPE_AT_SENSOR_S_FLOW) ?
      hasNewData() :
      ClassPeriodic::itsTime(updateNextTime);
}

/**
 * Set the sensor's configuration using JSON configuration data.
 *
 * @param[in] json the JSON configuration data.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool Sensor::setConfiguration(const JsonObject& json)
{
  const char *psValue;
  bool        res = true;

  setName(    json["name"]    .as<const char *>());
  setUniqueId(json["uniqueId"].as<const char *>());
  this->_intSensitivityRead = CNSSInt::INT_FLAG_NONE;
  this->_sendOnInterrupt    = false;


  // Set period
  this->_periodNormalSec = ConnecSenS::getPeriodSec(json);
  setPeriodSec(this->_periodNormalSec);
  if(json["period"].success() && (psValue = json["period"].as<const char *>()))
  {
    if(!*psValue)
    {
      log_error(logger, "Parameter 'period' has no value.");
      res = false;
    }
    else if(strcasecmp(psValue, "sensorFlow") == 0)
    {
      if(hasFeature(FEATURE_CAN_FOLLOW_SENSOR_S_PACE)) { setPeriodAtSensorSFlow(); }
      else
      {
	log_error(logger, "The reading pace cannot be set by the sensor's output flow.");
	res = false;
      }
    }
    else
    {
      log_error(logger, "Unknown 'period' parameter value: %s.", psValue);
      res = false;
    }
  }

  if(!hasFeature(FEATURE_CAN_DETECT_FIRMWARE_VERSION))
  {
    setFirmwareVersion(json["firmwareVersion"].as<const char *>());
  }

  if(!*name())
  {
    log_error_sensor(logger, "very sensor must have a name.");
    goto error_exit;
  }
  if(!periodSec() && !hasFeature(FEATURE_CAN_FOLLOW_SENSOR_S_PACE))
  {
    log_error_sensor(logger, "Period cannot be 0.");
    goto error_exit;
  }

  // Set interruption line used
  this->_intSensitivityRead     |= CNSSInt::instance()->registerClient(*this, json["interruptChannel"].as<const char *>());
  if(json["interruptChannels"].success())
  {
    // There is a list of interruption channels
    uint32_t count = json["interruptChannels"].size();
    for(uint32_t i = 0; i < count; i++)
    {
      this->_intSensitivityRead |= CNSSInt::instance()->registerClient(*this, json["interruptChannels"][i].as<const char *>());
    }
  }
  this->_intSensitivityToSend = this->_intSensitivityRead;
  setIntSensitivity(this->_intSensitivityRead, false);

  // Process +5V usage
  if(json["use5VWhenActive"].as<bool>()) { this->_power      |= POWER_EXTERNAL; }
  if(json["use5VWhenAsleep"].as<bool>()) { this->_powerSleep |= POWER_EXTERNAL; }

  // Process Interruption power forced usage.
  if(json["useInt3V3WhenActive"].as<bool>()) { this->_power  |= POWER_EXTERNAL_INT; }

  // Process the alarm part
  if(json["alarm"].success())
  {
    const JsonObject &alarm   = json ["alarm"];
    this->_sendOnAlarmSet     = alarm["sendOnAlarmSet"]    .as<bool>();
    this->_sendOnAlarmCleared = alarm["sendOnAlarmCleared"].as<bool>();
    this->_periodAlarmSec     = ConnecSenS::getPeriodSec(alarm);
    this->_hasAlarmToSet      = jsonAlarmSpecific(alarm, &this->_intSensitivityToAlarm);
    if(this->_intSensitivityToAlarm != CNSSInt::INT_FLAG_NONE)
    {
      CNSSInt::instance()->registerClient(*this, this->_intSensitivityToAlarm);
      setIntSensitivity(this->_intSensitivityToAlarm, true);
    }
  }
  if(this->_periodAlarmSec && state() && this->_pvState->isInAlarm)
  {
    setPeriodSec(this->_periodAlarmSec);
  }

  // Sensor specific initialisation from JSON
  res &= jsonSpecificHandler(json);

  // If the sensor can react to interruption then we still have some stuff to set up.
  if(res && isTriggerable())
  {
    // Do we send data on interruption?
    this->_sendOnInterrupt = json["sendOnInterrupt"].as<bool>();
  }

  return res;

  error_exit:
  return false;
}


/**
 * Return the sensor's type hash.
 *
 * @param[out] pb_ok set to true in case of success, to false otherwise.
 *                   Can be NULL if we don't need this information.
 *
 * @return the hash.
 * @return 0 in case of error.
 */
Sensor::TypeHash Sensor::typeHash(bool *pb_ok)
{
  bool        dummy;
  const char *ps_type;

  if(!pb_ok) { pb_ok = &dummy; }
  *pb_ok = this->_typeHashComputed;

  if(this->_typeHashComputed) { goto exit; }

  ps_type = type();
  if(!ps_type || !*ps_type)
  {
    *pb_ok = false;
    goto exit;
  }

  this->_typeHash = mm3_32_cnss((const uint8_t *)ps_type, strlen(ps_type));
  *pb_ok          = this->_typeHashComputed = true;

  exit:
  return this->_typeHash;
}


/**
 * Return the sensor's unique identifier.
 *
 * @return the identifier.
 * @return an empty string if no identifier is set.
 */
const char *Sensor::uniqueId()
{
  return this->_uniqueId;
}

/**
 * Set the sensor's unique identifier.
 *
 * @param[in] psUID the identifier. Use NULL or an empty string to remove the current identifier.
 */
void Sensor::setUniqueId(const char *psUID)
{
  if(!psUID || !*psUID ||
      strlcpy(this->_uniqueId, psUID, sizeof(this->_uniqueId)) >= sizeof(this->_uniqueId))
  {
    this->_uniqueId[0] = '\0';
  }
}

/**
 * Return the sensor's firmware version.
 *
 * @return the firmware version.
 * @return an empty string if no identifier is set.
 */
const char *Sensor::firmwareVersion()
{
  return this->_firmwareVersion;
}

/**
 * Set the sensor's firmware version.
 *
 * @param[in] psFwv the firmware version Use NULL or an empty string to remove the current version.
 */
void Sensor::setFirmwareVersion(const char *psFwv)
{
  if(!psFwv || !*psFwv ||
      strlcpy(  this->_firmwareVersion, psFwv, sizeof(this->_firmwareVersion))
      >= sizeof(this->_firmwareVersion))
  {
    this->_firmwareVersion[0] = '\0';
  }
}


/**
 * Set the global interruption sensitivity.
 *
 * @param[in] sensitivity the interruption flags to add to the sensitivity.
 * @param[in] append      append to current sensitivity (true) or
 *                        replace current sensitivity with 'sensitivity' (false).
 */
void Sensor::setIntSensitivity(CNSSInt::Interruptions sensitivity, bool append)
{
  this->_intSensitivity = append ? (this->_intSensitivity | sensitivity) : sensitivity;

  // Do we need to activate external interruptions power supply?
  if(CNSSInt::containsExternalInterrupt(this->_intSensitivity))
  {
    this->_power      |= POWER_EXTERNAL_INT;
    this->_powerSleep |= POWER_EXTERNAL_INT;
  }

  // Do we need to activate internal interruptions power supply?
  if(CNSSInt::containsInternalInterrupt(this->_intSensitivity))
  {
    this->_power      |= POWER_INTERNAL;
    this->_powerSleep |= POWER_INTERNAL;
  }

  CNSSInt::instance()->registerClient(*this, sensitivity, true);
}

/**
 * Set the sensor's specific interruption sensitivity.
 *
 * @param[in] sensitivity  the interruption flags the specific part is sensitive to.
 */
void Sensor::setSpecificIntSensitivity(CNSSInt::Interruptions sensitivity)
{
  this->_intSensitivitySpecific = sensitivity;

  setIntSensitivity(sensitivity, true);
}


/**
 * Remove all the sensors' states.
 *
 * @return true on success.
 * @return false otherwise.
 */
bool Sensor::removeAllStates()
{
  return sdcard_remove_using_pattern(SENSOR_STATE_DIR, "*" SENSOR_STATE_FILENAME_EXT);
}

/**
 * Get the name of the file used to store the sensor's state.
 *
 * @param[in] create create the state file if it does not exist?
 *
 * @return the file name.
 * @return NULL if failed to build the file name.
 */
const char *Sensor::getStateFilename(bool create)
{
  char c;
  int  len;

  if(this->_psStateFilename) { goto exit; }

  // First get the filename's length.
  len = snprintf(&c, 1, "%s/%s%s%u%s%s%s",
		 SENSOR_STATE_DIR, type(),    SENSOR_STATE_FILENAME_SEP_STR,
		 (uint8_t)this->_dataChannel, SENSOR_STATE_FILENAME_SEP_STR,
		 uniqueId(),                 SENSOR_STATE_FILENAME_EXT);
  this->_psStateFilename = new char[len + 1];
  if(!this->_psStateFilename) { goto error_exit; }

  // Then actually build the name
  snprintf(this->_psStateFilename, len + 1, "%s/%s%s%u%s%s%s",
	   SENSOR_STATE_DIR, type(),    SENSOR_STATE_FILENAME_SEP_STR,
	   (uint8_t)this->_dataChannel, SENSOR_STATE_FILENAME_SEP_STR,
	   uniqueId(),                  SENSOR_STATE_FILENAME_EXT);

  // If the file does not exist then create it.
  if(create && !sdcard_exists(this->_psStateFilename))
  {
    sdcard_fcreateEmpty(this->_psStateFilename, true);
  }

  exit:
  return this->_psStateFilename;

  error_exit:
  return NULL;
}

/**
 * Default implementation. The sensor do not use a state.
 *
 * @param[in] pu32_size the the state object's size is written to. Is NOT NULL.
 *
 * @return NULL to indicate that the sensor do no use state.
 */
Sensor::State *Sensor::defaultState(uint32_t *pu32_size)
{
  *pu32_size = 0;
  return NULL;
}

/**
 * Read the sensor's state.
 *
 * @return the sensor's state.
 * @return NULL if the sensor do not use state.
 * @return NULL we could not get the state.
 */
Sensor::State *Sensor::state()
{
  const char *psStateFilename;
  uint8_t     u8;
  File        file;

  if( this->_pvState) { goto exit; }
  if((this->_pvState = defaultState(&this->_stateSize)))
  {
    this->_pvState->isInAlarm = false;
  }
  else { goto exit; }
  if(!(psStateFilename = getStateFilename(false))) { goto exit; }

  // Open the file
  if(!sdcard_exists(psStateFilename))
  {
    log_info_sensor(logger, "Sensor's state file does not exist; use default state.");
    goto exit;
  }
  if(!sdcard_fopen(&file, psStateFilename, FILE_OPEN | FILE_READ))
  {
    log_error_sensor(logger, "Failed to open sensor's state file in read mode. Use default state.");
    goto exit;
  }

  // Check the file size and the version
  if(  sdcard_fsize(&file) != this->_stateSize ||
      !sdcard_fread(&file, &u8, 1)             || ((State *)&u8)->version != this->_pvState->version)
  {
    log_info(logger, "Sensor's state object size or version do not match; use default state.");
    sdcard_fclose(&file);
    sdcard_remove(psStateFilename);
    goto exit;
  }

  // Read the file
  if(!sdcard_rewind(&file) || !sdcard_fread(&file, (uint8_t *)this->_pvState, this->_stateSize))
  {
    log_error_sensor(logger, "Failed to read sensor's state file '%s'. Use default state");
    sdcard_fclose(&file);
    this->_pvState            = defaultState(&this->_stateSize); // To be sure that we have the default state
    this->_pvState->isInAlarm = false;
    goto exit;
  }
  else { setCurrentStateAsReference(); }
  sdcard_fclose(&file);

  exit:
  return this->_pvState;
}

/**
 * Save the current sensor's state.
 *
 * @param[in] force  force state save, even if no changes have been detected?
 *
 * @return true on success.
 * @return false if the state has not been read.
 * @return false it the sensor do not use state.
 * @return false if failed to write state to disk.
 */
bool Sensor::saveState(bool force)
{
  const char *psStateFilename;
  File        file;
  bool        res = false;

  if(!force && !stateHasChanged()) { res = true; goto exit; }

  if(!this->_pvState || !(psStateFilename = getStateFilename(false))) { goto exit; }

  // Open the file
  if(!sdcard_fopen(&file, psStateFilename, FILE_OPEN_OR_CREATE| FILE_TRUNCATE | FILE_WRITE))
  {
    log_error_sensor(logger, "Failed to open sensor's state file in write mode.");
    goto exit;
  }

  // Write to file
  if(!(res = sdcard_fwrite(&file, (uint8_t *)this->_pvState, this->_stateSize)))
  {
    log_error_sensor(logger, "Failed to write sensor's state to file '%s'.", psStateFilename);
  }
  else { setCurrentStateAsReference(); }
  sdcard_fclose(&file);

  exit:
  return res;
}

/**
 * Set the current state as the reference state used to detect changes.
 */
void Sensor::setCurrentStateAsReference()
{
  if(this->_pvState)
  {
    if(   !this->_pu8StateRef) { this->_pu8StateRef = new uint8_t[this->_stateSize]; }
    memcpy(this->_pu8StateRef, (uint8_t *)this->_pvState,   this->_stateSize);
  }
  else if( this->_pu8StateRef) { delete this->_pu8StateRef; this->_pu8StateRef = NULL; }
}

/**
 * Indicate if there is a difference between the current state and the reference state.
 *
 * @return true  if there is a difference.
 * @return false otherwise.
 */
bool Sensor::stateHasChanged()
{
  return this->_pvState && (
      !this->_pu8StateRef ||
      memcmp(this->_pu8StateRef, (uint8_t *)this->_pvState, this->_stateSize) != 0);
}

/**
 * Indicate if the sensor is in alarm or not.
 *
 * @return true  if it is in alarm.
 * @return false otherwise.
 */
bool Sensor::isInAlarm()
{
  State *pvState;

  return (pvState = state()) && pvState->isInAlarm;
}

/**
 * Set or unset the current alarm status.
 *
 * @param[in] alarm is alarm set (true) or not (false)?
 */
void Sensor::setIsInAlarm(bool alarm)
{
  State *pvState;

  if((pvState = state()))
  {
    if(alarm && !pvState->isInAlarm)
    {
      // Alarm has just been triggered
      this->_alarmStatusJustChanged = true;
      pvState->isInAlarm            = true;
      saveState();

      if(this->_periodAlarmSec) { setPeriodSec(this->_periodAlarmSec); }
      if(this->_sendOnAlarmSet) { setRequestSendData();                }
      log_info_sensor(logger,  "The alarm has just been triggered.");
    }
    else if(!alarm && pvState->isInAlarm)
    {
      // Alarm has just been cleared
      this->_alarmStatusJustChanged = true;
      pvState->isInAlarm            = false;
      saveState();

      if(this->_periodAlarmSec)     { setPeriodSec(this->_periodNormalSec); }
      if(this->_sendOnAlarmCleared) { setRequestSendData();                 }
      log_info_sensor(logger, "The alarm has just stopped.");
    }
  }
}


/**
 * Open the sensor; set it up for a reading or for configuration.
 */
bool Sensor::open()
{
  board_add_power((BoardPower)this->_power);

  if(!this->_isOpened)
  {
    log_debug_sensor(logger,  "Opening...");
    if((this->_isOpened = openSpecific())) { log_debug_sensor(logger, "Opened.");                }
    else                                   { log_error_sensor(logger, "Failed to open sensor."); }

    // Take advantage of this opening to set up alerts
    if(this->_isOpened && this->_hasAlarmToSet) { configAlert(); }
  }

  return this->_isOpened;
}

/**
 * Close sensor.
 */
void Sensor::close()
{
  if(this->_isOpened)
  {
    this->_isOpened = false;
    closeSpecific();
    log_debug_sensor(logger, "Closed.");
  }
}

/**
 * Set the readings timestamp.
 *
 * @param[in] ts The new timestamp, in seconds since 2000-01-01T00:00:00.
 *               O means use current time.
 */
void Sensor::setReadingsTimestamp(ts2000_t ts)
{
  this->_readingsTs2000 = ts ? ts : rtc_get_date_as_secs_since_2000();
}

/**
 * Set the interruption timestamp.
 *
 * @param[in] ts The new timestamp, in seconds since 2000-01-01T00:00:00.
 *               O means use current time.
 */
void Sensor::setInterruptionTimestamp(ts2000_t ts)
{
  this->_interruptionTs2000 = ts ? ts : rtc_get_date_as_secs_since_2000();
}

void Sensor::setHasNewData(bool hasNewData)
{
  if((this->_hasNewData = hasNewData))
  {
    setReadingsTimestamp();
    setIsInAlarm(currentValuesAreInAlarmRange());
  }
}

/**
 * Get reading from a sensor
 */
bool Sensor::read()
{
  bool res;

  setHasNewData(false);
  log_debug_sensor(logger, "Reading...", name());
  if((res = readSpecific()))
  {
    setHasNewData();
    log_info_sensor(logger, "Reading done.");
  }
  else { log_error_sensor(logger, "Failed to read sensor."); }

  return res;
}

/**
 * Configure the sensor to generate alerts
 */
bool Sensor::configAlert()
{
  bool ok;

  log_debug_sensor(logger, "Setting up alarms...");
  if((ok = configAlertSpecific())) { log_info_sensor( logger, "Alarms have been set.");    }
  else                             { log_error_sensor(logger, "Failed to set up alarms."); }

  if(ok) { this->_hasAlarmToSet = false; }
  return ok;
}


/**
 * Process interruptions.
 *
 * @param[in] ints a ORed combination of interruption flags.
 */
void Sensor::processInterruption(CNSSInt::Interruptions ints)
{
  if(!(ints & this->_intSensitivity)) { return; }

  setInterruptionTimestamp();
  if(  ints & this->_intSensitivitySpecific) { processInterruptionSpecific(ints); }
  if(  ints & this->_intSensitivityRead)
  {
    // Get a sensor reading
    open();
    read();
    close();
  }
  if(this->_sendOnInterrupt && (ints & this->_intSensitivityToSend)) { setRequestSendData(); }
  if(  ints & this->_intSensitivityToAlarm)
  {
    processAlarmInterruption(this->_intSensitivityToAlarm);
  }
}


/**
 * Indicate if the alarm status has just changed: has just been triggered or has just been cleared.
 *
 * @param[in] clear clear the 'has just changed' flag?
 *
 * @return true  if the alarm status has just changed.
 * @return false otherwise.
 */
bool Sensor::alarmStatusHasJustChanged(bool clear)
{
  bool res = this->_alarmStatusJustChanged;
  if(clear) { this->_alarmStatusJustChanged = false; }

  return res;
}

/**
 * Indicate if we have to read the sensor when the alarm status changes.
 *
 * @return true  if we have to read it.
 * @return false otherwise.
 */
bool Sensor::readOnAlarmChange() { return true; }


/**
 * Indicate if the sensor request to send data or not.
 *
 * @param[in] clear clear the request flag?
 *                  If set to true then the next call to this function will always return false,
 *                  provided that the flag has not been set again between the two calls.
 *
 * @return true  if the sensor is requesting to send data.
 * @return false otherwise.
 */
bool Sensor::requestToSendData(bool clear)
{
  bool res =  this->_requestSendData;
  if(clear) { this->_requestSendData = false; }

  return res;
}


/**
 * Write the sensor's data to a ConnecSenS RF data frame.
 *
 * @param[in,out] pvFrame the data frame. MUST be NOT NULL. MUST have been initialised.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool Sensor::writeDataToCNSSRFDataFrame(CNSSRFDataFrame *pvFrame)
{
  bool res = false;

  // Check that we can write to the frame
  if(!hasNewData() || this->_dataChannel == CNSSRF_DATA_CHANNEL_UNDEFINED) { return false; }

  cnssrf_data_frame_set_current_data_channel(pvFrame, this->_dataChannel);

  if(this->_writeTypeHashToCNSSRF)
  {
    if(!cnssrf_dt_hash_write_sensortype_mm3hash32(pvFrame, typeHash())) { goto exit; }
  }

  res = writeDataToCNSSRFDataFrameSpecific(pvFrame);

  exit:
  return res;
}


/**
 * Write the CSV header for this sensor to an output buffer, as a string.
 *
 * @param[out] ps_header where the CSV header is written to. MUST be NOT NULL.
 * @param[in]  size      ps_header's size.
 * @param[in]  prepend   what, if anything, to prepend the the header values.
 *
 * @return the header's string length.
 * @return 0 if ps_header is too small.
 * @return 0 for any other error.
 */
uint32_t Sensor::csvHeader(char *ps_header, uint32_t size, CSVHeaderPreprend prepend)
{
  uint32_t     l, s;
  bool         firstValue = true;
  const char **ppsValues  = csvHeaderValues();

  this->_csvNbValues = 0;
  if(!ppsValues) { goto error_exit; }

  for(s = size ; *ppsValues; ppsValues++, this->_csvNbValues++)
  {
    if(!firstValue)
    {
      if(s <= 1) { goto error_exit; }
      *ps_header++ = OUTPUT_DATA_CSV_SEP;
      s--;
    }

    switch(prepend)
    {
      case CSV_HEADER_PREPEND_NAME:
	l = strlcpy(ps_header, name(), s);
	if(l >= s) { goto error_exit; }
	ps_header += l;
	s         -= l;
	break;

      case CSV_HEADER_PREPEND_NONE:
      default:
	// Do nothing
	break;
    }
    if(prepend != CSV_HEADER_PREPEND_NONE)
    {
      if(s <= 1) { goto error_exit; }
      *ps_header++ = OUTPUT_DATA_CSV_HEADER_PREPEND_SEP;
      s --;
    }
    l = strlcpy(ps_header, *ppsValues, s);
    if(l >= s) { goto error_exit; }
    ps_header += l;
    s         -= l;
    firstValue = false;
  }

  return size - s;

  error_exit:
  this->_csvNbValues = 0;
  return 0;
}

/**
 * Write the CSV data to a buffer.
 * If there are no new data then write empty values.
 *
   * @param[out] ps_data where the data are written to. A string is written. MUST be NOT NULL.
   * @param[in]  size    ps_data's size.
   *
   * @return the CSV data string length. Can be 0 if the sensor outputs only one value and
   *         no value is available.
   * @return -1 if ps_data is too small.
   * @return -1 for any other type of error.
 */
int32_t Sensor::csvData(char *ps_data, uint32_t size)
{
  uint8_t i;
  int32_t res = -1;

  if(hasNewData())
  {
    res = csvDataSpecific(ps_data, size);
  }
  else
  {
    if(size >= this->_csvNbValues)
    {
      for(i = 0; i < this->_csvNbValues - 1; i++)
      {
	ps_data[i] = OUTPUT_DATA_CSV_SEP;
      }
      ps_data[i] = '\0';
      res        = (int32_t)i;
    }
  }

  return res;
}

/**
 * Create a CSV string using sting values.
 *
 * The result is: "<V1><CSV_SEP><V2><CSV_SEP>...<Vn>"
 *
 * @param[out] ps_data    where the CSV string is written to. MUST be NOT NULL.
 * @param[in]  size       ps_data's size.
 * @param[in]  pps_values a table of string values. The strings can be empty or NULL.
 * @para[in]   nb_values  the number of value strings in the table pps_values.
 *
 * @return the CSV string length. Can be 0 if the sensor outputs only one value and
 *         no value is available.
 * @return -1 if ps_data is too small.
 * @return -1 if nb_values is different from csvNbValues();
 */
int32_t Sensor::csvMakeStringUsingStringValues(char         *ps_data,
					       uint32_t     size,
					       const char **pps_values,
					       uint32_t     nb_values)
{
  uint32_t    i, s, l;
  const char *psValue;
  bool        firstValue;

  if(nb_values != csvNbValues()) { goto error_exit; }

  for(i = 0, s = size, firstValue = true; i < nb_values; i++)
  {
    if(!firstValue)
    {
      if(s <= 1) { goto error_exit; }
      *ps_data++ = OUTPUT_DATA_CSV_SEP;
      s--;
    }

    psValue = pps_values[i];
    if(psValue && *psValue)
    {
      l = strlcpy(ps_data, psValue, s);
      if(l >= s) { goto error_exit; }
      ps_data += l;
      s       -= l;
    }
    firstValue = false;
  }

  return size - s;

  error_exit:
  return -1;
}



// ================== Default implementation for optional virtual functions

bool Sensor::configAlertSpecific()
{
  // Do nothing
  return true;
}

bool Sensor::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  UNUSED(json);
  UNUSED(pvAlarmInts);
  // Do nothing
  return true;
}

bool Sensor::jsonSpecificHandler(const JsonObject& json)
{
  UNUSED(json);
  // Do nothing
  return true;
}

void Sensor::processAlarmInterruption(CNSSInt::Interruptions ints)
{
  UNUSED(ints);
  // Do nothing
}

void Sensor::processInterruptionSpecific(CNSSInt::Interruptions ints)
{
  UNUSED(ints);
  // Do nothing
}

bool Sensor::currentValuesAreInAlarmRange()
{
  return isInAlarm();  // So that the alarm status does not change.
}

