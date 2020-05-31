/*
 * Base sensor class for all Gill MaxiMets weather stations, using SDI-12 communication.
 *
 *  @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date:   2018
 */

#include "gillmaximetsdi12.h"
#ifdef USE_SENSOR_GILL_MAXIMET_SDI12
#include <string.h>
#include "sdi12gen_standardcommands.h"
#include "rtc.h"

#define STATE_VERSION  2


  CREATE_LOGGER(gillmaximetsdi12);
#undef  logger
#define logger  gillmaximetsdi12


GillMaximMetSDI12::MeasurementInfos
GillMaximMetSDI12::_MeasurementsInfos[GILL_MAXIMET_SDI12_NB_MEASUREMENTS] =
{
    { M_RELATIVE_WIND_DIRECTION,  "relativeWindDirection",    M_TYPE_INT,      CMD_M_IDFLAG_M,  0, false },
    { M_RELATIVE_WIND_SPEED,      "relativeWindSpeed",        M_TYPE_FLOAT,    CMD_M_IDFLAG_M,  1, false },
    { M_CORRECTED_WIND_DIRECTION, "correctedWindDirection",   M_TYPE_INT,      CMD_M_IDFLAG_M,  2, false },
    { M_CORRECTED_WIND_SPEED,     "correctedWindSpeed",       M_TYPE_FLOAT,    CMD_M_IDFLAG_M,  3, true  },
    { M_TEMPERATURE,              "temperature",              M_TYPE_FLOAT,    CMD_M_IDFLAG_M1, 0, false },
    { M_RELATIVE_HUMIDITY,        "relativeHumidity",         M_TYPE_INT,      CMD_M_IDFLAG_M1, 1, false },
    { M_PRESSURE,                 "pressure",                 M_TYPE_FLOAT,    CMD_M_IDFLAG_M1, 3, false },
    { M_TOTAL_PRECIPITATION,      "precipitation",            M_TYPE_FLOAT,    CMD_M_IDFLAG_M3, 1, false },
    { M_SOLAR_RADIATION,          "solarRadiation",           M_TYPE_INT,      CMD_M_IDFLAG_M4, 0, false },
    { M_GEO_POSITION,             "geographicalPosition",     M_TYPE_COMPOUND, CMD_M_IDFLAG_M5, 0, true  },
    { M_DATETIME,                 "datetime",                 M_TYPE_COMPOUND, CMD_M_IDFLAG_M6, 0, false }
};

GillMaximMetSDI12::Model GillMaximMetSDI12::_models[GILL_MAXIMET_SDI12_NB_MODELS] =
{
    { "GMX100", M_TOTAL_PRECIPITATION },
    { "GMX101", M_SOLAR_RADIATION     },
    {
	"GMX200",
	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED |
	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION
    },
    {
	"GMX240",
	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TOTAL_PRECIPITATION
    },
    { "GMX300", M_TEMPERATURE | M_RELATIVE_HUMIDITY | M_PRESSURE                         },
    { "GMX301", M_TEMPERATURE | M_RELATIVE_HUMIDITY | M_PRESSURE | M_SOLAR_RADIATION     },
    { "GMX400", M_TEMPERATURE | M_RELATIVE_HUMIDITY | M_PRESSURE | M_TOTAL_PRECIPITATION },
    {
	"GMX500",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE
    },
    {
	"GMX501",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_SOLAR_RADIATION
    },
    {
	"GMX531",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_SOLAR_RADIATION        |
	M_TOTAL_PRECIPITATION
    },
    {
	"GMX541",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_SOLAR_RADIATION        |
	M_TOTAL_PRECIPITATION
    },
    {
	"GMX550",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_TOTAL_PRECIPITATION
    },
    {
	"GMX551",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_SOLAR_RADIATION        |
	M_TOTAL_PRECIPITATION
    },
    {
	"GMX600",
    	M_RELATIVE_WIND_DIRECTION  | M_RELATIVE_WIND_SPEED    |
    	M_CORRECTED_WIND_DIRECTION | M_X_Y_TILT_Z_ORIENTATION |
	M_TEMPERATURE              | M_RELATIVE_HUMIDITY      |
	M_PRESSURE                 | M_TOTAL_PRECIPITATION
    },
};


const char *GillMaximMetSDI12::TYPE_NAME = "GillMaxiMetSDI12";


/**
 * Constructor.
 */
GillMaximMetSDI12::GillMaximMetSDI12() :
    SensorSDI12(POWER_NONE, POWER_NONE,
		NULL,
		FEATURE_CAN_DETECT_UNIQUE_ID | FEATURE_CAN_DETECT_FIRMWARE_VERSION)
{
  this->_pvModel            = NULL;
  this->_psType[0]          = '\0';
  this->_hasGPS             = false;
  this->_measurementsToGet  = M_NONE;
  this->_csvHeaderValues[0] = NULL;

  clearReadings();
}

GillMaximMetSDI12::~GillMaximMetSDI12()
{
  // Do nothing
}

Sensor *GillMaximMetSDI12::getNewInstance()
{
  return new GillMaximMetSDI12();
}

const char *GillMaximMetSDI12::type()
{
  return this->_psType;
}

/**
 * Reset current reading values.
 */
void GillMaximMetSDI12::clearReadings()
{
  this->_resetRainCountAtMidnight  = true;
  this->_measurementsWeGot         = M_NONE;
  this->_windDirectionDegRelative  = this->_windDirectionDegCorrected = 0;
  this->_windSpeedMPerSecRelative  = this->_windSpeedMPerSecCorrected = 0.0;
  this->_temperatureDegC           = 0.0;
  this->_relHumidityPercent        = 0;
  this->_pressureHPa               = 0.0;
  this->_precipitationMM           = 0.0;
  this->_precipitationPeriodSec    = 0;
  this->_solarRadiationWPerM2      = 0;
  this->_latitudeDegDeci           = this->_longitudeDegDeci = this->_altitudeM = 0.0;
  datetime_clear(&this->_datetime);
}

/**
 * Get the sensor's model from it's part number.
 *
 * @param[in]  psPartNumber the sensor's part number.
 * @param[out] pbHasGPS     is set to true if this sensor has GPS, set to false otherwise.
 *                          Can be set to NULL if we are not interested in this information.
 *
 * @return the model.
 * @return NULL if no model could be found for the given part number.
 */
const GillMaximMetSDI12::Model *GillMaximMetSDI12::getModelFromPartNumber(const char *psPartNumber,
									  bool       *pbHasGPS)
{
  const Model *pvModel;
  const char *pc, *psModelNumber;
  uint8_t     i;

  if(!psPartNumber || strlen(psPartNumber) != GILL_MAXIMET_SDI12_PART_NUMBER_LEN) { goto error_exit; }

  pc    = &psPartNumber[GILL_MAXIMET_SDI12_PART_NUMBER_MODEL_POS + 1]; // First digit always is '0'; skip it.
  for(i = 0; i < GILL_MAXIMET_SDI12_NB_MODELS; i++)
  {
    pvModel       = &_models[i];
    psModelNumber = pvModel->psName + 3;  // +3 to jump over the 'GMX' prefix.
    if(psModelNumber[0] == pc[0] && psModelNumber[1] == pc[1] && psModelNumber[2] == pc[2])
    {
      // Found matching model
      if(pbHasGPS) { *pbHasGPS = psPartNumber[GILL_MAXIMET_SDI12_PART_NUMBER_GPS_IND_POS] == '1'; }
      return pvModel;
    }
  }

  error_exit:
  return NULL;
}

/**
 * Get a measurement identifier using it's name.
 *
 * @note the comparison is not case sensitive.
 *
 * @param[in] psName the measurement's name.
 *
 * @return the measurement's identifier.
 * @return M_NONE if no identifier could be found for the given name.
 */
GillMaximMetSDI12::MeasurementId GillMaximMetSDI12::getMesurementIdFromName(const char *psName)
{
  uint8_t                 i;
  const MeasurementInfos *pvMInfos;

  if(psName && *psName)
  {
    for(i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
    {
      pvMInfos = &_MeasurementsInfos[i];
      if(strcasecmp(psName, pvMInfos->psName) == 0) { return pvMInfos->measurement; }
    }
  }

  return M_NONE;
}


bool GillMaximMetSDI12::jsonSpecificHandler(const JsonObject& json)
{
  uint8_t                 count, i;
  const char             *psValue;
  MeasurementId           mid;
  Measurements            measurements;
  const MeasurementInfos *pvMInfos;
  bool                    ok = SensorSDI12::jsonSpecificHandler(json);

  // Get model and if sensor has GPS
  this->_pvModel                  = NULL;
  this->_hasGPS                   = false;
  this->_psType[0]                = '\0';
  this->_resetRainCountAtMidnight = true;
  psValue          = json["partNumber"].as<const char *>();
  if(!psValue || !*psValue)
  {
    log_error_sensor(logger, "You must specify MaxiMet part number using configuration parameter 'partNumber'.");
    ok = false;
  }
  else if(!(this->_pvModel = getModelFromPartNumber(psValue, &this->_hasGPS)))
  {
    log_error_sensor(logger, "Unknown MaxiMet part number '%s'.", psValue);
    ok = false;
  }
  else
  {
    log_info_sensor(logger, "Configuration for model '%s', %s GPS.",
		    this->_pvModel->psName,
		    this->_hasGPS ? "with" : "without");
    strcpy(this->_psType, "Gill-");
    strcat(this->_psType, this->_pvModel->psName);
    if(this->_hasGPS) { strcat(this->_psType, "-GPS"); }
  }

  // Is the rain count reseted at midnight?
  if(json["stationResetsRainCountAtMidnight"].success())
  {
    this->_resetRainCountAtMidnight = json["stationResetsRainCountAtMidnight"].as<bool>();
  }

  // Get the measurements to get from the sensor.
  // First get all the measurement request from the configuration file
  this->_measurementsToGet = M_NONE;
  if(!json["measurements"].success())
  {
    // No specific measurements is indicated; get them all
    if(this->_pvModel)
    {
      this->_measurementsToGet = this->_pvModel->measurements;
      if(this->_hasGPS && (this->_pvModel->measurements & M_RELATIVE_WIND_SPEED))
      {
	this->_measurementsToGet |= M_CORRECTED_WIND_SPEED;
      }
    }
  }
  else if(json["measurements"].is<const char *>())
  {
    if((psValue = json["measurements"].as<const char *>()))
    {
      if(strcasecmp(psValue, "all") == 0)
      {
	if(this->_pvModel)
	{
	  this->_measurementsToGet = this->_pvModel->measurements;
	  if(this->_hasGPS && (this->_pvModel->measurements & M_RELATIVE_WIND_SPEED))
	  {
	    this->_measurementsToGet |= M_CORRECTED_WIND_SPEED;
	  }
	}
      }
      else
      {
	log_error_sensor(logger, "Invalid string value '%s' for configuration parameter 'measurements'.", psValue);
	ok = false;
      }
    }
  }
  else
  {
    const JsonArray &jsonArray = json["measurements"];
    if(jsonArray.success())
    {
      for(i = 0, count = jsonArray.size(); i < count; i++)
      {
	if((mid = getMesurementIdFromName((psValue = jsonArray[i].as<const char *>()))) == M_NONE)
	{
	  log_error_sensor(logger, "Unknown measurement '%s'.", psValue);
	  ok = false;
	}
	else { this->_measurementsToGet |= mid; }
      }
    }
  }
  // Then check this sensor model can provide all the requested measurements
  if(this->_pvModel && (measurements = this->_measurementsToGet & ~this->_pvModel->measurements))
  {
    // We have been asked to get measurements that we cannot get from this sensor model.
    for(i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
    {
      pvMInfos = &_MeasurementsInfos[i];
      if(measurements & pvMInfos->measurement)
      {
	log_error_sensor(logger, "Measurement '%s' is not available on sensor model %s.", pvMInfos->psName, this->_pvModel->psName);
	this->_measurementsToGet &= ~pvMInfos->measurement;
      }
    }
    ok = false;
  }
  // If the sensor does not have a GPS then some measurements are not available
  if(!this->_hasGPS)
  {
    for(i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
    {
      pvMInfos = &_MeasurementsInfos[i];
      if((this->_measurementsToGet & _MeasurementsInfos[i].measurement) && pvMInfos->requireGPS)
      {
	log_error_sensor(logger, "Cannot get measurement '%s' from sensor; it requires GPS and the sensor does not have it.", pvMInfos->psName);
	this->_measurementsToGet &= ~pvMInfos->measurement;
      }
    }
  }
  // Make sure that we are asked to get at least one measurement
  if(this->_measurementsToGet == M_NONE)
  {
    log_error_sensor(logger, "You must set up at least one measurement to get from the sensor using configuration parameter 'measurements'.");
    ok = false;
  }

  return ok;
}


bool GillMaximMetSDI12::setUniqueIdAndFirmwareVersionFromGetIdCmd(const SDI12Command *pvCmd)
{
  char     *buffer;
  uint8_t *pu8Data, size;
  bool     ok = true;

  // Get the command's data
  if(!(pu8Data = sdi12_command_response_data(pvCmd, &size)) || size < 20)
  {
    log_error_sensor(logger, "Send Identification response is too short.");
    ok = false;
    goto exit;
  }

  // Set the firmware version
  buffer    = (char *)Sensor::firmwareVersion();
  buffer[0] = pu8Data[12]; buffer[1] = pu8Data[13];
  buffer[2] = pu8Data[14]; buffer[3] = pu8Data[15];
  buffer[4] = '-';
  buffer[5] = pu8Data[16];
  buffer[6] = '.';
  buffer[7] = pu8Data[17]; buffer[8] = pu8Data[18];
  buffer[9] = '\0';

  // Set the unique id if not already set
  if(*(buffer = (char *)Sensor::uniqueId()) || !this->_psType[0]) { goto exit; }

  strlcpy(buffer, type(), SENSOR_UNIQUE_ID_SIZE_MAX);
  strlcat(buffer, "-",    SENSOR_UNIQUE_ID_SIZE_MAX);
  strlcat(buffer, (const char *)(pu8Data + 19), SENSOR_UNIQUE_ID_SIZE_MAX);
  buffer[strlen(buffer) - SDI12_COMMAND_END_OF_RESPONSE_LEN] = '\0';

  exit:
  return ok;
}


Sensor::State *GillMaximMetSDI12::defaultState(uint32_t *pu32_size)
{
  this->_state.base.version        = STATE_VERSION;
  this->_state.tsLastRead          = 0;
  this->_state.lastPrecipitationMM = 0;
  this->_state.sensorDate          = 0;

  *pu32_size = sizeof(this->_state);
  return &this->_state.base;
}

#include "cnssrf.h"
bool GillMaximMetSDI12::readSpecific()
{
  uint8_t                 i;
  uint32_t                secsToMidnight, sensorDate;
  float                   precipitationMM;
  CommandMId              cmdId;
  CommandMFlag            cmdFlag;
  StateSpecific          *pvState;
  const MeasurementInfos *pvMInfos;
  CommandMFlags           cmdFlags;
  const SDI12Command     *pvCmd;
  ts2000_t                tsRead2000;
  Measurements            measurements = this->_measurementsToGet;
  bool                    ok           = true;

  clearReadings();

  // Get the sensor's state
  if(!(pvState = (StateSpecific *)state())) { ok = false; goto exit; }

  // If we are asked to get precipitation then we may need some extra measurements
  if(measurements & M_TOTAL_PRECIPITATION)
  {
    if(this->_resetRainCountAtMidnight) { measurements |= M_DATETIME; }
  }
  else { pvState->tsLastRead = 0; } // Make sure to clear the timestamp in the sensor's state.

  // Call SDI-12 commands and get the values
  //ConnecSenS::addVerboseSyslogEntry(name(), "Measurements to get: %08X.", measurements);
  for(cmdFlags = 0, i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
  {
    pvMInfos   = &_MeasurementsInfos[i];
    cmdFlag    = gill_maximet_sdi12_cmdm_idflag_flag(pvMInfos->cmdIdFlag);
    if((measurements & pvMInfos->measurement) && !(cmdFlag & cmdFlags))
    {
      // Send the command
      cmdId    = gill_maximet_sdi12_cmdm_idflag_id(pvMInfos->cmdIdFlag);
      //ConnecSenS::addVerboseSyslogEntry(name(), "SDI-12 command: aMC%u", cmdId);
      if(!sendCommandStartMeasurement(cmdId, true, &pvCmd))
      {
	log_error_sensor(logger, "Failed to execute SDI-12 Start Measurement with id '%d'.", cmdId);
	ok = false;
	goto exit;
      }

      // Get measurements from command response.
      //ConnecSenS::addVerboseSyslogEntry(name(), "SDI-12 %s: '%s'.", (char *)pvCmd->pu8_cmd, (char *)pvCmd->pu8_buffer);
      if(!getMeasurementsFromCmdResponse(cmdId, pvCmd, measurements))
      {
	log_error_sensor(logger, "While reading measurement from SDI-12 command with id '%d'.", cmdId);
	ok = false;
	goto exit;
      }

      // Indicate that the SDI-12 command already has been sent.
      cmdFlags |= cmdFlag;
    }
  }
  this->_measurementsWeGot = this->_measurementsToGet;

  // If precipitation are requested then some extra computations are needed
  tsRead2000 = rtc_get_date_as_secs_since_2000();
  if(measurements & M_TOTAL_PRECIPITATION)
  {
    log_info_sensor(logger,
		    "Time: %04d-%02d-%02dT%02d:%02d:%02d, Precipitation: %0.2fmm.",
		    this->_datetime.year,  this->_datetime.month,   this->_datetime.day,
		    this->_datetime.hours, this->_datetime.minutes, this->_datetime.seconds,
		    this->_precipitationMM);
    precipitationMM = this->_precipitationMM;
    sensorDate      = this->_datetime.year * 10000 + this->_datetime.month * 100 + this->_datetime.day;

    if(pvState->tsLastRead)
    {
      if(this->_resetRainCountAtMidnight && sensorDate != pvState->sensorDate)
      {
	// The day has changed since last reading.
	// So the sensor has reseted its precipitation counter at midnight.
	// If there were precipitation counted before midnight that we did not get, then they are lost to us.
	// Keep the current precipitation count obtained from the sensor.
	this->_precipitationPeriodSec =
	    this->_datetime.hours   * 3600 +
	    this->_datetime.minutes * 60   +
	    this->_datetime.seconds;
      }
      else if(tsRead2000 > pvState->tsLastRead &&
	  this->_precipitationMM >= pvState->lastPrecipitationMM)
      {
	this->_precipitationPeriodSec = tsRead2000 - pvState->tsLastRead;
	this->_precipitationMM        =
	    (this->_precipitationMM * 100 - pvState->lastPrecipitationMM * 100) / 100.0;  // *100 and /100 to avoid rounding or precision errors.
      }
      else
      {
	// Problem, do not use precipitation value.
	this->_measurementsWeGot     &= ~M_TOTAL_PRECIPITATION;
	log_info_sensor(logger, "Rain amount is ignored because node's current time or current rain amount are lower than the previous ones.");
      }
    }
    else { this->_measurementsWeGot  &= ~M_TOTAL_PRECIPITATION; }  // No previous measurement; ignore rain reading.

    // Set up state for next reading.
    pvState->tsLastRead          = tsRead2000;
    pvState->lastPrecipitationMM = precipitationMM;
    pvState->sensorDate          = sensorDate;

    if(this->_resetRainCountAtMidnight)
    {
      // See if we have to change the sensor's period to get the next reading before midnight is passed.
      secsToMidnight = DT_SECONDS_IN_1DAY -
	  this->_datetime.hours   * DT_SECONDS_IN_1HOUR   +
	  this->_datetime.minutes * DT_SECONDS_IN_1MINUTE +
	  this->_datetime.seconds;
      // Use 10 seconds margin
      if(secsToMidnight > 10 && periodSec() > secsToMidnight)
      {
	// Update the time left on periodic reading to perform a reading just before sensor's midnight.
	setNextTime(tsRead2000 + secsToMidnight - 10);
      }
    }
  }

  // Save the sensor's state
  saveState();

  exit:
  return ok;
}

/**
 * Get measurements from a SDI-12 command's response.
 *
 * @param[in] cmdId             the identifier of the SDI-12 command that has been sent.
 * @param[in] pvCmd             the SDI-12 command. MUST contains a response and MUST be NOT NULL.
 * @param[in] measurementsToGet indicate the measurements to get.
 *                              Can contain flags to measurements that are not in the response.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool GillMaximMetSDI12::getMeasurementsFromCmdResponse(CommandMId          cmdId,
						       const SDI12Command *pvCmd,
						       Measurements        measurementsToGet)
{
  uint8_t                 i;
  float                   valueFloat;
  int32_t                 valueInt32;
  const MeasurementInfos *pvMInfos;
  SDI12CmdValuesIterator  it;
  SDI12CmdValue           values[10], *pvValue;

  // Get iterator on response's values.
  if(!sdi12_cmd_get_iterator_on_values(pvCmd, &it) || it.nb_values > 10) { goto error_exit; }

  // Read all the values
  for(pvValue = values; sdi12_cmd_iterator_get_next_value(&it, pvValue); pvValue++) ; // Do nothing

  // Look for the measurements that can be extracted from the response
  for(i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
  {
    pvMInfos = &_MeasurementsInfos[i];
    if(gill_maximet_sdi12_cmdm_idflag_id(pvMInfos->cmdIdFlag) != cmdId) continue;

    pvValue    = &values[pvMInfos->valuePos];
    valueFloat = pvValue->value.float_value;
    valueInt32 = pvValue->value.integer_value;
    switch(pvMInfos->measurement)
    {
      case M_RELATIVE_WIND_DIRECTION:  this->_windDirectionDegRelative  = valueInt32; break;
      case M_CORRECTED_WIND_DIRECTION: this->_windDirectionDegCorrected = valueInt32; break;
      case M_RELATIVE_WIND_SPEED:      this->_windSpeedMPerSecRelative  = valueFloat; break;
      case M_CORRECTED_WIND_SPEED:     this->_windSpeedMPerSecCorrected = valueFloat; break;
      case M_TEMPERATURE:              this->_temperatureDegC           = valueFloat; break;
      case M_RELATIVE_HUMIDITY:        this->_relHumidityPercent        = valueInt32; break;
      case M_PRESSURE:                 this->_pressureHPa               = valueFloat; break;
      case M_TOTAL_PRECIPITATION:      this->_precipitationMM           = valueFloat; break;
      case M_SOLAR_RADIATION:          this->_solarRadiationWPerM2      = valueInt32; break;

      case M_DATETIME:
	datetime_init(&this->_datetime,
	    values[0].value.integer_value, values[1].value.integer_value ,values[2].value.integer_value,
	    values[3].value.integer_value, values[4].value.integer_value, values[5].value.integer_value,
	    0);
	break;

// The next case are commented out because we should not be able to request them.
      case M_GEO_POSITION:
	// TODO: implement me.
	break;

/*      case M_DEWPOINT:
      case M_WIND_CHILL:
      case M_HEAT_INDEX:
      case M_AIR_DENSITY:
      case M_WET_BULB_TEMPERATURE:
      case M_PRECIPITATION_INTENSITY:
      case M_X_Y_TILT_Z_ORIENTATION:
      case M_SUNSHINE_HOURS:
      case M_SUNRISE_TIME:
      case M_SOLAR_NOON:
      case M_SUNSET_TIME:
      case M_SUN_POSITION:
      case M_TWILIGHT_CIVIL:
      case M_TWILIGHT_NAUTICAL:
      case M_TWILIGHT_ASTRONOMICAL:
	// Derived measurements that we have decided not to get for now.
	break;
*/
      default:
	log_error_sensor(logger, "Unknown measurement identifier: '%d'.", pvMInfos->measurement);
	goto error_exit;
    }
  }

  return true;

  error_exit:
  return false;
}



bool GillMaximMetSDI12::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  bool res = true;

  if(this->_measurementsWeGot & M_TEMPERATURE)
  {
    res &= cnssrf_dt_temperature_write_degc_to_frame(pvFrame,
						     this->_temperatureDegC,
						     CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE);
  }

  if(this->_measurementsWeGot & M_RELATIVE_HUMIDITY)
  {
    res &= cnssrf_dt_humidity_write_air_relpercents_to_frame(pvFrame, this->_relHumidityPercent);
  }

  if(this->_measurementsWeGot & M_PRESSURE)
  {
    res &= cnssrf_dt_pressure_write_atmo_hpa_to_frame(pvFrame, this->_pressureHPa);
  }

  if((this->_measurementsWeGot & (M_RELATIVE_WIND_DIRECTION | M_RELATIVE_WIND_SPEED)) ==
      (M_RELATIVE_WIND_DIRECTION | M_RELATIVE_WIND_SPEED))
  {
    // We have both wind speed and wind direction
    res &= cnssrf_dt_wind_write_speed_direction(pvFrame,
    						  this->_windSpeedMPerSecRelative, false,
    						  this->_windDirectionDegRelative, false);
  }
  else if (this->_measurementsWeGot & M_RELATIVE_WIND_DIRECTION)
  {
    // We only have wind direction
    res &= cnssrf_dt_wind_write_direction_deg(  pvFrame, this->_windDirectionDegRelative, false);
  }
  else if (this->_measurementsWeGot & M_RELATIVE_WIND_SPEED)
  {
    // We only have wind speed
    res &= cnssrf_dt_wind_write_speed_ms(       pvFrame, this->_windSpeedMPerSecRelative, false);
  }
  // Else, do nothing.

  if((this->_measurementsWeGot & (M_CORRECTED_WIND_DIRECTION | M_CORRECTED_WIND_SPEED)) ==
      (M_CORRECTED_WIND_DIRECTION | M_CORRECTED_WIND_SPEED))
  {
    // We have both wind speed and wind direction
    res &= cnssrf_dt_wind_write_speed_direction(pvFrame,
    						  this->_windSpeedMPerSecCorrected, true,
    						  this->_windDirectionDegCorrected, true);
  }
  else if (this->_measurementsWeGot & M_CORRECTED_WIND_DIRECTION)
  {
    // We only have wind direction
    res &= cnssrf_dt_wind_write_direction_deg(  pvFrame, this->_windDirectionDegCorrected, true);
  }
  else if (this->_measurementsWeGot & M_CORRECTED_WIND_SPEED)
  {
    // We only have wind speed
    res &= cnssrf_dt_wind_write_speed_ms(       pvFrame, this->_windSpeedMPerSecCorrected, true);
  }
  // Else, do nothing.

  if(this->_measurementsWeGot & M_TOTAL_PRECIPITATION)
  {
    res &= cnssrf_dt_rainamount_write_mm_to_frame(pvFrame,
						  this->_precipitationPeriodSec,
						  this->_precipitationMM,
						  false);
  }

  if(this->_measurementsWeGot & M_SOLAR_RADIATION)
  {
    res &= cnssrf_dt_solar_write_irradiance_wm2_to_frame(pvFrame, this->_solarRadiationWPerM2);
  }

  // TODO: Add solar radiation
  return res;
}



const char **GillMaximMetSDI12::csvHeaderValues()
{
  const char **pps = this->_csvHeaderValues;

  if(this->_measurementsToGet & M_TEMPERATURE)
  {
    *pps++ = "airTemperatureDegC";
  }
  if(this->_measurementsToGet & M_RELATIVE_HUMIDITY)
  {
    *pps++ = "airHumidityRelPercent";
  }
  if(this->_measurementsToGet & M_PRESSURE)
  {
    *pps++ = "airPressureHPa";
  }
  if(this->_measurementsToGet & M_RELATIVE_WIND_DIRECTION)
  {
    *pps++ = "windDirectionRelativeDegN";
  }
  if(this->_measurementsToGet & M_RELATIVE_WIND_SPEED)
  {
    *pps++ = "windSpeedRelativeMPS";
  }
  if(this->_measurementsToGet & M_CORRECTED_WIND_DIRECTION)
  {
    *pps++ = "windDirectionCorrectedDegN";
  }
  if(this->_measurementsToGet & M_CORRECTED_WIND_SPEED)
  {
    *pps++ = "windSpeedCorrectedMPS";
  }
  if(this->_measurementsToGet & M_TOTAL_PRECIPITATION)
  {
    *pps++ = "rainMM";
    *pps++ = "rainPeriodSec";
  }
  if(this->_measurementsToGet & M_SOLAR_RADIATION)
  {
    *pps++ = "solarRadiationWPM2";
  }
  *pps = NULL;

  return this->_csvHeaderValues;
}

int32_t GillMaximMetSDI12::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len, i;
  char     values[   GILL_MAXIMET_SDI12_NB_MEASUREMENTS][10];
  char    *ppsValues[GILL_MAXIMET_SDI12_NB_MEASUREMENTS];

  // Set all values to empty string
  for(i = 0; i < GILL_MAXIMET_SDI12_NB_MEASUREMENTS; i++)
  {
    values[i][0] = '\0';
    ppsValues[i] = values[i];
  }

  // Get values as strings
  len = 0;
  if(  this->_measurementsToGet & M_TEMPERATURE)
  {
    if(this->_measurementsWeGot & M_TEMPERATURE)
    {
      sprintf(values[len], "%.1f", this->_temperatureDegC);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_RELATIVE_HUMIDITY)
  {
    if(this->_measurementsWeGot & M_RELATIVE_HUMIDITY)
    {
      sprintf(values[len], "%u", this->_relHumidityPercent);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_PRESSURE)
  {
    if(this->_measurementsWeGot & M_PRESSURE)
    {
      sprintf(values[len], "%.3f", this->_pressureHPa);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_RELATIVE_WIND_DIRECTION)
  {
    if(this->_measurementsWeGot & M_RELATIVE_WIND_DIRECTION)
    {
      sprintf(values[len], "%u", this->_windDirectionDegRelative);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_RELATIVE_WIND_SPEED)
  {
    if(this->_measurementsWeGot & M_RELATIVE_WIND_SPEED)
    {
      sprintf(values[len], "%.3f", this->_windSpeedMPerSecRelative);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_CORRECTED_WIND_DIRECTION)
  {
    if(this->_measurementsWeGot & M_CORRECTED_WIND_DIRECTION)
    {
      sprintf(values[len], "%u", this->_windDirectionDegCorrected);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_CORRECTED_WIND_SPEED)
  {
    if(this->_measurementsWeGot & M_CORRECTED_WIND_SPEED)
    {
      sprintf(values[len], "%.3f", this->_windSpeedMPerSecCorrected);
    }
    len++;
  }
  if(  this->_measurementsToGet & M_TOTAL_PRECIPITATION)
  {
    if(this->_measurementsWeGot & M_TOTAL_PRECIPITATION)
    {
      sprintf(values[len++], "%.2f", this->_precipitationMM);
      sprintf(values[len++], "%u",   (unsigned int)this->_precipitationPeriodSec);
    }
    else { len += 2; }
  }
  if(  this->_measurementsToGet & M_SOLAR_RADIATION)
  {
    if(this->_measurementsWeGot & M_SOLAR_RADIATION)
    {
      sprintf(values[len], "%u", (unsigned int)this->_windSpeedMPerSecRelative);
    }
    len++;
  }

  // Write CSV string
  return csvMakeStringUsingStringValues(ps_data, size, (const char **)ppsValues, len);
}


#endif  // USE_SENSOR_GILL_MAXIMET_SDI12


