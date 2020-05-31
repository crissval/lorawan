#include <stdarg.h>
#include "connecsens.hpp"
#include "version.h"
#include "cnsslog.h"
#include "cnssint.hpp"
#include "utils.h"
#include "leds.h"
#include "logger.h"
#include "statusindication.h"
#include "board.h"
#include "sdcard.h"
#include "factory.hpp"
#include "rtc.h"
#include "datalog-cnssrf.h"
#include "configmonitor.h"
#include "nodeinfo.h"
#include "board.h"
#include "buzzer.h"
#include "nodebattery.hpp"
#include "murmur3.h"
#include "powerandclocks.h"
#include "LoRaWAN.hpp"
#include "SigFox.hpp"
#include "NwkSimul.hpp"


#ifndef SYS_LOG_FILE_SIZE_MAX
#define SYS_LOG_FILE_SIZE_MAX          10000000  // 10 Mo
#endif
#ifndef SYS_LOG_FILES_TOTAL_SIZE_MAX
#define SYS_LOG_FILES_TOTAL_SIZE_MAX  100000000  // 100 Mo
#endif

#ifndef NODE_BATT_V_READING_PERIOD_MAX_SEC
#define NODE_BATT_V_READING_PERIOD_MAX_SEC  3600  // Read battery if it has not been read for an hour.
#endif

#define CONNECSENS_NODE_UNIQUE_ID_PREFIX  "CNSS-NDSTEPAT-"


#ifdef RF_FRAMES_ADD_GEOPOS_TO_EACH
#define ADD_GEOPOS_TO_EACH_RF_FRAME_DEFAULT_VALUE  true
#else
#define ADD_GEOPOS_TO_EACH_RF_FRAME_DEFAULT_VALUE  false
#endif


  CREATE_LOGGER(connecsens);
#undef  logger
#define logger  connecsens


uint8_t        ConnecSenS::_cnssrfBuffer[        CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE];
CNSSRFMetaData ConnecSenS::_cnssrfMetaBuffer[    CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY];
uint8_t        ConnecSenS::_cnssrfSendBuffer[    CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE];
CNSSRFMetaData ConnecSenS::_cnssrfSendMetaBuffer[CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY];


ConnecSenS *ConnecSenS::_pvInstance = NULL;

const char *ConnecSenS::_requiredSDDirectoryStructure[] =
{
    ENV_DIRECTORY_NAME,
    LOG_DIRECTORY_NAME,
    SYSLOG_DIRECTORY_NAME,
    DATALOG_DIR,
    PRIVATE_DATA_DIRECTORY_NAME,
    SENSOR_STATE_DIR,
    RTC_PRIVATE_DIR_NAME,
    OUTPUT_DIR,
    OUTPUT_DATA_CSV_DIR,
    NULL
};

const char *ConnecSenS::_oldDirAndFilesToRemove[] =
{
    "env/tosend.log",
    NULL
};


/**********************************************************/
/* Constructeur / Initialisation de l'environnement *******/
/**********************************************************/
ConnecSenS::ConnecSenS() : myBuffer(CONNECSENS_BUFFER_SIZE)
{
  this->DeviceName[0]              = '\0';
  this->_uniqueId[ 0]              = '\0';
  this->_experimentName[0]         = '\0';
  this->_cnssrfDatalogFileName[0]  = '\0';
  this->_lastWakeupTs2000          = 0;
  this->_enableGPS                 = false;
  this->_addGeoPosToEachRFFrame    = ADD_GEOPOS_TO_EACH_RF_FRAME_DEFAULT_VALUE;
  this->_firstGPSFix               = true;
  this->_gpsFixDoneOutsidePeriodic = false;
  this->_timeSyncMethodChanged     = false;
  this->_workingMode               = WMODE_NORMAL;
  this->NumberOfSensors            = 0;
  this->Network                    = NULL;
  this->_lastPeriodicProcess       = 0;
  this->_batteryLastReadTs2000     = 0;
  this->_sensitiveToExternalInterruption = false;
  this->_sensitiveToInternalInterruption = false;
  this->_output_data_to_csv              = true;
  this->_output_data_csv_file_is_opened  = false;

  _pvInstance = this;
}

ConnecSenS *ConnecSenS::instance() { return _pvInstance; }

/**
 * Return the node's unique identifier.
 *
 * @return the identifier.
 * @return an empty string if no identifier is set.
 */
const char *ConnecSenS::uniqueId()
{
  uint8_t     len;
  const char *psBoardSN;

  if(*this->_uniqueId) { return this->_uniqueId; }

  // No unique identifier is set; build one.
  strcpy(    this->_uniqueId, CONNECSENS_NODE_UNIQUE_ID_PREFIX);
  if(*(psBoardSN = nodeinfo_main_board_SN()))
  {
    if(strlcat(this->_uniqueId, psBoardSN, sizeof(this->_uniqueId)) >= sizeof(this->_uniqueId))
    {
      this->_uniqueId[0] = '\0';
    }
  }
  else
  {
    // Use the µC UID
    len = strlen(this->_uniqueId);
    binToHex(   &this->_uniqueId[len], (const uint8_t *) UID_BASE,  5, true); len += 10;
    strlcat(    &this->_uniqueId[len], (const char *)   (UID_BASE + 5), 7);
  }

  return this->_uniqueId;
}

/**
 * Set the node's unique identifier.
 *
 * @param[in] psUId the identifier. Use NULL or an empty string to remove the current identifier.
 *
 * @return true
 */
bool ConnecSenS::setUniqueId(const char *psUId)
{
  bool res = true;

  if(!psUId || !*psUId ||
      !(res = strlcpy(this->_uniqueId, psUId, sizeof(this->_uniqueId)) < sizeof(this->_uniqueId)))
  {
    this->_uniqueId[0] = '\0';
  }

  return res;
}

void ConnecSenS::initialisation()
{
  uint8_t  i;
  bool     ok;
  Datetime dt;

  HAL_PWREx_EnableVddIO2();
  board_init();
  sdcard_init();
  cnsslog_init();

 // Init stuff
  CNSSInt::instance();  // To make sure that the interruption IOs are configured
  config_monitor_init();

  RefreshTimestamp(); // Set current timestamp value. Will be overwritten if time is updated.
  timer_init(&this->_wakeupTimer, 0, TIMER_SECS | TIMER_SINGLE_SHOT | TIMER_RELATIVE);

  // Remove old files and directories that are no longer needed
  for(const char **psPath = _oldDirAndFilesToRemove; *psPath; psPath++)
  {
    if(sdcard_exists(*psPath)) { sdcard_remove(*psPath); }
  }

  // Create directory structure on SD card
  for(const char **psDirName = _requiredSDDirectoryStructure; *psDirName; psDirName++)
  {
    if(!sdcard_exists(*psDirName)) { sdcard_mkdir(*psDirName); }
  }

  // Perform some cleaning up to remove old files that are not used anymore
  if(sdcard_exists(SLEEP_FILE_NAME)) { sdcard_remove(SLEEP_FILE_NAME); }

  // Node information initialisation
  nodeinfo_init();

  // Load configuration from configuration file
  ok = loadConfig();
  status_ind_set_status(STATUS_IND_AWAKE);
  log_info(logger, "ConnecSenS OS " VERSION_STR " started.");
  if(!ok)
  {
    log_error(logger, "While reading configuration file.");
    // Make the LEDs blink forever.
    status_ind_set_status(STATUS_IND_CFG_ERROR);
  }
  if(!this->_enableGPS) { log_warn(logger, "GPS is disabled."); }

  // Is this the first run since power up? If so then some things must be done.
  if(rtc_is_first_run_since_power_up())
  {
    log_info(logger, "This is first run since power up.");

    // Set RTC calibration
    if(rtc_load_and_set_calibration()) { log_info( logger, "RTC has been calibrated."); }
    else                               { log_error(logger, "Failed to calibrate RTC."); }

    // Delete state file if there is one
    if(sdcard_exists(STATE_FILE_NAME))
    {
      if(sdcard_remove(STATE_FILE_NAME)) { log_debug(logger, "State file has been removed."); }
      else                               { log_error(logger, "Failed to remove state file."); }
    }
  }

  // Initialise ConnecSenS RF communication
  cnssrf_data_frame_init(&this->_cnssrfDataFrame,
			 _cnssrfBuffer,         sizeof(_cnssrfBuffer),
			 _cnssrfMetaBuffer,     CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY);
  cnssrf_data_frame_init(&this->_cnssrfSendDataFrame,
  			 _cnssrfSendBuffer,     sizeof(_cnssrfSendBuffer),
  			 _cnssrfSendMetaBuffer, CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY);

  // Get battery voltage
  readBatteryVoltage();

  // Update time if needed
  if(workingMode() != WMODE_CAMPAIGN_RANGE &&
      (rtc_is_first_run_since_power_up() ||
	  this->_timeSyncMethodChanged   ||
	  config_monitor_manual_time_has_changed(&this->manualTimestamp, false)))
  {
    ok = false;
    if(this->_enableGPS)
    {
      rtc_get_date(&dt);
      if(this->_timeSyncMethodChanged || dt.year < CURRENT_YEAR_MIN)
      {
	ok = setRTCUsingGPS(true);
	this->_gpsFixDoneOutsidePeriodic = true;
      }
    }
    else { ok = setRTCUsingManualTime(); }
    log_info(logger, ok ? "Time has been updated" : "Current time is kept");
  }

  this->myBuffer.clean();

  // Initialise the CNSSRF datalogger
  openCNSSRFDatalog();

  // The configuration file may have been changed
  detectConfigurationChange();

  // If we have a network interface then start the joining process now.
  if(this->Network && this->Network->periodSec())
  {
    this->Network->registerEventClient(this);
    this->Network->open();
    //this->Network->join(this->_workingMode != WMODE_CAMPAIGN_RANGE);
    this->Network->sleep();
  }

  // Some sensor may need to be constantly opened, so do it here
  for(i = 0; i < this->NumberOfSensors; i++)
  {
    Sensor *pvSensor = this->_sensors[i];
    if(pvSensor->needsToBeConstantlyOpened())
    {
      log_debug(logger, "Sensor '%s' needs to be constantly opened.", pvSensor->name());
      if(!pvSensor->open())
      {
	log_error(logger, "Failed to persistently open sensor '%s'.", pvSensor->name());
      }
      else { log_info(logger, "Sensor '%s' has been opened to work in continuous mode.", pvSensor->name()); }
    }
    else { log_debug(logger, "Sensor '%s' does not need to be constantly opened.", pvSensor->name()); }
  }
}


/**
 * Open the ConnecSenS RF datalog file.
 */
void ConnecSenS::openCNSSRFDatalog()
{
  if(!*this->_cnssrfDatalogFileName)
  {
    char    *psBuffer   = this->_cnssrfDatalogFileName;
    uint32_t bufferSize = sizeof(this->_cnssrfDatalogFileName);

    strlcpy(psBuffer, DATALOG_DIR "/", bufferSize);
    if(this->Network && *this->Network->deviceIdAsString())
    {
      strlcat(psBuffer,  this->Network->deviceIdAsString(), bufferSize);
      strlcat(psBuffer, "-",                                bufferSize);
    }
    else
    {
      log_warn(logger, "No network set or no device identifier on this network; datalog file name will be generic.");
    }
    strlcat(psBuffer, DATALOG_CNSSRF_FILENAME, bufferSize);
  }

  // Open the datalog file
  log_debug(logger, "Opening datalog file '%s'...", this->_cnssrfDatalogFileName);
  if(datalog_cnssrf_init(this->_cnssrfDatalogFileName))
  {
    log_debug(logger, "CNSSRF datalog file '%s' has been opened.", this->_cnssrfDatalogFileName);
  }
  else
  {
    log_error(logger, "Failed to open CNSSRF datalog file '%s'.", this->_cnssrfDatalogFileName);
  }
}

/**
 * Close the ConnecSenS RF datalog file.
 */
void ConnecSenS::closeCNSSRFDatalog()
{
  datalog_cnssrf_deinit();
}


/**
 * Process currently awaiting tasks, if there are any.
 *
 * @param[in] irq      Also process interruptions?
 * @param[in] periodic Also process periodic tasks?
 *
 * @return true  if there are some more processing left to do.
 * @return false otherwise.
 */
bool ConnecSenS::process(bool irq, bool periodic)
{
  uint8_t i;
  bool    processedSomething;

  do
  {
    board_watchdog_reset();
    processedSomething = false;
    while(timer_process()) { processedSomething = true; }
    for(i = 0; i < this->NumberOfSensors; i++)
    {
      while(this->_sensors[i]->process()) { processedSomething = true; }
    }
    if(this->Network)
    {
      while(this->Network    ->process()) { processedSomething = true; }
    }
  }
  while(processedSomething);

  if(irq)      { InterruptHandler(); }
  if(periodic) { PeriodicHandler();  }

  return false;
}


/**
 * Execute processing that can, or should, be done when a task is blocked,
 * waiting for something to happen.
 */
void ConnecSenS::yield()
{
  if(_pvInstance)
  {
    while(_pvInstance->process());   // Process while the is something left to process.
  }
}

/**
 * Start a new ConnecSenS RF dataframe.
 *
 * @param[in] timestamp the timestamp to sue for the data. If 0 then use current timestamp.
 */
bool ConnecSenS::startNewCNSSRFDataFrame(ts2000_t timestamp)
{
  ts2000_t nowTs2000;
  uint32_t hash;
  float    latitude, longitude;

  // Clear RF frame object
  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);

  // First write the node informations
  cnssrf_data_frame_set_current_data_channel(&this->_cnssrfDataFrame,
					     CNSSRF_DATA_CHANNEL_NODE);
  if(timestamp)
  {
    if(!cnssrf_dt_timestamp_utc_write_secs_to_frame(&this->_cnssrfDataFrame, timestamp))
    { goto error_exit; }
  }
  else
  {
    if(!cnssrf_dt_timestamp_utc_write_datetime_to_frame(
	&this->_cnssrfDataFrame,
	&this->currentTimestamp)) { goto error_exit; }
  }

  // Add battery voltage informations
  nowTs2000 = rtc_get_date_as_secs_since_2000();
  if(nowTs2000 - this->_batteryLastReadTs2000 > NODE_BATT_V_READING_PERIOD_MAX_SEC)
  {
    readBatteryVoltage();
  }
  NodeBattery::instance()->writeDataToCNSSRFDataFrame(&this->_cnssrfDataFrame);

  if(config_monitor_hash_mm3_32(&hash) &&
      !cnssrf_dt_hash_write_config_mm3hash32(&this->_cnssrfDataFrame, hash))
  { goto error_exit; }

  // Add geographical position?
  if(this->_addGeoPosToEachRFFrame && rtc_geoposition(&latitude, &longitude, NULL))
  {
    if(!cnssrf_dt_position_write_geo2d_to_frame(&this->_cnssrfDataFrame,
						latitude,
						longitude))
    { goto error_exit; }
  }

  return true;

  error_exit:
  return false;
}

bool ConnecSenS::appendSensorDataToCurrentCNSSRFDataFrame(Sensor *pvSensor)
{
  cnssrf_data_frame_set_current_data_channel(&this->_cnssrfDataFrame, pvSensor->cnssrfDataChannel());
  if(!pvSensor->writeDataToCNSSRFDataFrame(  &this->_cnssrfDataFrame)) { return false; }

  return true;
}

bool ConnecSenS::saveCurrentCNSSRFDataFrame()
{
  if(!cnssrf_data_frame_is_empty(&this->_cnssrfDataFrame))
  {
    // Write to datalog file
    if(!datalog_cnssrf_add(&this->_cnssrfDataFrame))
    {
      log_error(logger, "Failed to add new entry to CNSSRF datalog.");
      // TODO: find a better solution; it seems to be a problem with the SD card.
      log_info( logger, "Restarting.");
      board_reset(BOARD_SOFTWARE_RESET_ERROR);
      return false;
    }

    // Transform the binary data to an hex string
    binToHex((char *)this->myBuffer.getBufferPtr(),
	     cnssrf_data_frame_data(&this->_cnssrfDataFrame),
	     cnssrf_data_frame_size(&this->_cnssrfDataFrame),
	     true);

    // Write to log
    log_info(logger, "CNSSRF payload: %s", (char *)this->myBuffer.getBufferPtr());

    // Write meta data to log
    // Has an hex string, list of uint16_t values stored in little endian
    binToHex((char *)this->myBuffer.getBufferPtr(),
	     (uint8_t *)cnssrf_data_frame_meta_data(&this->_cnssrfDataFrame),
	     cnssrf_data_frame_meta_data_size(      &this->_cnssrfDataFrame),
	     true);
    log_debug(logger, "CNSSRF meta data: %s", (char *)this->myBuffer.getBufferPtr());
  }

  return true;
}

void  ConnecSenS::readBatteryVoltage()
{
  bool         ok        = false;
  NodeBattery *pvBattery = NodeBattery::instance();

  if(pvBattery->open())
  {
    if((ok = pvBattery->read()))
    {
      log_info(logger, "Battery voltage: %1.3f V", pvBattery->voltageV());
      if(pvBattery->isCharging()) { log_info(logger, "The battery is charging"); }
      this->_batteryLastReadTs2000 = rtc_get_date_as_secs_since_2000();
    }
    pvBattery->close();
  }
  if(!ok) { log_error(logger, "Failed to read node's battery."); }
}

/**
 * Save a sensor's data.
 *
 * Create a ConnecSenS RF frame with the data for only this sensor.
 * Will most likely be used by sensors that produce data asyncronously.
 *
 * @param[in] pvSensor The sensor. MUST be NOT NULL.
 * @param[in] ts2000   The timestamp to use for the CNSSRF frame.
 *
 * @return true  On success.
 * @return false Otherwise.
 */
bool ConnecSenS::saveSensorData(Sensor *pvSensor, ts2000_t ts2000)
{
  char    *psCSVBuffer;
  uint32_t csvBufferSize, len, i;
  int32_t  l;
  float    latitude, longitude;
  Sensor  *pvS;
  Datetime dt;
  bool     res;
  bool     writeCSVData = false;

  // Write CNSSRF data
  if((res = startNewCNSSRFDataFrame(ts2000)))
  {
    res &= appendSensorDataToCurrentCNSSRFDataFrame(pvSensor);
    res &= saveCurrentCNSSRFDataFrame();
  }

  // Open the CSV data output file if not already done
  datetime_from_sec_2000(&dt, ts2000);
  if((writeCSVData = (openDataOutputCSVFile() && startDataOutputCSVLine(&dt))))
  {
    len            = strlen(this->_output_data_csv_buffer);
    psCSVBuffer    =        this->_output_data_csv_buffer  + len;
    csvBufferSize  = sizeof(this->_output_data_csv_buffer) - len;
  }

  // So go through all the sensors
  for(i = 0; i < this->NumberOfSensors; i++)
  {
    pvS = this->_sensors[i];

    // Write CSV data, even if the sensor has produced no data
    if(writeCSVData && csvBufferSize >= 1)
    {
      *psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
      csvBufferSize--;
      l = pvS->csvData(psCSVBuffer, csvBufferSize);
      if(l < 0) { writeCSVData = false; }
      else
      {
	psCSVBuffer   += l;
	csvBufferSize -= l;
      }
    }
    else { writeCSVData = false; }
  }

  // Write GPS data to CSV if we have to
  if(this->_addGeoPosToEachRFFrame && rtc_geoposition(&latitude, &longitude, NULL))
  {
    len = snprintf(psCSVBuffer, csvBufferSize, "%c%f%c%f",
    		     OUTPUT_DATA_CSV_SEP, latitude,
    		     OUTPUT_DATA_CSV_SEP, longitude);
    if(len >= csvBufferSize) { writeCSVData = false; }
  }
  else
  {
    if(csvBufferSize >= 3)
    {
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = '\0';
    }
    else { writeCSVData = false; }
  }

  // Write CSV data
  if(writeCSVData)
  {
    sdcard_fwrite_string(&this->_output_data_csv_file, this->_output_data_csv_buffer);
    sdcard_fsync(        &this->_output_data_csv_file);
  }

  pvSensor->clearHasNewData();
  return res;
}

void ConnecSenS::InterruptHandler()
{
  char    *psCSVBuffer;
  uint32_t csvBufferSize, len, i;
  ts2000_t ts2000;
  int32_t  l;
  float    latitude, longitude;
  bool     writeCSVData    = false;
  bool     send            = false;
  bool     wroteCNSSRFData = false;

  RefreshTimestamp();

  // Process interruptions
  CNSSInt::instance()->processInterruptions();

  // Open the CSV data output file if not already done
  if((writeCSVData = (openDataOutputCSVFile() && startDataOutputCSVLine())))
  {
    len            = strlen(this->_output_data_csv_buffer);
    psCSVBuffer    =        this->_output_data_csv_buffer  + len;
    csvBufferSize  = sizeof(this->_output_data_csv_buffer) - len;
  }

  // See if a sensor ask to send data
  for(i = 0; i < this->NumberOfSensors; i++)
  {
    Sensor *pvSensor = this->_sensors[i];
    if(pvSensor->alarmStatusHasJustChanged(true))
    {
      if(pvSensor->readOnAlarmChange() && !pvSensor->hasNewData() && pvSensor->open())
      {
	pvSensor->read();
	pvSensor->close();
      }
    }

    if(pvSensor->requestToSendData(true))
    {
      if(!pvSensor->hasNewData())
      {
	if(pvSensor->open() && pvSensor->read()) { send = true; }
	pvSensor->close();
      }
      else { send = true; }
    }

    // Write CSV data, even if the sensor has produced no data
    if(writeCSVData && csvBufferSize >= 1)
    {
      *psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
      csvBufferSize--;
      l = pvSensor->csvData(psCSVBuffer, csvBufferSize);
      if(l < 0) { writeCSVData = false; }
      else
      {
	psCSVBuffer   += l;
	csvBufferSize -= l;
      }
    }
    else { writeCSVData = false; }

    if(pvSensor->hasNewData())
    {
      // Write a CNSSRF data frame with only this sensor's data and
      // use the sensor's interruption timestamp if it has one, else use readings timestamp.
      ts2000 = pvSensor->interruptionTimestamp();
      pvSensor->clearInterruptionTimestamp();
      log_debug(logger, "Interruption timestamp: %lu.", ts2000);
      if(!ts2000) { ts2000 = pvSensor->readingsTimestamp(); }
      if(!startNewCNSSRFDataFrame(ts2000)) { return; }
      appendSensorDataToCurrentCNSSRFDataFrame(pvSensor);
      pvSensor->clearHasNewData();
      saveCurrentCNSSRFDataFrame();
      wroteCNSSRFData = true; // Well, at least we tried; maybe CSV will get more lucky.
    }
  }

  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);

  // Write GPS data to CSV if we have to
  if(this->_addGeoPosToEachRFFrame && rtc_geoposition(&latitude, &longitude, NULL))
  {
    len = snprintf(psCSVBuffer, csvBufferSize, "%c%f%c%f",
    		     OUTPUT_DATA_CSV_SEP, latitude,
    		     OUTPUT_DATA_CSV_SEP, longitude);
    if(len >= csvBufferSize) { writeCSVData = false; }
  }
  else
  {
    if(csvBufferSize >= 3)
    {
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = '\0';
    }
    else { writeCSVData = false; }
  }

  // Write CSV data
  if(writeCSVData && wroteCNSSRFData)
  {
    sdcard_fwrite_string(&this->_output_data_csv_file, this->_output_data_csv_buffer);
    sdcard_fsync(        &this->_output_data_csv_file);
  }

  // Send data?
  if(send)
  {
    log_info(logger, "Trigger data sending.");
    this->ActionOnNetwork();
  }
}

void ConnecSenS::PeriodicHandler()
{
  char    *psCSVBuffer;
  uint32_t csvBufferSize, len, i;
  int32_t  l;
  Sensor  *pvSensor;
  float    latitude, longitude;
  bool     actionOnSensorDone = false;
  bool     actionOnGPSDone    = false;
  bool     writeCSVData       = this->_output_data_to_csv;
  bool     gpsDataWritten     = false;
  bool     cnssrfFrameStarted = false;
  bool     aSensorIsInAlarm   = false;

  // Our time granularity is 1 second so do not do anything if this function
  // was called less than a second ago.
  i = board_ms_now();
  if(board_ms_diff(this->_lastPeriodicProcess, i) < 1000) { goto exit; }
  this->_lastPeriodicProcess = i;

  RefreshTimestamp();

  // Write the config if we have to
  if(this->_sendConfigTimer.itsTime())
  {
    log_info(logger, "Prepare an RF frame with node configuration data.");
    if(!writeCNSSRFFramesWithConfig()) { log_error(logger, "Failed to write RF frame with configuration."); }
  }

  // Open the CSV data output file if not already done
  if((writeCSVData = (openDataOutputCSVFile() && startDataOutputCSVLine())))
  {
    len            = strlen(this->_output_data_csv_buffer);
    psCSVBuffer    =        this->_output_data_csv_buffer  + len;
    csvBufferSize  = sizeof(this->_output_data_csv_buffer) - len;
  }

  // Do we have to do with the sensors?
  for(i = 0; i < this->NumberOfSensors; i++)
  {
    pvSensor = this->_sensors[i];

    if(pvSensor->itsTime())
    {
      if(!cnssrfFrameStarted)
      {
	// Start a new CNSSRF data frame
	if(!startNewCNSSRFDataFrame())
	{
	  log_error(logger, "Failed to start new CNSS RF frame.");
	  goto data_write_failed;
	}
	cnssrfFrameStarted = true;
      }

      this->ActionOnSensor(pvSensor);
      actionOnSensorDone       = true;
      board_watchdog_reset();
    }

    aSensorIsInAlarm |= pvSensor->isInAlarm();

    // Write CSV data, even if the sensor has produced no data
    if(writeCSVData && csvBufferSize >= 1)
    {
      *psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
      csvBufferSize--;
      l = pvSensor->csvData(psCSVBuffer, csvBufferSize);
      if(l < 0) { writeCSVData = false; }
      else
      {
	psCSVBuffer   += l;
	csvBufferSize -= l;
      }
    }
    else { writeCSVData = false; }

    pvSensor->clearHasNewData();
  }

  // Do the GPS synchronisation if it's time
  // Make sure to call GPS.itsTime() to update next time, if GPS is enabled.
  if(this->_enableGPS && this->GPS.itsTime() && !this->_gpsFixDoneOutsidePeriodic)
  {
    setRTCUsingGPS(false);
  }
  if(this->GPS.hasLocation())  // If we got a GPS location from periodic fix or from an unperiodic fix.
  {
    if(cnssrf_data_frame_is_empty(&this->_cnssrfDataFrame))
    {
      // Start a new CNSSRF data frame
      if(!startNewCNSSRFDataFrame())
      {
        log_error(logger, "Failed to start new CNSS RF frame.");
        goto data_write_failed;
      }
      gpsDataWritten = this->_addGeoPosToEachRFFrame;
    }
    if(!gpsDataWritten)
    {
      cnssrf_data_frame_set_current_data_channel( &this->_cnssrfDataFrame, CNSSRF_DATA_CHANNEL_NODE);
      if(!cnssrf_dt_position_write_geo2d_to_frame(&this->_cnssrfDataFrame,
						  this->GPS.location().latitude,
						  this->GPS.location().longitude))
      { goto data_write_failed; }
    }
    actionOnGPSDone = true;
  }

  this->_gpsFixDoneOutsidePeriodic = false;  // So that next periodic GPS fix can be done.

  // Write GPS information to CSV data output
  if(!actionOnSensorDone && !actionOnGPSDone) { writeCSVData = false; }
  if(writeCSVData)
  {
    if(actionOnGPSDone)
    {
      len = snprintf(psCSVBuffer, csvBufferSize, "%c%f%c%f",
		     OUTPUT_DATA_CSV_SEP, this->GPS.location().latitude,
		     OUTPUT_DATA_CSV_SEP, this->GPS.location().longitude);
      if(len >= csvBufferSize) { writeCSVData = false; }
    }
    else if(this->_addGeoPosToEachRFFrame && rtc_geoposition(&latitude, &longitude, NULL))
    {
      len = snprintf(psCSVBuffer, csvBufferSize, "%c%f%c%f",
      		     OUTPUT_DATA_CSV_SEP, latitude,
      		     OUTPUT_DATA_CSV_SEP, longitude);
      if(len >= csvBufferSize) { writeCSVData = false; }
    }
    else
    {
      if(csvBufferSize >= 3)
      {
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = OUTPUT_DATA_CSV_SEP;
	*psCSVBuffer++ = '\0';
      }
      else { writeCSVData = false; }
    }
  }

  if(actionOnSensorDone || actionOnGPSDone) { saveCurrentCNSSRFDataFrame(); }
  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);

  // Write CSV data
  if(writeCSVData && (actionOnSensorDone || actionOnGPSDone))
  {
    sdcard_fwrite_string(&this->_output_data_csv_file, this->_output_data_csv_buffer);
    sdcard_fsync(        &this->_output_data_csv_file);
  }

  data_write_failed:

  // Network
  if(this->Network)
  {
    if(this->Network->itsTime()) { this->ActionOnNetwork(); }

    // Maybe change the network period for the next time if a sensor is in alarm
    this->Network->useSensorInAlarmPeriod(aSensorIsInAlarm);
    if(aSensorIsInAlarm)
    {
      log_debug(logger, "At least one sensor is in alarm.");
    }
  }

  exit:
  return;
}


/**********************************************************/
/* Phase de sommeil ***************************************/
/**********************************************************/
void ConnecSenS::EndOfExecution()
{
  ts2000_t ts;
  Sensor  *pvSensor;
  ts2000_t tsNextTime = UINT32_MAX;

  setPowerForSleepMode();

  // Get timestamp of the next wakeup
  if(this->_enableGPS && (ts = this->GPS.nextTime()) && ts < tsNextTime) { tsNextTime = ts; }
  for(uint8_t i = 0; i < this->NumberOfSensors; i++)
  {
    pvSensor = this->_sensors[i];
    if((ts   = pvSensor->nextTime()) && ts < tsNextTime) { tsNextTime = ts; }

    // And while we're at it make sure that the sensor is closed
    if(!pvSensor->needsToBeConstantlyOpened()) { pvSensor->close(); }
  }
  if((ts = this->Network        ->nextTime()) && ts < tsNextTime) { tsNextTime = ts; }
  if((ts = this->_sendConfigTimer.nextTime()) && ts < tsNextTime) { tsNextTime = ts; }

  this->_timeSyncMethodChanged = false;
  rtc_set_is_not_first_run();
  this->myBuffer.clean();

  // Go to sleep
  Sleep(tsNextTime);
}

/**
 * Go to sleep, if next wakeup is in the future.
 *
 * @param[in] tsWakeup timestamp of the next scheduled wakeup,
 *                     in seconds since 2001/01/01T00:00:00.
 */
void ConnecSenS::Sleep(ts2000_t tsWakeup)
{
  Datetime  dt;
  ts2000_t  tsNow;

  datetime_from_sec_2000(&dt, tsWakeup);
  tsNow = rtc_get_date_as_secs_since_2000();

  if(tsWakeup > tsNow)
  {
    // Flush some files
    datalog_cnssrf_sync();

    if(tsWakeup != this->_lastWakeupTs2000)
    {
      log_info(logger,
	       "Next wakeup is scheduled for %04d/%02d/%02d %02d:%02d:%02d"
	       "\r\n---------------------------------------------------------------------",
	       dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
    }

    // We are going to sleep
    status_ind_set_status(STATUS_IND_ASLEEP);

    CNSSInt::instance()->sleep();

    // Start wakeup timer
    timer_stop(      &this->_wakeupTimer);
    timer_set_period(&this->_wakeupTimer, tsWakeup - tsNow, TIMER_TU_SECS);
    timer_start(     &this->_wakeupTimer);

    // Go to deep sleep.
    cnsslog_sleep();
    if(!timer_has_timed_out(&this->_wakeupTimer))
    {
      pwrclk_stop();
    }

    // We woke up
    cnsslog_wakeup();
    timer_stop(&this->_wakeupTimer);
    status_ind_set_status(STATUS_IND_AWAKE);
  }
  else
  {
    // Do not go to sleep, execute now.
    log_info(logger,
	       "Next wakeup was scheduled for %04d/%02d/%02d %02d:%02d:%02d; do not go to sleep.",
	       dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
  }

  board_watchdog_reset();
  this->_lastWakeupTs2000 = tsWakeup;
  RefreshTimestamp();
}

/**********************************************************/
/* Gestion des Sensors ************************************/
/**********************************************************/
void ConnecSenS::ActionOnSensor(Sensor *pvSensor)
{
  if(pvSensor && pvSensor->open())
  {
    if(pvSensor->read() && !appendSensorDataToCurrentCNSSRFDataFrame(pvSensor))
    {
      log_error(logger, "Failed to write data for sensor: %s.", pvSensor->name());
    }
    pvSensor->close();

    // Clear the alarm has just changed status.
    // To avoid the sensor being read a second time because if alarm status change.
    pvSensor->alarmStatusHasJustChanged(true);
  }
}

/**********************************************************/
/* Gestion de l'interface réseau **************************/
/**********************************************************/
void ConnecSenS::ActionOnNetwork()
{
  ts2000_t since;
  uint32_t max_payload_size;
  bool     has_more_data;
  bool     success = true;

  if(!cnssrf_data_frame_is_empty(&this->_cnssrfSendDataFrame))
  {
    if(this->Network->isBusy())
    {
      log_warn(logger, "Cannot send data we still are sending the previous data.");
      return;
    }
    else
    {
      // There is a discrepancy between our state and the network state.
      // Forget the data that are currently set to be sent.
      cnssrf_data_frame_clear(&this->_cnssrfSendDataFrame);
    }
  }

  board_watchdog_reset();
  status_ind_set_status(STATUS_IND_RF_SEND_TRYING);

  // Get data to send
  // Only look for last week's data.
  since            = rtc_get_date_as_secs_since_2000();
  since            = since > NB_SECS_IN_A_WEEK ? since - NB_SECS_IN_A_WEEK : 0;
  max_payload_size = this->Network->maxPayloadSize();
  log_info(logger, "Network payload is %d bytes max.", max_payload_size);
  if(!datalog_cnssrf_get_frame(&this->_cnssrfSendDataFrame,
			       max_payload_size,
			       since,
			       &has_more_data))
  {
    log_error(logger, "Failed to get data to send from datalog.");
    success = false;
    goto exit;
  }
  board_watchdog_reset();

  // Do we have data to send?
  if(cnssrf_data_frame_is_empty(&this->_cnssrfSendDataFrame))
  {
    log_info(logger, "No data to send.");
    success = false;
    goto exit;
  }

  // Print data to log
  // Transform the binary data to an hex string
  binToHex((char *)this->myBuffer.getBufferPtr(),
	   cnssrf_data_frame_data(&this->_cnssrfSendDataFrame),
	   cnssrf_data_frame_size(&this->_cnssrfSendDataFrame),
	   true);

  // Write to log
  log_info(logger, "Amount of data to send: %d bytes.", cnssrf_data_frame_size(&this->_cnssrfSendDataFrame));
  log_info(logger, "CNSSRF payload: %s", (char *)this->myBuffer.getBufferPtr());
  log_info(logger, "Datalog has %smore data available to send for the specified time frame.",
	   has_more_data ? "" : "no ");

  // Send data
  success = this->Network->send(cnssrf_data_frame_data(&this->_cnssrfSendDataFrame),
				cnssrf_data_frame_size(&this->_cnssrfSendDataFrame),
				ClassNetwork::SEND_OPTION_REQUEST_ACK |
				ClassNetwork::SEND_OPTION_SLEEP_WHEN_DONE);

  exit:
  if(!success) { status_ind_set_status(STATUS_IND_RF_SEND_KO); }
  return;
}

/**
 * Called when a network interface send data state has changed.
 *
 * @param[in] pvInterface the network interface source of the event.
 * @param[in] state       the new send state.
 */
void ConnecSenS::sendStateChanged(ClassNetwork           *pvInterface,
				  ClassNetwork::SendState state)
{
  UNUSED(pvInterface);

  if( state != ClassNetwork::SEND_STATE_SENT &&
      state != ClassNetwork::SEND_STATE_FAILED) { return; }

  if(state == ClassNetwork::SEND_STATE_SENT)
  {
    // Signal that the data retrieved from the datalog have been sent
    // so that the datalog can be updated.
    datalog_cnssrf_frame_has_been_sent();

    status_ind_set_status(STATUS_IND_RF_SEND_OK);
  }
  else
  {
    status_ind_set_status(STATUS_IND_RF_SEND_KO);
  }
  // Clear send data frame to indicate that we are done with sending data.
  cnssrf_data_frame_clear(&this->_cnssrfSendDataFrame);
}


/**
 * Look at the reset source andwrite to log and send some of them.
 */
void ConnecSenS::lookAtResetSource()
{
  BoardResetFlags flags = board_reset_source_flags(true);

  if(flags == BOARD_RESET_FLAG_NONE) { goto exit; }

  // Start new CNSSRF frame
  if(!startNewCNSSRFDataFrame()) { goto error_exit; }

  // Write software version
  if(!cnssrf_dt_system_write_app_version(&this->_cnssrfDataFrame,
					 VERSION_MAJOR,
					 VERSION_MINOR,
					 VERSION_REVISION,
					 STRINGIFY(VERSION_SUFFIX),
					 VERSION_REV_ID_UNIT32)) { goto error_exit; }

  if(flags & BOARD_RESET_FLAG_BOR)
  {
    log_info(logger, "Node reseted because powered on or BOR.");
    cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					CNSSRF_DT_SYS_RESET_SOURCE_BOR);
  }
  else
  {
    if(flags & BOARD_RESET_FLAG_PIN)
    {
      log_info(logger, "Node reseted because user pushed the reset button.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_PIN);
    }
    if(flags & BOARD_RESET_FLAG_SOFTWARE)
    {
      log_info(logger, "Node reseted following a normal software request.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_SOFTWARE);
    }
    if(flags & BOARD_RESET_FLAG_SOFTWARE_ERROR)
    {
      log_info(logger, "Node reseted following a software request for an error.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_SOFTWARE_ERROR);
    }
    if(flags & BOARD_RESET_FLAG_WATCHDOG)
    {
      log_info(logger, "Node reseted by watchdog.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_WATCHOG);
    }
    if(flags & BOARD_RESET_FLAG_INDEPENDENT_WATCHDOG)
    {
      log_info(logger, "Node reseted by independent watchdog.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_INDEPENDENT_WATCHDOG);
    }
    if(flags & BOARD_RESET_FLAG_LOW_PWR_ERROR)
    {
      log_info(logger, "Node reseted because of low power mode usage error.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_LOW_POWER_ERROR);
    }
    if(flags & BOARD_RESET_FLAG_OPTION_BYTES_LOADING)
    {
      log_info(logger, "Node reseted to reload µC option bytes.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_OPTION_BYTES_LOADING);
    }
    if(flags & BOARD_RESET_FLAG_FIREWALL)
    {
      log_info(logger, "Node reseted by µC firewall.");
      cnssrf_dt_system_write_reset_source(&this->_cnssrfDataFrame,
					  CNSSRF_DT_SYS_RESET_SOURCE_FIREWALL);
    }
  }

  // Write CNSSRF data to datalog.
  saveCurrentCNSSRFDataFrame();
  goto exit;

  error_exit:
  log_error(logger, "Failed to write data to CNSSRF frame.");

  exit:
  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);
}


/**
 * Set the board power configuration for sleep mode
 */
void ConnecSenS::setPowerForSleepMode()
{
  Sensor::Power power = Sensor::POWER_NONE;

  for(uint16_t i = 0; i < this->NumberOfSensors; i++)
  {
    power |= this->_sensors[i]->powerConfigSleep();
  }
  board_set_power((BoardPower)power);
}


/**
 * Set time using GPS data.
 *
 * @param[in] blocking is the operation a blocking one (true) or not (false)?
 *
 * @return true  if the time has been set using GPS data.
 * @return false otherwise.
 */
bool ConnecSenS::setRTCUsingGPS(bool blocking)
{
  ts2000_t ts_now, ts_gps;
  uint32_t diff_abs;
  bool     diff_plus;
  bool     res = false;

  if(this->_enableGPS)
  {
    status_ind_set_status(STATUS_IND_GPS_FIX_TRYING);
    log_info(logger, "GPS synchronisation...");

    // Get data from GPS
    log_info(logger, "Opening GPS...");
    this->GPS.open();
    log_info(logger, "GPS opened; waiting for data...");
    res = this->GPS.refresh(blocking);
    this->GPS.close();
    log_info(logger, "GPS closed.");

    if(!res)
    {
      // No fix
      log_error(logger, "GPS synchronisation failed.");
      status_ind_set_status(STATUS_IND_GPS_FIX_KO);
    }
    else
    {
      // Fix
      ts_now   = rtc_get_date_as_secs_since_2000();
      ts_gps   = datetime_to_timestamp_sec_2000(&this->GPS.time());
      diff_abs = (diff_plus = (ts_now >= ts_gps)) ? ts_now - ts_gps : ts_gps - ts_now;

      rtc_set_date(&this->GPS.time());
      RefreshTimestamp();
      updatePeriodicTaskNextTime();
      log_info(logger, "RTC time drift: %c%u seconds.", diff_plus ? '+' : '-', diff_abs);

      // Output GPS info to log file
      log_info(logger, "GPS time: %04d-%02d-%02d %02d:%02d:%02d",
	       this->GPS.time().year,  this->GPS.time().month,   this->GPS.time().day,
	       this->GPS.time().hours, this->GPS.time().minutes, this->GPS.time().seconds);
      log_info(logger, "GPS position: (lat=%f; long=%f)", this->GPS.location().latitude, this->GPS.location().longitude);

      // User friendly GPS fix indication: make LED2 blink
      status_ind_set_status(STATUS_IND_GPS_FIX_OK);
    }
  }
  else
  {
    log_warn(logger, "GPS synchronisation is disabled.");
  }

  return res;
}

void ConnecSenS::RefreshTimestamp()
{
  rtc_get_date(&this->currentTimestamp);
}

/**
 * Set the real time clock time to the manual one defined in the configuration file.
 *
 * @return true  if RTC has been set to the manual time.
 * @return false if the current time has been kept because the manual time seems to be invalid.
 */
bool ConnecSenS::setRTCUsingManualTime()
{
  bool timeSet = false;

  // Check that the manual date set in the configuration file has changed since last time
  // manual values have been used.
  if(!config_monitor_manual_time_has_changed(&this->manualTimestamp, true))
  {
    log_debug(logger, "Manual time has not been changed.");
    goto exit;
  }
  log_info(logger, "Manual time has been changed.");

  // Set time using manual values
  if( this->manualTimestamp.year    >= CURRENT_YEAR_MIN                       &&
      this->manualTimestamp.month   > 0  && this->manualTimestamp.month <= 12 &&
      this->manualTimestamp.day     > 0  && this->manualTimestamp.day   <= 31 &&
      this->manualTimestamp.hours   < 24 &&
      this->manualTimestamp.minutes < 60 &&
      this->manualTimestamp.seconds < 60)
  {
    log_info(logger, "Use time set in configuration: %04d-%02d-%02d %02d:%02d:%02d.",
	     this->manualTimestamp.year,  this->manualTimestamp.month,   this->manualTimestamp.day,
	     this->manualTimestamp.hours, this->manualTimestamp.minutes, this->manualTimestamp.seconds);
    timeSet = rtc_set_date(&this->manualTimestamp);
    RefreshTimestamp();
    updatePeriodicTaskNextTime();
  }
  else
  {
    log_info(logger, "No time is set in the configuration, or it's too old too, or it's invalid.");
  }

  exit:
  return timeSet;
}

/**
 * Update the next time for the periodic task. For example if the current RTC time has changed.
 */
void ConnecSenS::updatePeriodicTaskNextTime()
{
  uint8_t i;

  for(i = 0; i < this->NumberOfSensors; i++) { this->_sensors[i]->setNextTime(); }
  if(this->Network) { this->Network->setNextTime(); }
  this->GPS             .setNextTime();
  this->_sendConfigTimer.setNextTime();

  this->_lastWakeupTs2000 = 0;  ///< Force setting up next wakup timer.
}


/**
 * Get the period, in seconds, from a JSON object.
 *
 * @param[in]  obj          the JSON object.
 * @param[in]  defaultValue the value to return if there is no period parameter in the object
 * @param[out] pbOk         is set to true if there was a period parameter.
 *                          Set to false otherwise.
 *                          Can be NULL if we are not interested i this information.
 * @param[in]  psBase       the parameter base name. MUST be NOT NULL and NOT empty.
 *
 * @return the period, in seconds.
 */
uint32_t ConnecSenS::getPeriodSec(const JsonObject &obj, uint32_t defaultValue, bool *pbOk, const char *psBase)
{
  uint8_t n;
  char   *pc, *pcUnit;
  char    name[32];
  bool    ok;

  if(!pbOk) { pbOk = &ok; }
  *pbOk = true;

  // Copy the base name and get a pointer to the first unit character
  for(pc = name, n  = sizeof(name) - 4; *psBase && n; n--) { *pc++ = *psBase++; }
  if(*psBase) { *pbOk = false; return defaultValue; }
  pcUnit = pc;

  // Try psBaseSec
  *pc++ = 'S'; *pc++ = 'e'; *pc++ = 'c'; *pc = '\0';
  if(obj[name].success()) { return obj[name].as<uint32_t>(); }

  // Try psBaseMn
  pc = pcUnit;
  *pc++ = 'M'; *pc++ = 'n'; *pc = '\0';
  if(obj[name].success()) { return (uint32_t)(obj[name].as<float>() * 60.0); }

  // Try psBaseHr
  pc = pcUnit;
  *pc++ = 'H'; *pc++ = 'r'; *pc = '\0';
  if(obj[name].success()) { return (uint32_t)(obj[name].as<float>() * 3600.0); }

  // Try psBaseDay
  pc = pcUnit;
  *pc++ = 'D'; *pc++ = 'a'; *pc++ = 'y'; *pc = '\0';
  if(obj[name].success()) { return (uint32_t)(obj[name].as<float>() * 86400.0); }

  *pbOk = false;
  return defaultValue;
}

/**
 * Read configuration file and set up all the objects affected by this configuration.
 *
 * @post this->myBuffer contains the configuration file's data.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ConnecSenS::loadConfig()
{
  File          file;
  uint32_t      size;
  uint8_t       i, count;
  float         f;
  const char   *psValue;
  bool          res = true;

  /* Récupération du contenu du fichier config */
  sdcard_fopen(     &file, CONFIG_FILE_NAME, FILE_OPEN_OR_CREATE | FILE_READ);
  if(!sdcard_fread( &file, this->myBuffer.getBufferPtr(), (size = sdcard_fsize(&file))))
  {
    log_error(logger, "Failed to read configuration file '%s'.", CONFIG_FILE_NAME);
    return false;
  }
  sdcard_fclose(&file);
  this->myBuffer.setWriteCursor(size); // So that we can append a null character to make a string.
  this->myBuffer.push('\0');
  this->myBuffer.setWriteCursor(size); // Do not count the ending null character

  // Go through JSON file
  StaticJsonBuffer<CONNECSENS_BUFFER_SIZE> jsonBuffer;
  JsonObject&   object = jsonBuffer.parseObject(this->myBuffer.getBufferPtr());
  if(object == JsonObject::invalid())
  {
    log_error(logger, "Configuration file syntax error.");
    return false;
  }

  // Get logs configuration
  if(object["logs"].success())
  {
    JsonObject& logs = object["logs"];

    // Set default log level
    logger_set_default_level_using_string(logs["defaultLevel"].as<const char *>());

    // Log to serial output?
    cnsslog_enable_serial_logging(logs["serial"].as<const char *>());
  }

  // Get debug configuration
  if(object["debug"].success())
  {
    JsonObject& debug = object["debug"];
    leds_enable_debug(debug["useInternalLEDs"].as<bool>());
    buzzer_set_enable(debug["useBuzzer"]      .as<bool>());
    if(debug["outputCSVData"].success())
    {
      this->_output_data_to_csv = debug["outputCSVData"].as<bool>();
    }
  }

  // Get node's name
  this->DeviceName[0] = '\0';
  if(object["name"].success())
  {
    strlcpy(this->DeviceName, object["name"].as<const char*>(), CONNECSENS_NAME_MAX_SIZE);
  }
  if(!this->DeviceName[0])
  {
    log_error(logger, "You must specify a node's name, less than %u characters long, using configuration parameter 'name'.",
	      CONNECSENS_NAME_MAX_SIZE - 1);
    res = false;
  }

  // Get the experiment's name
  this->_experimentName[0] = '\0';
  if(object["experimentName"].success())
  {
    strlcpy(this->_experimentName,
	    object["experimentName"].as<const char*>(),
	    CONNECSENS_EXPERIMENT_NAME_SIZE);
  }
  if(!this->_experimentName[0])
  {
    log_error(logger, "You must specify an experiment's name, less than %u characters long, using configuration parameter 'experimentName'.",
	      CONNECSENS_EXPERIMENT_NAME_SIZE - 1);
    res = false;
  }

  // Get node's unique identifier, if one is set.
  setUniqueId(object["uniqueId"].as<const char*>());

  if((f = object["batteryLowV"].as<float>()) != 0.0) { NodeBattery::instance()->setVoltageLowV(f); }
  _sendConfigTimer.setPeriodSec(getPeriodSec(object, 0, NULL, "sendConfigPeriod"));

  // Add geopgraphical position to each RF frame?
  this->_addGeoPosToEachRFFrame = ADD_GEOPOS_TO_EACH_RF_FRAME_DEFAULT_VALUE;
  if(object["addGeoPosToAllReadings"].success())
  {
    this->_addGeoPosToEachRFFrame = object["addGeoPosToAllReadings"].as<bool>();
  }

  psValue = object["workingMode"].as<const char *>();
  if(psValue && strcasecmp(psValue, "campaignRange") == 0) { this->_workingMode = WMODE_CAMPAIGN_RANGE; }
  else                                                     { this->_workingMode = WMODE_NORMAL;         }

  // Configure interruptions
  CNSSInt::instance()->setConfiguration(object["interruptions"]);

  // Configure buzzer
  if(object["buzzer"].success())
  {
    if((psValue = object["buzzer"]["activationExtPin"].as<const char *>()) && *psValue &&
	!buzzer_configure_on_off(psValue))
    {
      log_error(logger, "Unknown buzzer's activationExtPin: '%s'.", psValue);
      res = false;
    }
  }

  // Initialise sensors
  this->NumberOfSensors = 0;
  count = object["sensors"].size();
  for(i = 0; i < count; i++)
  {
    const char *psType   = object["sensors"][i]["type"].as<const char *>();
    Sensor     *pvSensor = SensorFactory::getNewSensorInstanceUsingType(psType);
    if(!pvSensor)
    {
      log_error(logger, "Unknown sensor type: %s", psType);
      continue;
    }

    // For now the sensor's ConnecSenS RF Data Channel identifier
    // built using it's position in the sensor's list.
    // We should offer the possibility, or even maybe impose,
    // that the channel number is set through a configuration parameter.
    pvSensor->setCNSSRFDataChannel((CNSSRFDataChannel)(this->NumberOfSensors + CNSSRF_DATA_CHANNEL_1));

    // Set sensor's configuration using JSON
    if(!pvSensor->setConfiguration(object["sensors"][i]))
    {
      log_error(logger, "Failed to configure sensor number %d of type '%s'", i, psType);
      delete pvSensor; pvSensor = NULL;
      continue;
    }

    // Add the sensor to the sensor's list
    if(this->NumberOfSensors >= CONNECSENS_NB_SENSOR_MAX)
    {
      log_error(logger, "Too many sensors are defined in the configuration; We can handle %d sensors at most.", CONNECSENS_NB_SENSOR_MAX);
      delete pvSensor; pvSensor = NULL;
      break;
    }
    this->_sensors[this->NumberOfSensors++] = pvSensor;
  }

  // Network
  psValue = object["network"]["type"].as<const char*>();
  if(     strcasecmp(psValue, "LoRaWAN") == 0) { this->Network = new ClassLoRaWAN;  }
  else if(strcasecmp(psValue, "SigFox")  == 0) { this->Network = new ClassSigFox;   }
  else                                         { this->Network = new ClassNwkSimul; }
  if(!this->Network->setConfiguration(object["network"]))
  {
    log_error(logger, "Failed to configure network");
    delete this->Network; this->Network = NULL;
    res = false;
  }

  // Get time parameters
  this->_enableGPS = false;
  psValue = object["time"]["syncMethod"].as<const char *>();
  if(!psValue || !*psValue)
  {
    log_error(logger, "You must specify the 'syncMethod' used to synchronise the node's time. ");
    res = false;
  }
  else
  {
    if(strcasecmp(psValue, "GPS") == 0)
    {
      if(object["time"]["GPS"].success())
      {
	if(this->GPS.setConfiguration(object["time"]["GPS"])) { this->_enableGPS = true; }
	else
	{
	  log_error(logger, "Failed to configure GPS.");
	  res = false;
	}
      }
      else
      {
	log_error(logger, "Configured to use GPS but cannot find 'GPS' json object in configuration.");
	res = false;
      }
    }
    else if(strcasecmp(psValue, "manual")    == 0 ||
	strcasecmp(    psValue, "manualUTC") == 0) { }  // Do nothing
    else
    {
      log_error(logger, "Unknown time synchronisation method: '%s'.", psValue);
      res = false;
    }
    if((this->_timeSyncMethodChanged = config_monitor_time_sync_method_has_changed(psValue, true, true)))
    {
      log_warn(logger, "Time synchronisation method has been changed to %s.", psValue);
    }
  }
  this->manualTimestamp.hours   = object["time"]["manualUTC"]["hours"]  .as<uint8_t>();
  this->manualTimestamp.minutes = object["time"]["manualUTC"]["minutes"].as<uint8_t>();
  this->manualTimestamp.seconds = object["time"]["manualUTC"]["seconds"].as<uint8_t>();
  this->manualTimestamp.day     = object["time"]["manualUTC"]["day"]    .as<uint8_t>();
  this->manualTimestamp.month   = object["time"]["manualUTC"]["month"]  .as<uint8_t>();
  this->manualTimestamp.year    = object["time"]["manualUTC"]["year"]   .as<uint16_t>();

  if(!this->_enableGPS) { this->_addGeoPosToEachRFFrame = false; }

  return res;
}

/**
 * Check if the configuration has changed, and if so then take some actions.
 */
void ConnecSenS::detectConfigurationChange()
{
  File     file;
  uint32_t size;

  // Read the configuration file's contents.
  if(!sdcard_fopen(&file, CONFIG_FILE_NAME, FILE_OPEN | FILE_READ))
  {
    log_error(logger, "Failed to open configuration file '%s' in read mode", CONFIG_FILE_NAME);
    return;
  }
  size = sdcard_fsize(&file);
  this->myBuffer.clean();
  if(!sdcard_fread(   &file, this->myBuffer.getBufferPtr(), size))
  {
    log_error(logger, "Failed to read configuration file '%s''s contents.", CONFIG_FILE_NAME);
    sdcard_fclose(&file);
    return;
  }
  sdcard_fclose(  &file);

  // Look for changes
  if(config_monitor_config_has_changed(this->myBuffer.getBufferPtr(), size, false))
  {
    // Set current configuration has the reference
    config_monitor_save_config(        this->myBuffer.getBufferPtr(), size);

    // The configuration file has changed
    log_warn(logger, "Configuration file has changed.");

    // Write the interesting parts of the configuration in the data to send over RF.
    if(writeCNSSRFFramesWithConfig()) { log_info( logger, "System configuration will be sent over RF");             }
    else                              { log_error(logger, "Failed to create frame to send configuration over RF."); }

    // Remove all the sensors' states
    if(Sensor::removeAllStates()) { log_info( logger, "All sensor state files have been deleted."); }
    else                          { log_error(logger, "Failed to remove sensors' states.");         }
  }
  else
  {
    log_debug(logger, "Configuration file has not changed.");
  }
}

/**
 * Write CNSSRF frame(s) with the interesting part of the configuration.
 *
 * @return true on success.
 * @return false otherwise.
 */
bool ConnecSenS::writeCNSSRFFramesWithConfig()
{
  CNSSRFDataChannel channel;
  Sensor           *pvSensor;
  uint8_t           i;
  char              buffer[10];

  // Clear RF frame object
  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);

  // First write the node parameters
  if( !appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					     CNSSRF_CONFIG_PARAM_NAME,
					     this->DeviceName)       ||
      !appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					     CNSSRF_CONFIG_PARAM_TYPE,
					     CONNECSENS_NODE_TYPE)   ||
      !appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					     CNSSRF_CONFIG_PARAM_UNIQUE_ID,
					     uniqueId())             ||
      !appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					     CNSSRF_CONFIG_PARAM_FIRMWARE_VERSION,
					     VERSION_STR)            ||
      !appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					     CNSSRF_CONFIG_PARAM_EXPERIMENT_NAME,
					     this->_experimentName))
  { goto error_exit; }

  // Write integer values
  sprintf(buffer, "%lu", this->_sendConfigTimer.periodSec());
  if(!appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					    CNSSRF_CONFIG_PARAM_SEND_CONFIG_PERIOD_SEC,
					    buffer))
  { goto error_exit; }
  sprintf(buffer, "%lu", this->GPS.periodSec());
  if(!appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					    CNSSRF_CONFIG_PARAM_GPS_READ_PERIOD_SEC,
					    buffer))
  { goto error_exit; }

  // Write the sensor configuration parameters
  for(i = 0; i < this->NumberOfSensors; i++)
  {
    pvSensor = this->_sensors[i];
    channel  = pvSensor->cnssrfDataChannel();

    // Write parameter values
    if( !appendConfigValueToCurrentCNSSRFFrame(channel,
					       CNSSRF_CONFIG_PARAM_NAME,
					       pvSensor->name())     ||
	!appendConfigValueToCurrentCNSSRFFrame(channel,
					       CNSSRF_CONFIG_PARAM_TYPE,
					       pvSensor->type())     ||
        !appendConfigValueToCurrentCNSSRFFrame(channel,
					       CNSSRF_CONFIG_PARAM_UNIQUE_ID,
					       pvSensor->uniqueId()) ||
	!appendConfigValueToCurrentCNSSRFFrame(channel,
					       CNSSRF_CONFIG_PARAM_FIRMWARE_VERSION,
					       pvSensor->firmwareVersion()))
    { goto error_exit; }

    // Write interger values
    sprintf(buffer, "%lu", pvSensor->periodSec());
    if(!appendConfigValueToCurrentCNSSRFFrame(CNSSRF_DATA_CHANNEL_NODE,
					      CNSSRF_CONFIG_PARAM_SENSOR_READ_PERIOD_SEC,
					      buffer))
    { goto error_exit; }
  }

  // Save frame
  saveCurrentCNSSRFDataFrame();
  cnssrf_data_frame_clear(&this->_cnssrfDataFrame);
  return true;

  error_exit:
  return false;
}

/**
 * Append a configuration parameter value to the current CNSSRF frame.
 *
 * If there is not enough space left in the current frame to receive the value then
 * the current frame is saved to the datalog and a new one is started with the current
 * Data Channel and the same timestamp.
 *
   @param[in] channel the data channel the parameter belongs to.
 * @param[in] paramId the parameter's identifier.
 * @param[in] psValue the parameter's value. Can be NULL or EMPTY.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ConnecSenS::appendConfigValueToCurrentCNSSRFFrame(CNSSRFDataChannel   channel,
						       CNSSRFConfigParamId paramId,
						       const char         *psValue)
{
  uint32_t hash;

  if(!psValue || !*psValue) { return true; }

  // Check if enough space left
  if(!cnssrf_data_frame_has_enough_space_left_for(&this->_cnssrfDataFrame, 35))  // 35 = 1 Data Channel byte + 1 Data Type + 2 Config Data Type header bytes + 31 value string bytes max.
  {
    // Save current frame and start a new one
    channel = cnssrf_data_frame_current_data_channel(&this->_cnssrfDataFrame);
    saveCurrentCNSSRFDataFrame();
    cnssrf_data_frame_clear(&this->_cnssrfDataFrame);
  }

  // If the frame is empty then write the channel and the timestamp
  if(cnssrf_data_frame_is_empty(&this->_cnssrfDataFrame))
  {
    if( !cnssrf_data_frame_set_current_data_channel(     &this->_cnssrfDataFrame, CNSSRF_DATA_CHANNEL_NODE) ||
    	!cnssrf_dt_timestamp_utc_write_datetime_to_frame(&this->_cnssrfDataFrame,
    							 &this->currentTimestamp))
        { goto error_exit; }

    // Add the configuration hash if we can get it.
    if(config_monitor_hash_mm3_32(&hash) &&
	!cnssrf_dt_hash_write_config_mm3hash32(&this->_cnssrfDataFrame, hash))
    { goto error_exit; }
  }

  // Append
  return cnssrf_data_frame_set_current_data_channel(&this->_cnssrfDataFrame, channel) &&
      cnssrf_dt_config_write_parameter_value(       &this->_cnssrfDataFrame, paramId, psValue);

  error_exit:
  return false;
}


/**
 * Open the CSV data output file.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ConnecSenS::openDataOutputCSVFile()
{
  uint32_t h, l, size;
  char    *psBuffer;
  char     filename[64];
  char     buffer[OUTPUT_DATA_CSV_BUFFER_SIZE];
  char     hashStr[9];
  bool     ok;
  bool     res = this->_output_data_to_csv;

  if(!res || this->_output_data_csv_file_is_opened) { goto exit; }

  // Build the CSV header
  strcpy(this->_output_data_csv_buffer,
	 "nodeTimestampUTC" OUTPUT_DATA_CSV_SEP_STR
	 "nodeBattVoltageV" OUTPUT_DATA_CSV_SEP_STR);
  l        = strlen(this->_output_data_csv_buffer);
  size     = sizeof(this->_output_data_csv_buffer) - l;
  psBuffer = this->_output_data_csv_buffer         + l;
  for(h = 0; h < this->NumberOfSensors; h++)
  {
    Sensor *pvSensor = this->_sensors[h];
    l = pvSensor->csvHeader(psBuffer, size, Sensor::CSV_HEADER_PREPEND_NAME);
    if(!l) { goto error_exit; }
    psBuffer += l;
    size      -= l;
    if(size <= 1) { goto error_exit; }
    *psBuffer++ = OUTPUT_DATA_CSV_SEP;
    size--;
  }
  l = strlcpy(psBuffer, "nodeGeoPosLatitude" OUTPUT_DATA_CSV_SEP_STR "nodeGeoPosLongitude", size);
  if(l >= size) { goto error_exit; }
  l = strlen(this->_output_data_csv_buffer);

  // Get hash from CSV header
  h = mm3_32_cnss((const uint8_t *)this->_output_data_csv_buffer, l);

  // Build the file name
  strcpy(  filename, OUTPUT_DATA_CSV_DIR "/");
  if(this->Network && *this->Network->deviceIdAsString())
  {
    strcat(filename, this->Network->deviceIdAsString());
    strcat(filename, "-");
  }
  else
  {
    log_warn(logger, "No network set or no device identifier on this network; CSV file name will not include network identifier.");
  }
  sprintf(hashStr, "%08X", (unsigned int)h);
  strcat(filename, hashStr);
  strcat(filename, OUTPUT_DATA_CSV_FILE_EXT);

  // Open the file
  if(sdcard_exists(filename))
  {
    if(!sdcard_fopen(&this->_output_data_csv_file, filename, FILE_READ_WRITE))
    {
      log_error(logger, "Failed to open CSV data output file '%s'.", filename);
      goto error_exit;
    }

    // Compare CSV headers
    sdcard_read_line(&this->_output_data_csv_file, buffer, sizeof(buffer), true, &ok);
    if(!ok)
    {
      log_error(logger, "Failed to read CSV header from CSV data output file '%s'.", filename);
      sdcard_fclose(&this->_output_data_csv_file);
      goto error_exit;
    }
    if(strcmp(this->_output_data_csv_buffer, buffer) != 0)
    {
      log_warn(logger, "CSV data output file '%s' already exist with a different CSV header.", filename);
      log_warn(logger, "Delete previous CSV data output file '%s'.", filename);
      sdcard_fclose(    &this->_output_data_csv_file);
      if(!sdcard_remove(filename))
      {
	log_error(logger, "Failed to remove CSV data output file '%s'.", filename);
	goto error_exit;
      }
    }
    else
    {
      // Headers are the same, seek to the end of the file to append data
      if(!sdcard_fseek_abs(&this->_output_data_csv_file, sdcard_fsize(&this->_output_data_csv_file)))
      {
	log_error(logger, "Failed to seek to the end of the  CSV data output file '%s'.", filename);
	sdcard_fclose(&this->_output_data_csv_file);
	goto error_exit;
      }
    }
  }
  if(!sdcard_exists(filename))
  {
    // CSV file does not exist. Create it and write CSV header
    if(!sdcard_fopen(&this->_output_data_csv_file, filename, FILE_READ_WRITE | FILE_OPEN_OR_CREATE))
    {
      log_error(logger, "Failed to create CSV data output file '%s'.", filename);
      goto error_exit;
    }
    else { log_info(logger, "CSV data output file '%s' has been created.", filename); }
    if( !sdcard_fwrite_string(&this->_output_data_csv_file, this->_output_data_csv_buffer))
    {
      log_error(logger, "Failed to write CSV header to CSV data output file '%s'.", filename);
      sdcard_fclose(&this->_output_data_csv_file);
      goto error_exit;
    }
  }

  this->_output_data_csv_file_is_opened = true;

  exit:
  return res;

  error_exit:
  return false;
}

/**
 * Close the CSV data output file.
 */
void ConnecSenS::closeDataOutputCSVFile()
{
  if(this->_output_data_csv_file_is_opened)
  {
    sdcard_fclose(&this->_output_data_csv_file);
    this->_output_data_csv_file_is_opened = false;
  }
}


/**
 * Start a new CSV data line in the CSV data output file.
 *
 * @param[in] pvDt The Datetime object to use as timestamp. If NULL then use current timestamp.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ConnecSenS::startDataOutputCSVLine(const Datetime *pvDt)
{
  bool res = false;

  if(!this->_output_data_to_csv || !this->_output_data_csv_file_is_opened) { goto exit; }

  // Write node values
  if(!pvDt) { pvDt = &this->currentTimestamp; }
  sprintf(this->_output_data_csv_buffer,
	  "%s%04d-%02d-%02d %02d:%02d:%02d%c%1.3f",
	  OUTPUT_DATA_CSV_EOL_STR,
	  pvDt->year,  pvDt->month,   pvDt->day, pvDt->hours, pvDt->minutes, pvDt->seconds,
	  OUTPUT_DATA_CSV_SEP,
	  NodeBattery::instance()->voltageV());
  res = true;

  exit:
  return res;
}


void ConnecSenS::startCampaignRange()
{
  File          file;
  char          buffer[24];
  char          outCSVFilename[32];
  char          latitudeStr[16], longitudeStr[16];
  char          rssiStr[8],      snrStr[8];
  char         *pcMyBuffer;
  ClassLoRaWAN::Datarate dataRate;
  ClassPeriodic timer;
  uint8_t       sentCounter          = 0;
  bool          hasGPSData           = false;
  bool          networkHasBeenOpened = false;
  const char   *psDevEUIStr          = "";

  log_info(logger, "Start campaignRange working mode");

  // Turn on the GPS
  this->GPS.open();
  log_info(logger, "GPS is ON");

  // Check that we have a network object set
  if(this->Network)
  {
    psDevEUIStr = this->Network->deviceIdAsString();

    // Use it's period as our working period.
    timer.setPeriodSec(this->Network->periodSec());
  }
  else
  {
    log_error(logger, "No network is set.");
    timer.setPeriodSec(60);  // Use a default period of 1 minute.
  }

  // Make sure that we send data in the first loop
  timer.setNextTime(rtc_get_date_as_secs_since_2000());

  // Create the working directory for the campaign on the SD card.
  if(!sdcard_exists(CAMPAIGN_DIRECTORY_NAME))       { sdcard_mkdir(CAMPAIGN_DIRECTORY_NAME);       }
  if(!sdcard_exists(CAMPAIGN_RANGE_DIRECTORY_NAME)) { sdcard_mkdir(CAMPAIGN_RANGE_DIRECTORY_NAME); }

  // Do the work
  pcMyBuffer = (char *)this->myBuffer.getBufferPtr();
  while(1)
  {
    latitudeStr[0] = longitudeStr[0] = '?'; latitudeStr[1] = longitudeStr[1] = '\0';
    rssiStr[0]     = snrStr[0]       = '?'; rssiStr[1]     = snrStr[1]       = '\0';

    // Get a GPS reading
    status_ind_set_status(STATUS_IND_GPS_FIX_TRYING);
    log_info(logger, "Trying to get GPS data...");
    if((hasGPSData = this->GPS.refresh()))
    {
      // Synchronise RTC only if the network has not been opened yet; to avoid messing up the time server.
      if(!networkHasBeenOpened) { rtc_set_date(&this->GPS.time()); }

      // Write time to log
      log_info(logger, "GPS time: %04d-%02d-%02d %02d:%02d:%02d",
	       this->GPS.time().year,  this->GPS.time().month,   this->GPS.time().day,
	       this->GPS.time().hours, this->GPS.time().minutes, this->GPS.time().seconds);

      // Write position to log
      log_info(logger, "Position: (lat=%f; long=%f)",
	       this->GPS.location().latitude, this->GPS.location().longitude);

      // Convert position to strings
      snprintf(latitudeStr,  sizeof(latitudeStr),  "%f", this->GPS.location().latitude);
      snprintf(longitudeStr, sizeof(longitudeStr), "%f", this->GPS.location().longitude);
      latitudeStr[ sizeof(latitudeStr)  - 1] = '\0';
      longitudeStr[sizeof(longitudeStr) - 1] = '\0';

      status_ind_set_status(STATUS_IND_GPS_FIX_OK);
    }
    else
    {
      status_ind_set_status(STATUS_IND_GPS_FIX_KO);
      log_error(logger, "GPS fix failed");
    }
    board_watchdog_reset();
    Datetime dt;
    rtc_get_date(&dt);

    // Is it time to get a reading and to send?
    uint32_t tsMs = HAL_GetTick();
    if(timer.itsTime())
    {
      // Send data by network
      if(this->Network && hasGPSData)
      {
	// Send the data
	board_watchdog_reset();
	status_ind_set_status(STATUS_IND_AWAKE);
	log_info(logger, "Sending data...");

	if(!networkHasBeenOpened)
	{
	  // Initialise network
	  this->Network->open();
	  networkHasBeenOpened = true;
	}

	// Set the Tx data rate
	dataRate = sentCounter % 2 == 0 ? ClassLoRaWAN::DR_0 : ClassLoRaWAN::DR_5;
	((ClassLoRaWAN *)this->Network)->setDatarate(dataRate);
	dataRate = ((ClassLoRaWAN *)this->Network)->datarate();

	// Prepare the data to send
	this->myBuffer.clean();
	uint32_t size = snprintf(pcMyBuffer,  CONNECSENS_BUFFER_SIZE, "%04d-%02d-%02d %02d:%02d:%02d,%s,%s,DR%d",
				 dt.year,     dt.month,     dt.day, dt.hours, dt.minutes, dt.seconds,
				 latitudeStr, longitudeStr, dataRate);
	this->myBuffer.setWriteCursor(size);

	// Send
	tsMs = HAL_GetTick();
	if(this->Network->send(this->myBuffer.getBufferPtr(),
			       this->myBuffer.getNumberOfElements(),
			       ClassNetwork::SEND_OPTION_NO_ACK))
	{
	  log_info(logger, "Sent %d bytes: %s", this->myBuffer.getNumberOfElements(), this->myBuffer.getBufferPtr());
	  status_ind_set_status(STATUS_IND_RF_SEND_OK);
	}
	else
	{
	  bool internalError = HAL_GetTick() - tsMs < 2000;
	  log_error(logger, "Failed to send: %s", this->myBuffer.getBufferPtr());
	  this->Network->close();
	  networkHasBeenOpened = false;
	  status_ind_set_status(STATUS_IND_RF_SEND_KO);
	  if(internalError)
	  {
	    log_error(logger, "Internal error detected; reset node.");
	    status_ind_set_status(STATUS_IND_ASLEEP);
	    board_watchdog_reset();
	    board_reset(BOARD_SOFTWARE_RESET_ERROR);
	  }
	}
	status_ind_set_status(STATUS_IND_ASLEEP);
	board_watchdog_reset();

	// Get RSSI and SNR
	ClassNetwork::QoSValues qosValues;
	ClassNetwork::QoSFlags  qosFlags = ClassNetwork::QOS_RSSI_DBM | ClassNetwork::QOS_SNR_DB;
	if(this->Network->getQoSValues(&qosValues, qosFlags) && qosValues.flags == qosFlags)
	{
	  snprintf(rssiStr, sizeof(rssiStr), "%d", qosValues.rssi); rssiStr[sizeof(rssiStr) - 1] = '\0';
	  snprintf(snrStr,  sizeof(snrStr),  "%d", qosValues.snr);  snrStr[ sizeof(snrStr)  - 1] = '\0';
	}

	log_info(logger, "RSSI: %s dBm, SNR: %s dB", rssiStr, snrStr);
	sentCounter++;
      }

      // Write to data file
      // Build the campaign output data file name
      snprintf(outCSVFilename, sizeof(outCSVFilename), CAMPAIGN_RANGE_DIRECTORY_NAME "/%04d%02d%02d.csv",
	       dt.year, dt.month, dt.day);
      outCSVFilename[sizeof(outCSVFilename) - 1] = '\0';
      if(sdcard_fopen(&file, outCSVFilename, FILE_APPEND | FILE_WRITE))
      {
	if(!sdcard_fsize(&file))
	{
	  // The file is a new one; write CSV header.
	  sdcard_fwrite_string(&file, "timestamp;latitude;longitude;nodeName;RSSI;SNR;DevEUI;DR;RFPayload\r\n");
	}
	snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d;",
		 dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
	sdcard_fwrite_string(&file, buffer);
	sdcard_fwrite_string(&file, latitudeStr);      sdcard_fwrite_string(&file, ";");
	sdcard_fwrite_string(&file, longitudeStr);     sdcard_fwrite_string(&file, ";");
	sdcard_fwrite_string(&file, this->DeviceName); sdcard_fwrite_string(&file, ";");
	sdcard_fwrite_string(&file, rssiStr);          sdcard_fwrite_string(&file, ";");
	sdcard_fwrite_string(&file, snrStr);           sdcard_fwrite_string(&file, ";");
	sdcard_fwrite_string(&file, psDevEUIStr);      sdcard_fwrite_string(&file, ";");
	snprintf(buffer, sizeof(buffer), "DR%d;", dataRate); buffer[sizeof(buffer) - 1] = '\0';
	sdcard_fwrite_string(&file, buffer);
	sdcard_fwrite_string(&file, pcMyBuffer);
	sdcard_fwrite_string(&file, "\r\n");
	sdcard_fclose(&file);
	log_debug(logger, "New entry written to campaign range CSV data file.");
      }
      else
      {
	log_error(logger, "Failed to create range campaign data file: %s", outCSVFilename);
      }
    }
  }
}
