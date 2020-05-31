/*
 * Driver for Truebner soil humidity sensor SMT100 using SDI-12 interface.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date 2018
 */
#include "truebnersmt100sdi12.hpp"
#ifdef USE_SENSOR_TRUEBNER_SMT100_SDI12
#include "sdi12gen_standardcommands.h"

#define TRUEBNER_RAW_DATA_TYPE_ID   0x1F


  CREATE_LOGGER(truebnersmt100sdi12);
#undef  logger
#define logger  truebnersmt100sdi12


#define TRUEBNER_SENSOR_TYPE_NAME  "TruebnerSMT100SDI12"
const char *TruebnerSMT100SDI12::TYPE_NAME = TRUEBNER_SENSOR_TYPE_NAME;

const TruebnerSMT100SDI12::MeasurementInfos
TruebnerSMT100SDI12::_measurementInfos[TRUEBNER_SMT100_SDI12_MEASUREMENTS_COUNT] =
{
    { MEASUREMENT_ID_RAW,          "raw"          },
    { MEASUREMENT_ID_PERMITTIVITY, "permittivity" },
    { MEASUREMENT_ID_WATER,        "water"        },
    { MEASUREMENT_ID_TEMPERATURE,  "temperature"  },
    { MEASUREMENT_ID_VOLTAGE,      "voltage"      }
};


/**
 * The SMT100 does not follow very well the SDI-12 standard.
 * So we have to re-define some standard commands.
 * In particular, with the SMT100, we have to send a break before sending the
 * Send Data command, while in the specification it is said that sending a break
 * will cancel the current measurement.
 */
#define TRUEBNER_SDI12_CMD_START_MEASUREMENT  SDI12_CMD_STD_START_MEASUREMENT
#define TRUEBNER_SDI12_CMD_SEND_DATA          SDI12_CMD_STD_SEND_DATA
const SDI12GenSensorCommands TruebnerSMT100SDI12::_commandsDescription =
{
    TRUEBNER_SENSOR_TYPE_NAME,
    {
	// Overwrite standard Start Measurement command.
	{
	  TRUEBNER_SDI12_CMD_START_MEASUREMENT,
	  "Start Measurement",
	  "aM!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  TRUEBNER_SDI12_CMD_SEND_DATA,
	  TRUEBNER_SENSOR_TYPE_NAME
	},
	// Overwrite standard Send Data command.
	{
	  TRUEBNER_SDI12_CMD_SEND_DATA,
	  "Send Data",
	  "aD${n:inc,t:i,l:1,r:[0..9]}!",
	  "a<values><CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK,   // With SMT100 we have to send a break before the Send Data command
	  NULL,
	  NULL,
	  NULL
	},
	// End of command list indication
	{ NULL, NULL, NULL, NULL, SDI12_CMD_CONFIG_NONE, NULL, NULL, NULL }
    }
};

const char *TruebnerSMT100SDI12::_CSV_HEADER_VALUES[] =
{
    "soilVolumetricWaterContentPercent", "soilTemperatureDegC",
    "soilPermittivity",                  "rawValue",
    "voltageV",
    NULL
};


TruebnerSMT100SDI12::TruebnerSMT100SDI12() :
    SensorSDI12(POWER_EXTERNAL, POWER_NONE, &_commandsDescription, FEATURE_BASE)
{
  this->_measurementsToGet = MEASUREMENT_ID_ALL;
  this->_depthCm           = 0;

  clearReadings();
}

TruebnerSMT100SDI12::~TruebnerSMT100SDI12()
{
  // Do nothing
}

Sensor *TruebnerSMT100SDI12::getNewInstance()
{
  return new TruebnerSMT100SDI12();
}

const char *TruebnerSMT100SDI12::type()
{
  return TYPE_NAME;
}


/**
 * Clear readings.
 */
void TruebnerSMT100SDI12::clearReadings()
{
  this->_raw                    = 0;
  this->_dielectricPermittivity = 0;
  this->_waterPercent           = 0.0;
  this->_temperatureDegC        = 0.0;
}

/**
 * Get a measurement's identifier using it's name.
 *
 * @note the name comparison is not case sensitive.
 *
 * @param[in] psName the name.
 *
 * @return the identifier.
 * @return MEASUREMENT_ID_NONE if no identifier could be found for the given name.
 */
TruebnerSMT100SDI12::MeasurementId TruebnerSMT100SDI12::getMeasurementIdUsingName(const char *psName)
{
  uint8_t                 i;
  const MeasurementInfos *pvInfos;
  MeasurementId           res = MEASUREMENT_ID_NONE;

  if(psName && *psName)
  {
    for(i = 0; i < TRUEBNER_SMT100_SDI12_MEASUREMENTS_COUNT; i++)
    {
      pvInfos = &_measurementInfos[i];
      if(strcasecmp(pvInfos->psName, psName) == 0)
      {
	res = pvInfos->id;
	break;
      }
    }
  }

  return res;
}

bool TruebnerSMT100SDI12::jsonSpecificHandler(const JsonObject& json)
{
  uint8_t       i, count;
  const char   *psValue;
  MeasurementId mid;
  bool          res;

  res = SensorSDI12::jsonSpecificHandler(json);

  if(!(this->_depthCm = json["depthCm"].as<uint32_t>()))
  {
    log_error_sensor(logger, "'depthCm' configuration parameter is missing or has invalid value.");
    res = false;
  }

  // Get the list of measurements to get.
  this->_measurementsToGet = MEASUREMENT_ID_ALL;
  if(json["measurements"].success())
  {
    if(json["measurements"].is<const char *>())
    {
      if((psValue = json["measurements"].as<const char *>()) && strcasecmp(psValue, "all") != 0)
      {
	log_error_sensor(logger, "Invalid 'measurements' string configuration parameter value.");
	res = false;
      }
      // Else, do nothing, we already are set up to get all measurements.
    }
    else
    {
      const JsonArray &jsonArray = json["measurements"];
      if(jsonArray.success())
      {
	this->_measurementsToGet = MEASUREMENT_ID_NONE;
	for(i = 0, count = jsonArray.size(); i < count; i++)
	{
	  if((mid = getMeasurementIdUsingName((psValue = jsonArray[i].as<const char *>()))) == MEASUREMENT_ID_NONE)
	  {
	    log_error_sensor(logger, "Unknown measurement '%s'.", psValue);
	    res = false;
	  }
	  this->_measurementsToGet |= mid;
	}
      }
      else
      {
	log_error_sensor(logger, "Invalid type for configuration parameter 'measurements'.");
	res = false;
      }
    }
  }

  return res;
}

bool TruebnerSMT100SDI12::readSpecific()
{
  const SDI12Command    *pvCmd;
  SDI12CmdValuesIterator it;
  SDI12CmdValue          value;

  // Get the readings
  if(!sendCommandUsingName(TRUEBNER_SDI12_CMD_START_MEASUREMENT, NULL, &pvCmd)) { goto error_exit; }
  log_info_sensor(logger, "Readings: %s.", (const char *)pvCmd->pu8_buffer);

  // Go through the values
  if(!sdi12_cmd_get_iterator_on_values(pvCmd, &it)) { goto error_exit; }
  if(it.nb_values != 5)
  {
    log_error_sensor(logger, "We got %d values, we were expecting 5.", it.nb_values);
    goto error_exit;
  }

  // Raw
  if(!sdi12_cmd_iterator_get_next_value(&it, &value)) { goto error_exit; }
  if(this->_measurementsToGet & MEASUREMENT_ID_RAW)
  {
    this->_raw = (uint32_t)value.value.integer_value;
  }

  // Permittivity
  if(!sdi12_cmd_iterator_get_next_value(&it, &value)) { goto error_exit; }
  if(this->_measurementsToGet & MEASUREMENT_ID_PERMITTIVITY)
  {
    this->_dielectricPermittivity = value.value.float_value;
  }

  // Volumic water
  if(!sdi12_cmd_iterator_get_next_value(&it, &value)) { goto error_exit; }
  if(this->_measurementsToGet & MEASUREMENT_ID_WATER)
  {
    this->_waterPercent =  value.value.float_value;
  }

  // Temperature
  if(!sdi12_cmd_iterator_get_next_value(&it, &value)) { goto error_exit; }
  if(this->_measurementsToGet & MEASUREMENT_ID_TEMPERATURE)
  {
    this->_temperatureDegC = value.value.float_value;
  }

  // Voltage
  if(!sdi12_cmd_iterator_get_next_value(&it, &value)) { goto error_exit; }
  if(this->_measurementsToGet & MEASUREMENT_ID_VOLTAGE)
  {
    this->_voltageV = value.value.float_value;
  }

  return true;

  error_exit:
  return false;
}

bool TruebnerSMT100SDI12::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  CNSSRFValue value;
  bool        res;

  res = cnssrf_dt_depth_write_cm_to_frame(pvFrame, this->_depthCm);

  if(this->_measurementsToGet & MEASUREMENT_ID_WATER)
  {
    res &= cnssrf_dt_soilmoisture_write_volumetric_content_percent_to_frame(pvFrame, this->_waterPercent);
  }

  if(this->_measurementsToGet & MEASUREMENT_ID_TEMPERATURE)
  {
    res &= cnssrf_dt_temperature_write_degc_to_frame(pvFrame,
						     this->_temperatureDegC,
						     CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE);
  }

  if(this->_measurementsToGet & MEASUREMENT_ID_VOLTAGE)
  {
    res &= cnssrf_dt_battvoltage_write_volts_to_frame(pvFrame, this->_voltageV);
  }


  if(this->_measurementsToGet & MEASUREMENT_ID_PERMITTIVITY)
  {
    res &= cnssrf_dt_permittivity_write_relative_dielectric_to_frame(pvFrame, this->_dielectricPermittivity);
  }

  if(this->_measurementsToGet & MEASUREMENT_ID_RAW)
  {
    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = this->_raw;

    res &= cnssrf_data_type_write_values_to_frame(pvFrame,
						  TRUEBNER_RAW_DATA_TYPE_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }

  return res;
}


const char **TruebnerSMT100SDI12::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t TruebnerSMT100SDI12::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len, i;
  char     values[   5][10];
  char    *ppsValues[5];

  // Set all values to empty string
  for(i = 0; i < 5; i++)
  {
    values[i][0] = '\0';
    ppsValues[i] = values[i];
  }

  // Get values as strings
  len = 0;
  if(this->_measurementsToGet & MEASUREMENT_ID_WATER)
  {
    sprintf(values[len], "%.1f", this->_waterPercent);
  }

  len++;
  if(this->_measurementsToGet & MEASUREMENT_ID_TEMPERATURE)
  {
    sprintf(values[len], "%.2f", this->_temperatureDegC);
  }
  len++;
  if(this->_measurementsToGet & MEASUREMENT_ID_PERMITTIVITY)
  {
    sprintf(values[len], "%.2f", this->_dielectricPermittivity);
  }
  len++;
  if(this->_measurementsToGet & MEASUREMENT_ID_RAW)
  {
    sprintf(values[len], "%u", (unsigned int)this->_raw);
  }
  len++;
  if(this->_measurementsToGet & MEASUREMENT_ID_VOLTAGE)
  {
    sprintf(values[len], "%.2f", this->_voltageV);
  }
  len++;

  // Write CSV string
  return csvMakeStringUsingStringValues(ps_data, size, (const char **)ppsValues, len);
}


#endif  // USE_SENSOR_TRUEBNER_SMT100_SDI12

