/*
 * Driver for the soil humidity sensor made by Laure
 *
 *  Created on: 9 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "soilmoisturewatermarki2c.hpp"
#ifdef USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C
#include "cnssrf-dt_soilmoisture.h"
#include "sdcard.h"

#define I2C_ADDRESS                                    0x20
#define TIME_TO_WAIT_BETWEEN_READ_CMD_AND_RESPONSE_MS  2000
#define CENTIBAR_ERROR                                 255

#define STATE_VERSION  1

  CREATE_LOGGER(soilmoisturewatermarki2c);
#undef  logger
#define logger  soilmoisturewatermarki2c


const char *SoilMoistureWatermakI2C::TYPE_NAME = "soilMoistureWatermarkI2C";

const char *SoilMoistureWatermakI2C::_CSV_HEADER_VALUES[] =
{
    "1-DepthCm", "1-soilMoistureCb", "1-soilTemperatureDegC", "1-freqHz",
    "2-DepthCm", "2-soilMoistureCb", "2-soilTemperatureDegC", "2-freqHz",
    "3-DepthCm", "3-soilMoistureCb", "3-soilTemperatureDegC", "3-freqHz",
    NULL
};


SoilMoistureWatermakI2C::SoilMoistureWatermakI2C() :
    SensorI2C(POWER_EXTERNAL, POWER_NONE,
	      INTERFACE_EXTERNAL, I2C_ADDRESS, LITTLE_ENDIAN,
	      OPTION_FIXED_ADDR)
{
  uint8_t     i;
  SensorData *pvData;

  for(i = 0, pvData = this->_sensorData; i < SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX; i++, pvData++)
  {
    pvData->depthCm   = 0;
    pvData->centibars = 0;
  }

  clearAlarm();
}

SoilMoistureWatermakI2C::~SoilMoistureWatermakI2C()
{
  // Do nothing
}

Sensor *SoilMoistureWatermakI2C::getNewInstance()
{
  return new SoilMoistureWatermakI2C();
}

const char *SoilMoistureWatermakI2C::type()
{
  return TYPE_NAME;
}


Sensor::State *SoilMoistureWatermakI2C::defaultState(uint32_t *pu32_size)
{
  this->_state.base.version = STATE_VERSION;
  this->_state.alarmLowSet  = 0;
  this->_state.alarmHighSet = 0;

  *pu32_size = sizeof(this->_state);
  return &this->_state.base;
}


bool SoilMoistureWatermakI2C::readSpecific()
{
  uint8_t     data[9], i;
  SensorData *pvData;
  bool        alarmSet     = false;
  bool        alarmCleared = false;

  // Get the state file contents
  StateSpecific *pvState;
  if(!(pvState = (StateSpecific *)state())) { return false; }

  // Get the data from the sensors
  if(!i2cTransfert((const uint8_t *)"M", 1,
		   data,                 sizeof(data),
		   TIME_TO_WAIT_BETWEEN_READ_CMD_AND_RESPONSE_MS)) { return false; }

  for(i = 0, pvData = this->_sensorData; i < SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX; i++, pvData++)
  {
    pvData->freqHz         = ((uint16_t)data[i * 2]) | (((uint16_t)data[i * 2 + 1]) << 8);
    pvData->centibars      = data[SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX * 2 + i];
    pvData->tempDegC       = DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_ERROR;
    pvData->alarmLowActive = pvData->alarmHighActive = false;

    if(pvData->centibars != CENTIBAR_ERROR && this->_alarmIsSet)
    {
      if(pvState->alarmHighSet & (1u << i))
      {
	// The high alarm is set for sensor i
	pvData->alarmHighActive = true;
	if(pvData->centibars <= this->_alarmHighClearCb)
	{
	  pvData->alarmHighActive = false;
	  alarmCleared            = true;
	  pvState->alarmHighSet  &= ~(1u << i);
	}
      }
      else
      {
	// The high alarm is not set for sensor i
	if(pvData->centibars >= this->_alarmHighSetCb)
	{
	  pvData->alarmHighActive = true;
	  alarmSet                = true;
	  pvState->alarmHighSet  |= 1u << i;
	}
      }
      if(pvState->alarmLowSet & (1u << i))
      {
	// The low alarm is set for sensor i
	pvData->alarmLowActive = true;
	if(pvData->centibars >= this->_alarmLowClearCb)
	{
	  pvData->alarmLowActive = false;
	  alarmCleared           = true;
	  pvState->alarmLowSet  &= ~(1u << i);
	}
      }
      else
      {
	// The low alarm is not set for sensor i
	if(pvData->centibars <= this->_alarmHighSetCb)
	{
	  pvData->alarmLowActive  = true;
	  alarmSet                = true;
	  pvState->alarmLowSet   |= 1u << i;
	}
      }
    }
  }

  if(     alarmSet)     { setIsInAlarm(true);  }
  else if(alarmCleared) { setIsInAlarm(false); }
  saveState();

  return true;
}

/**
 * Indicate if we have to read the sensor when the alarm status changes.
 *
 * @return true  if we have to read it.
 * @return false otherwise.
 */
bool SoilMoistureWatermakI2C::readOnAlarmChange() { return false; }


void SoilMoistureWatermakI2C::clearAlarm()
{
  this->_alarmIsSet      = false;
  this->_alarmLowSetCb   = this->_alarmLowClearCb  = 0;
  this->_alarmHighSetCb  = this->_alarmHighClearCb = 0;
}


bool SoilMoistureWatermakI2C::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  UNUSED(json);
  UNUSED(pvAlarmInts);

  // Get the thresholds values
  if(json["lowSetCb"].success())
  {
    this->_alarmLowSetCb = json["lowSetCb"].as<uint8_t>();
    this->_alarmIsSet    = true;
  }
  if(json["lowClearCb"].success())
  {
    this->_alarmLowClearCb = json["lowClearCb"].as<uint8_t>();
    this->_alarmIsSet      = true;
  }
  if(json["highSetCb"].success())
  {
    this->_alarmHighSetCb = json["highSetCb"].as<uint8_t>();
    this->_alarmIsSet     = true;
  }
  if(json["highClearCb"].success())
  {
    this->_alarmHighClearCb = json["highClearCb"].as<uint8_t>();
    this->_alarmIsSet       = true;
  }
  if(!this->_alarmIsSet) { return true; }

  // Check their consistency
  // For low thresholds
  if( ( this->_alarmLowSetCb && !this->_alarmLowClearCb) ||
      (!this->_alarmLowSetCb &&  this->_alarmLowClearCb))
  {
    log_error_sensor(logger, "To activate low alarm both parameters 'lowSetCb' and 'lowClearCb' must be set.");
    goto exit_error;
  }
  if(this->_alarmLowSetCb && this->_alarmLowClearCb <= this->_alarmLowSetCb)
  {
    log_error_sensor(logger, "'lowClearCb' value must be strictly higher than 'lowSetCb'.");
    goto exit_error;
  }
  // For high thresholds
  if( ( this->_alarmHighSetCb && !this->_alarmHighClearCb) ||
      (!this->_alarmHighSetCb &&  this->_alarmHighClearCb))
  {
    log_error_sensor(logger, "To activate high alarm both parameters 'highSetCb' and 'highClearCb' must be set.");
    goto exit_error;
  }
  if(this->_alarmHighSetCb && this->_alarmHighClearCb >= this->_alarmHighSetCb)
  {
    log_error_sensor(logger, "'highClearCb' value must be strictly lower than 'highSetCb'.");
    goto exit_error;
  }

  return true;

  exit_error:
  clearAlarm();
  return false;
}

bool SoilMoistureWatermakI2C::jsonSpecificHandler(const JsonObject& json)
{
  if(!SensorI2C::jsonSpecificHandler(json)) { return false; }

  // Get measurement depths
  this->_sensorData[0].depthCm = json["depth1Cm"].as<uint8_t>();
  this->_sensorData[1].depthCm = json["depth2Cm"].as<uint8_t>();
  this->_sensorData[2].depthCm = json["depth3Cm"].as<uint8_t>();
  if(!this->_sensorData[0].depthCm && !this->_sensorData[1].depthCm && !this->_sensorData[2].depthCm)
  {
    log_error_sensor(logger, "At least one of 'depth1Cm', 'depth2Cm' or 'depth3Cm' parameters must be set.");
    return false;
  }

  return true;
}

bool SoilMoistureWatermakI2C::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  uint8_t i, count;
  SensorData                          *pvData;
  CNSSRFDTSoilMoistureCbDegCHzReading *pvReading, readings[SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX];

  for(i = 0, count = 0, pvData = this->_sensorData, pvReading = readings;
      i < SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX;
      i++, pvData++)
  {
    if(pvData->depthCm)
    {
      pvReading->base.depth_cm  = pvData->depthCm;
      pvReading->base.centibars = pvData->centibars;
      pvReading->base.flags     = CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_NONE;
      if(pvData->alarmLowActive)  { pvReading->base.flags |= CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_LOW;  }
      if(pvData->alarmHighActive) { pvReading->base.flags |= CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_HIGH; }
      pvReading->tempDegC       = pvData->tempDegC;
      pvReading->freqHz         = pvData->freqHz;
      pvReading++;
      count++;
    }
  }

  return cnssrf_dt_soilmoisture_write_moisture_cbdegchz_to_frame(pvFrame, readings, count);
}


const char **SoilMoistureWatermakI2C::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t SoilMoistureWatermakI2C::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t    l, s;
  SensorData *pvSensorData;
  bool        firstValue;
  uint8_t     i;

  for(i = 0, s = size, firstValue = true; i < SOIL_MOISTURE_WATERMARK_I2C_NB_SENSORS_MAX; i++)
  {
    if(!firstValue)
    {
      if(s <= 1) { goto error_exit; }
      *ps_data++ = OUTPUT_DATA_CSV_SEP;
      s--;
    }

    pvSensorData = &this->_sensorData[i];
    l = snprintf(ps_data, s, "%u%c%u%c%.1f%c%u",
		 pvSensorData->depthCm,   OUTPUT_DATA_CSV_SEP,
		 pvSensorData->centibars, OUTPUT_DATA_CSV_SEP,
		 pvSensorData->tempDegC,  OUTPUT_DATA_CSV_SEP,
		 (unsigned int)pvSensorData->freqHz);
    if(l >= s) { goto error_exit; }
    ps_data   += l;
    s         -= l;
    firstValue = false;
  }

  return size - s;

  error_exit:
  return -1;
}

#endif  // USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C

