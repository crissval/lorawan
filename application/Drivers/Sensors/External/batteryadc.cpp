/*
 * Battery monitoring using the µC ADC.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 */

#include <string.h>
#include "batteryadc.hpp"
#ifdef USE_SENSOR_BATTERY_ADC
#include "cnssrf-dt_battvoltage.h"


  CREATE_LOGGER(batteryadc);
#undef  logger
#define logger  batteryadc


const char *BatteryADC::TYPE_NAME            = "batteryADC";
const char *BatteryADC::_CSV_HEADER_VALUES[] = { "batteryV", NULL };



BatteryADC::BatteryADC() : SensorADC(POWER_NONE, POWER_NONE, ADC_LINE_NONE)
{
  this->_adcLine        = ADC_LINE_NONE;
  this->_voltageDivider = 1.0;
}

BatteryADC::~BatteryADC()
{
  // Do nothing
}

Sensor *BatteryADC::getNewInstance()
{
  return new BatteryADC();
}

const char *BatteryADC::type()
{
  return TYPE_NAME;
}


bool BatteryADC::jsonSpecificHandler(const JsonObject& json)
{
  const char        *ps;
  float              f;
  const ADCLineDesc *pvDesc;

  // Get the ADC line used
  if(!(ps = json["useADCLine"].as<const char *>()))
  {
    log_error_sensor(logger, "You must specify the configuration parameter 'useADCLine'.");
    goto error_exit;
  }
  if(!(pvDesc = lineDescUsingName(ps)))
  {
    log_error_sensor(logger, "Invalid 'useADCLine' parameter value: '%s'.", ps);
    goto error_exit;
  }
  setADCLines(     pvDesc->idFlag);
  this->_adcLine = pvDesc->id;

  // Get input voltage divider value
  if(!(this->_voltageDivider = json["voltageDivisor"].as<float>()))
  {
    log_error_sensor(logger, "You must specify configuration parameter 'voltageDivisor'.");
    goto error_exit;
  }
  if(this->_voltageDivider < 1)
  {
    log_error_sensor(logger, "Parameter 'voltageDivisor' must be >= 1.");
    goto error_exit;
  }

  // Get the battery type.
  ps = json["batteryType"]    .as<const char *>();
  f  = json["batteryVoltageV"].as<float>();
  if(!*ps) { ps = NULL; }
  if((ps && !f) || (!ps && f))
  {
    log_error_sensor(logger, "Both 'batteryType' and 'batteryVoltageV' parameters MUST be defined; one cannot go without the other.");
    goto error_exit;
  }
  if(ps && f && !battery_init_with_voltage_and_type_name(&this->_battery, ps, (uint32_t)(f * 1000)))
  {
    log_error_sensor(logger, "Failed to initialise battery sensor using type and voltage given in configuration.");
    goto error_exit;
  }

  // Get the low battery voltage value if one is set in the configuration file
  if(json["batteryLowV"].success() && (f = json["batteryLowV"].as<float>()) <= 0)
  {
    log_error_sensor(logger, "Parameter 'battLowV' must be > 0 Volts.");
    goto error_exit;
  }
  battery_set_voltage_low_mv(&this->_battery, (uint32_t)(f * 1000));

  return true;

  error_exit:
  return false;
}

bool BatteryADC::readSpecific()
{
  if(!SensorADC::readSpecific()) { return false; }

  battery_set_voltage_mv(&this->_battery,
			 (uint32_t)(adcValueMV(this->_adcLine) * this->_voltageDivider));

  log_info_sensor(logger, "Battery voltage: %.2fV.", (float)battery_voltage_mv(&this->_battery) / 1000.0);

  return true;
}


bool BatteryADC::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_battvoltageflags_write_millivolts_to_frame(
      pvFrame,
      battery_voltage_is_low(&this->_battery) ? CNSSRF_BATTVOLTAGE_FLAG_BATT_LOW : CNSSRF_BATTVOLTAGE_FLAG_NONE,
      battery_voltage_mv(    &this->_battery));
}


const char **BatteryADC::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}


int32_t BatteryADC::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.3f", battery_voltage_mv(&this->_battery) / 1000.0);

  return len >= size ? -1 : (int32_t)len;
}


#endif // USE_SENSOR_BATTERY_ADC
