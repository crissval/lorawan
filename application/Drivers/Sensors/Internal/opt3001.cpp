#include <string.h>
#include "opt3001.hpp"
#include "board.h"


#define I2C_ADDRESS  0b10001010

// OPT3001 registers
#define REG_RESULT       0x00
#define REG_CONFIGURE    0x01
#define REG_INT_LIMIT_L  0x02
#define REG_INT_LIMIT_H  0x03
#define REG_MAN_ID       0x7e
#define REG_DEV_ID       0x7f

// OPT3001 register values
#define MANUFACTURER_ID  0x5449
#define DEVICE_ID        0x3001

#define CFG_RANGE_POS         12
#define CFG_RANGE_40_95      (0x0 << CFG_RANGE_POS)
#define CFG_RANGE_81_90      (0x1 << CFG_RANGE_POS)
#define CFG_RANGE_163_80     (0x2 << CFG_RANGE_POS)
#define CFG_RANGE_327_60     (0x3 << CFG_RANGE_POS)
#define CFG_RANGE_655_20     (0x4 << CFG_RANGE_POS)
#define CFG_RANGE_1310_40    (0x5 << CFG_RANGE_POS)
#define CFG_RANGE_2620_80    (0x6 << CFG_RANGE_POS)
#define CFG_RANGE_5241_60    (0x7 << CFG_RANGE_POS)
#define CFG_RANGE_10483_20   (0x8 << CFG_RANGE_POS)
#define CFG_RANGE_20966_40   (0x9 << CFG_RANGE_POS)
#define CFG_RANGE_41932_80   (0xA << CFG_RANGE_POS)
#define CFG_RANGE_83865_60   (0xB << CFG_RANGE_POS)
#define CFG_RANGE_AUTOMATIC  (0xC << CFG_RANGE_POS)

#define CFG_CONV_TIME_POS      11
#define CFG_CONV_TIME_100_MS   0x0
#define CFG_CONV_TIME_800_MS  (0x1 << CFG_CONV_TIME_POS)

#define CFG_CONV_MODE_POS           9
#define CFG_CONV_MODE_MSK          (0x3 << CFG_CONV_MODE_POS)
#define CFG_CONV_MODE_SHUTDOWN      0x0
#define CFG_CONV_MODE_SINGLE_SHOT  (0x1 << CFG_CONV_MODE_POS)
#define CFG_CONV_MODE_CONTINUOUS   (0x2 << CFG_CONV_MODE_POS)

#define CFG_OVERFLOW_POS      8
#define CFG_OVERFLOW_FLAG    (0x1 << CFG_OVERFLOW_POS)

#define CFG_CONV_READY_POS    7
#define CFG_CONV_READY_FLAG  (0x1 << CFG_CONV_READY_POS)

#define CFG_TH_HIGH_POS       6
#define CFG_TH_HIGH_FLAG     (0x1 << CFG_TH_HIGH_POS)
#define CFG_TH_LOW_POS        5
#define CFG_TH_LOW_FLAG      (0x1 << CFG_TH_LOW_POS)

#define CFG_INT_MODE_POS          4
#define CFG_INT_MODE_MSK         (0x1 << CFG_INT_MODE_POS)
#define CFG_INT_MODE_HYSTERESIS   0x0
#define CFG_INT_MODE_LATCHED     (0x1 << CFG_INT_MODE_POS)

#define CFG_INT_POL_POS           3
#define CFG_INT_POL_ACTIVE_LOW    0x0
#define CFG_INT_POL_ACTIVE_HIGH  (0x1 << CFG_INT_POL_POS)

#define CFG_EXP_MASK_POS   2
#define CFG_EXP_MASK      (0x1 << CFG_EXP_MASK_POS)

#define CFG_FAULT_COUNT_POS  0
#define CFG_FAULT_COUNT_MSK  0x3
#define CFG_FAULT_COUNT_1    0x0
#define CFG_FAULT_COUNT_2    0x1
#define CFG_FAULT_COUNT_4    0x2
#define CFG_FAULT_COUNT_8    0x3

#define RAW_VALUE_MIN  0x0
#define RAW_VALUE_MAX  0xBFFF

#define RAW_VALUE_LOW_LIMIT_RESET   RAW_VALUE_MIN
#define RAW_VALUE_HIGH_LIMIT_RESET  RAW_VALUE_MAX


#define GET_READING_TIMEOUT_MS  3000
#define STATE_VERSION           1
#define DEFAULT_FAULT_COUNT     CFG_FAULT_COUNT_8


  CREATE_LOGGER(opt3001);
#undef  logger
#define logger  opt3001


const char *OPT3001::TYPE_NAME = "OPT3001";

const char *OPT3001::_CSV_HEADER_VALUES[] = { "illuminanceLux", NULL };


OPT3001::OPT3001() : SensorInternal(I2C_ADDRESS, BIG_ENDIAN)
{
  this->_illuminanceLux    = 0.0;
  this->_regCFGBase        =
      CFG_RANGE_AUTOMATIC     | CFG_CONV_TIME_800_MS | DEFAULT_FAULT_COUNT |
      CFG_INT_POL_ACTIVE_HIGH | CFG_INT_MODE_HYSTERESIS;

  clearAlarmParameters();
}

/**
 * Clear the alarm parameters, and thus the alarm functionality is disabled.
 */
void OPT3001::clearAlarmParameters()
{
  this->_alarmThLowSetRaw  = this->_alarmThLowClearRaw  = RAW_VALUE_MIN;
  this->_alarmThHighSetRaw = this->_alarmThHighClearRaw = RAW_VALUE_MAX;
  this->_useAlarm          = this->_useLowAlarm         = this->_useHighAlarm = false;
}

Sensor *OPT3001::getNewInstance()
{
  return new OPT3001();
}

const char *OPT3001::type()
{
  return TYPE_NAME;
}

Sensor::State *OPT3001::defaultState(uint32_t *pu32_size)
{
  this->_state.version = STATE_VERSION;

  *pu32_size = sizeof(State);
  return &this->_state;
}


bool OPT3001::openSpecific()
{
  return SensorI2C::openSpecific() && iAmOPT3001();
}

bool OPT3001::readSpecific()
{
  uint16_t u16;
  uint32_t tickStart;

  this->_illuminanceLux = 0.0;

  // Start reading
  if(!(this->_regCFGBase & CFG_CONV_MODE_CONTINUOUS))
  {
    // Not continuous mode, so trigger conversion.
    if(!i2cWrite2BytesToRegister(REG_CONFIGURE, this->_regCFGBase | CFG_CONV_MODE_SINGLE_SHOT))
    { goto error_exit; }
  }

  // Wait for result
  for(tickStart = HAL_GetTick(); 1; HAL_Delay(1))
  {
    if(i2cRead2BytesFromRegister(REG_CONFIGURE, &u16) && (u16 & CFG_CONV_READY_FLAG)) { break; }
    if(HAL_GetTick() - tickStart > GET_READING_TIMEOUT_MS) { goto error_exit; }
  }

  // Get result
  if(!i2cRead2BytesFromRegister(REG_RESULT, &u16)) { goto error_exit; }
  this->_illuminanceLux = rawToLux(u16);

  // Set up the next alarm
  if(!configureNextAlarm(this->_illuminanceLux))
  {
    log_error_sensor(logger, "Failed to set up next alarm.");
    goto error_exit;
  }

  return true;

  error_exit:
  return false;
}


bool OPT3001::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  int32_t i32;
  bool    ok = true;

  // Set up fault filter duration
  if(json["filterNb"].success())
  {
    i32 = json["filterNb"].as<int32_t>();
    this->_regCFGBase &= ~CFG_FAULT_COUNT_MSK;
    switch(i32)
    {
      case 1: break;  // Do nothing
      case 2: this->_regCFGBase |= CFG_FAULT_COUNT_2; break;
      case 4: this->_regCFGBase |= CFG_FAULT_COUNT_4; break;
      case 8: this->_regCFGBase |= CFG_FAULT_COUNT_8; break;
      default:
	this->_regCFGBase |= DEFAULT_FAULT_COUNT;
	log_error_sensor(logger, "Alarm 'filterSec' parameter must be in range [1..7].");
    }
  }

  this->_alarmThLowSetRaw    = luxToRaw(json["lowSetLux"]   .as<float>());
  this->_alarmThLowClearRaw  = luxToRaw(json["lowClearLux"] .as<float>());
  this->_alarmThHighSetRaw   = luxToRaw(json["highSetLux"]  .as<float>());
  this->_alarmThHighClearRaw = luxToRaw(json["highClearLux"].as<float>());

  if((this->_useLowAlarm = this->_alarmThLowSetRaw && this->_alarmThLowClearRaw))
  {
    if(this->_alarmThLowClearRaw < this->_alarmThLowSetRaw)
    {
      log_error_sensor(logger, "Alarm parameter 'lowClearLux' cannot be less than 'lowSetLux'.");
      ok = false;
    }
  }

  if((this->_useHighAlarm = this->_alarmThHighSetRaw && this->_alarmThHighClearRaw))
  {
    if(this->_alarmThHighClearRaw > this->_alarmThHighSetRaw)
    {
      log_error_sensor(logger, "Alarm parameter 'highClearLux' cannot be greater than 'highSetLux'.");
      ok = false;
    }
  }

  if(ok)
  {
    if(!_useLowAlarm)  { this->_alarmThLowSetRaw  = this->_alarmThLowClearRaw  = RAW_VALUE_MIN; }
    if(!_useHighAlarm) { this->_alarmThHighSetRaw = this->_alarmThHighClearRaw = RAW_VALUE_MAX; }
    this->_useAlarm = this->_useLowAlarm | this->_useHighAlarm;
  }
  else { this->_useAlarm = false; }

  if(this->_useAlarm) { *pvAlarmInts = CNSSInt::OPT3001_FLAG; }
  else
  {
    clearAlarmParameters();
    *pvAlarmInts = CNSSInt::INT_FLAG_NONE;
  }

  return ok;
}


bool OPT3001::currentValuesAreInAlarmRange()
{
  uint16_t valueRaw = luxToRaw(this->_illuminanceLux);

  return isInAlarm() ?
      (valueRaw <  this->_alarmThLowClearRaw || valueRaw >  this->_alarmThHighClearRaw) :
      (valueRaw <= this->_alarmThLowSetRaw   || valueRaw >= this->_alarmThHighSetRaw);
}

/**
 * Program the OPT3001 with the parameters for the next alarm.
 *
 * @pre the I2C communication MUST have previously been set up.
 *
 * @param[in] valueLux the current illuminance value, in Lux.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool OPT3001::configureNextAlarm(float valueLux)
{
  uint16_t lowLimitRaw, highLimitRaw, valueRaw;
  bool     ok  = true;

  valueRaw     = luxToRaw(valueLux);
  lowLimitRaw  = RAW_VALUE_MIN;
  highLimitRaw = RAW_VALUE_MAX;

  if(this->_useAlarm)
  {
    if(valueRaw >= this->_alarmThLowClearRaw && valueRaw <= this->_alarmThHighClearRaw)
    {
      lowLimitRaw  = this->_alarmThLowSetRaw;
      highLimitRaw = this->_alarmThHighSetRaw;
    }
    else if(valueRaw <= this->_alarmThLowSetRaw)  { highLimitRaw = this->_alarmThLowClearRaw;  }
    else if(valueRaw >= this->_alarmThHighSetRaw) { lowLimitRaw  = this->_alarmThHighClearRaw; }
    else if(valueRaw <  this->_alarmThLowClearRaw)
    {
      if(isInAlarm()) { highLimitRaw = this->_alarmThLowClearRaw; }
      else
      {
	lowLimitRaw  = this->_alarmThLowSetRaw;
	highLimitRaw = this->_alarmThHighSetRaw;
      }
    }
    else  // ValueRaw > this->_alarmThHighClearRaw
    {
      if(isInAlarm()) { lowLimitRaw  = this->_alarmThHighClearRaw; }
      else
      {
	lowLimitRaw  = this->_alarmThLowSetRaw;
	highLimitRaw = this->_alarmThHighSetRaw;
      }
    }
  }
  // Else do nothing

  // set up the alarms mode
  this->_regCFGBase &= ~CFG_INT_MODE_MSK;
  this->_regCFGBase |= CFG_INT_MODE_LATCHED;

  // Set up the conversion type
  if(lowLimitRaw == RAW_VALUE_MIN && highLimitRaw == RAW_VALUE_MAX)
  {
    // No alarm
    this->_regCFGBase &= ~CFG_CONV_MODE_MSK;  // Set shutdown mode
  }
  else
  {
    this->_regCFGBase |=  CFG_CONV_MODE_CONTINUOUS; // Set continuous conversions
//    ConnecSenS::addVerboseSyslogEntry(name(),
//  				    "Set alert limits to (low, high): (%f, %f) lux.",
//  				    rawToLux(lowLimitRaw), rawToLux(highLimitRaw));
  }

  // Write to the chip
  ok &= i2cWrite2BytesToRegister(REG_CONFIGURE,   this->_regCFGBase) &&
      i2cWrite2BytesToRegister(  REG_INT_LIMIT_L, lowLimitRaw)       &&
      i2cWrite2BytesToRegister(  REG_INT_LIMIT_H, highLimitRaw);

  return ok;
}


void OPT3001::processAlarmInterruption(CNSSInt::Interruptions ints)
{
  uint16_t valueRaw;

  if(this->_useAlarm && open() && i2cRead2BytesFromRegister(REG_RESULT, &valueRaw))
  {
    this->_illuminanceLux = rawToLux(valueRaw);
    configureNextAlarm(this->_illuminanceLux);
    setIsInAlarm(currentValuesAreInAlarmRange());
    i2cRead2BytesFromRegister(REG_CONFIGURE, &valueRaw);  // To clear default flags
    close();
  }
}


bool OPT3001::iAmOPT3001()
{
  uint16_t u16;

  return i2cRead2BytesFromRegister(REG_MAN_ID, &u16) && u16 == MANUFACTURER_ID &&
      i2cRead2BytesFromRegister(   REG_DEV_ID, &u16) && u16 == DEVICE_ID;
}


float OPT3001::rawToLux(uint16_t raw)
{
  return 0.01 * (2 << (raw >> 12)) * (raw & 0x0FFF);
}

uint16_t OPT3001::luxToRaw(float lux)
{
  uint16_t exponent;

  if(      lux <= 40.95)    exponent = 0x00;
  else if (lux <= 81.9)     exponent = 0x01;
  else if (lux <= 163.80)   exponent = 0x02;
  else if (lux <= 327.60)   exponent = 0x03;
  else if (lux <= 655.20)   exponent = 0x04;
  else if (lux <= 1310.40)  exponent = 0x05;
  else if (lux <= 2620.80)  exponent = 0x06;
  else if (lux <= 5241.60)  exponent = 0x07;
  else if (lux <= 10483.20) exponent = 0x08;
  else if (lux <= 20966.40) exponent = 0x09;
  else if (lux <= 41932.80) exponent = 0x0A;
  else                      exponent = 0x0B;

  return (exponent << 12) | (((uint16_t)(((uint32_t)(lux * 100)) / (2 << exponent))) & 0x0FFF);
}


bool OPT3001::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_solar_write_lux_to_frame(pvFrame, (uint16_t)this->_illuminanceLux);
}


const char **OPT3001::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t OPT3001::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.1f", this->_illuminanceLux);

  return len >= size ? -1 : (int32_t)len;
}



