/*
 * Driver for InSitu Aqua TROLL-200 sensor.
 *
 *  Created on: Jul 18, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include <string.h>
#include "insituaquatroll200sdi12.hpp"
#ifdef USE_SENSOR_INSITU_AQUATROLL_200_SDI12
#include "sdi12gen_standardcommands.h"
#include "units.h"
#include "cnssrf-dt_solutionconductivity.h"
#include "cnssrf-dt_pressure.h"
#include "cnssrf-dt_temperature.h"
#include "cnssrf-dt_level.h"


  CREATE_LOGGER(insituaquatroll200sdi12);
#undef  logger
#define logger  insituaquatroll200sdi12


#define AQUATROLL_SENSOR_TYPE  "InSituAquaTROLL200SDI12"
const char *InSituAquaTROLL200SDI12::TYPE_NAME = AQUATROLL_SENSOR_TYPE;


#define SDI12_CMD_ISCO_COMPATIBILITY      "ISCOCompatibility"
#define SDI12_CMD_SET_OUTPUT_SEQUENCE     "setOuputSequence"
#define SDI12_CMD_SET_PRESSURE_UNITS      "setPressureUnits"
#define SDI12_CMD_SET_TEMPERATURE_UNITS   "setTemperatureUnits"
#define SDI12_CMD_SET_LEVEL_UNITS         "setLevelUnits"
#define SDI12_CMD_SET_LEVEL_MODE          "setLevelMode"
#define SDI12_CMD_SET_CONDUCTIVITY_UNITS  "setConductivityUnits"
#define SDI12_CMD_SET_TDS_UNITS           "setTDSUnits"

const SDI12GenSensorCommands InSituAquaTROLL200SDI12::_commandsDescription =
{
    AQUATROLL_SENSOR_TYPE,
    {
	{
	    SDI12_CMD_ISCO_COMPATIBILITY,
	    "ISCO Compatibility",
	    "aXPR0!",
	    "a${n:pairs,t:s,l:[2..12]}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK,
	    NULL, NULL, NULL
	},
	{
	    SDI12_CMD_SET_OUTPUT_SEQUENCE,
	    "Set Output Sequence",
	    "aXO${n:nnn,t:s,l:[1..9]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_PRESSURE_UNITS,
	    "Set Pressure Units",
	    "aXPU${n:units_id,t:i,r:[17|19..22]]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_TEMPERATURE_UNITS,
	    "Set Temperature Units",
	    "aXTU${n:units_id,t:i,r:[1|2]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_LEVEL_UNITS,
	    "Set Level Units",
	    "aXLU${n:units_id,t:i,r:[33..35|37|38]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_LEVEL_MODE,
	    "Set Level Mode",
	    "aXLM${n:mode_id,t:i,r:[3..5]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_CONDUCTIVITY_UNITS,
	    "Set Conductivity Units",
	    "aXCU${n:units_id,t:i,r:[65|66]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	{
	    SDI12_CMD_SET_TDS_UNITS,
	    "Set TDS Units",
	    "aXDU${n:units_id,t:i,r:[13|14]}!",
	    "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	    SDI12_CMD_CONFIG_SEND_BREAK | SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	    NULL,
	    SDI12_CMD_STD_SEND_DATA,
	    NULL
	},
	// End of command list indication
	{ NULL, NULL, NULL, NULL, SDI12_CMD_CONFIG_NONE, NULL, NULL, NULL }
    }
};

const char *InSituAquaTROLL200SDI12::_CSV_HEADER_VALUES[] =
{
    "temperatureDegC",
    "conductivityUSCm",         "conductivityMSCm",
    "specificConductivityUSCm", "specificConductivityMSCm",
    "levelM",
    "pressureKPa",              "pressureIsAbsolute",
    NULL
};


InSituAquaTROLL200SDI12::InSituAquaTROLL200SDI12() :
    SensorSDI12(POWER_NONE, POWER_NONE, &_commandsDescription)
{
  clearReadingsFormat();
  clearReadings();
}

InSituAquaTROLL200SDI12::~InSituAquaTROLL200SDI12()
{
  // Do nothing
}

Sensor *InSituAquaTROLL200SDI12::getNewInstance()
{
  return new InSituAquaTROLL200SDI12();
}

const char *InSituAquaTROLL200SDI12::type()
{
  return TYPE_NAME;
}


/**
 * Clear the reading values.
 */
void InSituAquaTROLL200SDI12::clearReadings()
{
  this->_tempDegC             = 0.0;
  this->_conductivity         = 0.0;
  this->_specificConductivity = 0.0;
  this->_levelM               = 0.0;
  this->_pressKPa             = 0.0;
  this->_pressAbs             = false;
  this->_readingFlags         = READING_FLAG_NONE;
}


bool InSituAquaTROLL200SDI12::readOnAlarmChange() { return false; }


/**
 * Clear the cached readings format so that the next time we need
 * it we'll ask the sensor about it's current readings values format and order.
 */
void InSituAquaTROLL200SDI12::clearReadingsFormat()
{
  this->_readingsFormat[0] = '\0';
}

/**
 * Get the format, and the order, of the reading values sent by the sensor.
 *
 * The format is cached so that we don't ask the format every time we need it.
 * The format should never change, except in rare occasions where
 * the sensor's readings configuration is changed.
 *
 * @return the format. Can be empty if the sensor do not return any value.
 * @return NULL if failed to get the format from the sensor.
 */
const char *InSituAquaTROLL200SDI12::readingsFormat()
{
  if(!*this->_readingsFormat)
  {
    // We do not already have the format, ask the sensor for it.
    const SDI12Command *pvCmd;
    if(!sendCommandUsingName(SDI12_CMD_ISCO_COMPATIBILITY, NULL, &pvCmd))
    {
      log_error_sensor(logger, "Failed to get readings format from the sensor.");
      return NULL;
    }
    sdi12_gen_get_cmd_response_value_as_string(pvCmd, "pairs",
					       this->_readingsFormat, sizeof(this->_readingsFormat),
					       "", NULL);
    log_info_sensor(logger, "Readings format is: %s.", this->readingsFormat());
  }

  return (const char *)this->_readingsFormat;
}

bool InSituAquaTROLL200SDI12::readSpecific()
{
  const SDI12Command    *pvCmd;
  SDI12CmdValuesIterator it;
  SDI12CmdValue          value;
  const char            *psFormat, *pc;
  uint8_t                nb_retries;
  bool                   ok = true;

  clearReadings();

  // Get the readings format
  if(!(psFormat = readingsFormat())) { goto error_exit; }

  // Get the readings
  for(nb_retries = 3; ; )
  {
#if defined SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS && SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS != 0
    board_delay_ms(SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS);
#else
#if defined SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS && SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS != 0
    board_delay_ms(SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS);
#endif
#endif
    if(sendCommandUsingName(SDI12_CMD_STD_START_MEASUREMENT_WITH_CRC, NULL, &pvCmd)) break;

    if(!--nb_retries) { goto error_exit; }
  }
  log_info_sensor(logger, "Readings: %.*s.",
		  strlen((const char *)pvCmd->pu8_buffer) - 2,
		  (const char *)pvCmd->pu8_buffer);

  // Now use the readings format to know what reading values matches the values we are interested in
  if(!sdi12_cmd_get_iterator_on_values(pvCmd, &it)) { goto error_exit; }
  for(pc = psFormat; pc[0] && pc[1] && sdi12_cmd_iterator_get_next_value(&it, &value); pc += 2)
  {
    switch(*pc)
    {
      case 'd':
	// Value is a pressure
	if(pc[1] == '0' || pc[1] == '2')
	{
	  this->_pressKPa      = units_PSI_to_kPa(value.value.float_value);
	  this->_pressAbs      = (pc[1] == '0');
	  this->_readingFlags |= READING_FLAG_PRESSURE;
	}
	break;

      case 'A':
	// Value is a temperature
	if(pc[1] == '0' || pc[1] == '1')
	{
	  this->_tempDegC      = value.value.float_value;
	  if(pc[1] == '1') { this->_tempDegC = units_degF_to_degC(this->_tempDegC); }
	  this->_readingFlags |= READING_FLAG_TEMPERATURE;
	}
	break;

      case 'I':
	// Value is level
	if(pc[1] == '0' || pc[1] == '1')
	{
	  this->_levelM        = value.value.float_value;
	  if(pc[1] == '1') { this->_levelM = units_feet_to_meters(this->_levelM); }
	  this->_readingFlags |= READING_FLAG_LEVEL;
	}
	break;

      case 'B':
	// Value is conductivity
	if(pc[1] == '0' || pc[1] == '1')
	{
	  this->_conductivity     = value.value.float_value;
	  this->_conductivityType = (pc[1] == '0') ?
	      CNSSRF_DT_SOLUTION_CONDUCTIVITY_MSCM :
	      CNSSRF_DT_SOLUTION_CONDUCTIVITY_USCM;
	  this->_readingFlags    |= READING_FLAG_CONDUCTIVITY;
	}
	break;

      case 'C':
	// Value is specific conductivity
	if(pc[1] == '0' || pc[1] == '1')
	{
	  this->_specificConductivity     = value.value.float_value;
	  this->_specificConductivityType = (pc[1] == '0') ?
	      CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_MSCM :
	      CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_USCM;
	  this->_readingFlags            |= READING_FLAG_SPECIFIC_CONDUCTIVITY;
	}
	break;

      default:
	log_warn(logger, "Value with id '%c%c' is not handled.", pc[0], pc[1]);
	continue;
    }
  }

  // Check that we got all the readings that we were expecting
  if(!(this->_readingFlags & READING_FLAG_TEMPERATURE))           { ok = false; log_error_sensor(logger, "Failed to get temperature reading.");           }
  if(!(this->_readingFlags & READING_FLAG_SPECIFIC_CONDUCTIVITY)) { ok = false; log_error_sensor(logger, "Failed to get specific conductivity reading."); }
  if(!(this->_readingFlags & READING_FLAG_LEVEL))                 { ok = false; log_error_sensor(logger, "Failed to get level reading.");                 }
  if(!ok) { goto error_exit; }

  return true;

  error_exit:
  return false;
}


bool InSituAquaTROLL200SDI12::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  CNSSRFDTSolutionConductivityFlags conductivityFlags = CNSSRF_DT_SOLUTION_CONDUCTIVITY_FLAG_NONE;
  CNSSRFDTPressurePaFlags           pressureFlags     = CNSSRF_DT_PRESSURE_PA_FLAG_NONE;
  CNSSRFDTTemperatureDegCFlag       temperatureFlags  = CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE;
  bool                              ok                = true;

  if(this->_pressAbs) { pressureFlags |= CNSSRF_DT_PRESSURE_PA_FLAG_ABS; }

  if(this->_readingFlags & READING_FLAG_TEMPERATURE)
  {
    ok = ok && cnssrf_dt_temperature_write_degc_to_frame(pvFrame,
							 this->_tempDegC,
							 temperatureFlags);
  }
  if(this->_readingFlags & READING_FLAG_CONDUCTIVITY)
  {
    ok = ok && cnssrf_dt_solutionconductivity_write_to_frame(pvFrame,
							     this->_conductivity,
							     this->_conductivityType,
							     conductivityFlags);
  }
  if(this->_readingFlags & READING_FLAG_SPECIFIC_CONDUCTIVITY)
  {
    ok = ok && cnssrf_dt_solutionconductivity_write_to_frame(pvFrame,
							     this->_specificConductivity,
							     this->_specificConductivityType,
							     conductivityFlags);
  }
  if(this->_readingFlags & READING_FLAG_LEVEL)
  {
    ok = ok && cnssrf_dt_level_write_m_to_frame(pvFrame, this->_levelM);
  }
  if(this->_readingFlags & READING_FLAG_PRESSURE)
  {
    ok = ok && cnssrf_dt_pressure_write_pa_to_frame(pvFrame,
						    this->_pressKPa * 1000.0,
						    pressureFlags);
  }

  return ok;
}



const char **InSituAquaTROLL200SDI12::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t InSituAquaTROLL200SDI12::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len, i;
  char     values[   8][10];
  char    *ppsValues[8];

  // Set all values to empty string
  for(i = 0; i < 8; i++)
  {
    values[i][0] = '\0';
    ppsValues[i] = values[i];
  }

  // Get values as strings
  len = 0;
  if(this->_readingFlags & READING_FLAG_TEMPERATURE)
  {
    sprintf(values[len], "%.1f", this->_tempDegC);
  }
  len++;
  if(this->_readingFlags & READING_FLAG_CONDUCTIVITY)
  {
    switch(this->_conductivityType)
    {
      case CNSSRF_DT_SOLUTION_CONDUCTIVITY_USCM:
	sprintf(values[len],   "%.3f", this->_conductivity);
	len += 2;
	break;

      case CNSSRF_DT_SOLUTION_CONDUCTIVITY_MSCM:
	sprintf(values[++len], "%.3f", this->_conductivity);
	len++;
	break;

      default:
	len += 2;
	break;
    }
  }
  else { len += 2; }
  if(this->_readingFlags & READING_FLAG_SPECIFIC_CONDUCTIVITY)
  {
    switch(this->_specificConductivityType)
    {
      case CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_USCM:
	sprintf(values[len],   "%.3f", this->_specificConductivity);
	len += 2;
	break;

      case CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_MSCM:
	sprintf(values[++len], "%.3f", this->_specificConductivity);
	len++;
	break;

      default:
	len += 2;
	break;
    }
  }
  else { len += 2; }
  if(this->_readingFlags & READING_FLAG_LEVEL)
  {
    sprintf(values[len], "%.3f", this->_levelM);
  }
  len++;
  if(this->_readingFlags & READING_FLAG_PRESSURE)
  {
    sprintf(values[len++], "%.4f", this->_pressKPa);
    strcpy( values[len++], this->_pressAbs ? "True" : "False");
  }
  else { len += 2; }

  // Write CSV string
  return csvMakeStringUsingStringValues(ps_data, size, (const char **)ppsValues, len);
}


#endif // USE_SENSOR_INSITU_AQUATROLL_200_SDI12
