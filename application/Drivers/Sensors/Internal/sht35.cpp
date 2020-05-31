
#include <string.h>
#include "sht35.hpp"
#include "board.h"
#include "binhex.hpp"


#define SHT35_I2C_ADDRESS  (0x44 << 1)
#define repeatability      REPEATAB_MEDIUM
#define SHT35_TIMEOUT      50
#define frequency          FREQUENCY_HZ5
#define mode               MODE_CLKSTRETCH


  CREATE_LOGGER(sht35);
#undef  logger
#define logger  sht35


//=============================================================================
// IO-Pins                         /* -- adapt the defines for ConnecSens -- */
//-----------------------------------------------------------------------------
// Reset on port B, bit 12
//#define RESET_LOW()  (GPIOB->BSRR = 0x10000000) // set Reset to low
//#define RESET_HIGH() (GPIOB->BSRR = 0x00001000) // set Reset to high
// Alert on port B, bit 10
#define ALERT_READ   (GPIOB->IDR  & 0x0400)     // read Alert
//=============================================================================


const SHT35::AlertLimits SHT35::_DISABLE_THRESHOLDS =
{
    40.0f, 40.0f, // high set:   RH [%], T [°C]
    0.0f,  0.0f,  // high clear: RH [%], T [°C]
    0.0f,  0.0f,  // low clear:  RH [%], T [°C]
    50.0f, 50.0f  // low set:    RH [%], T [°C]
};


/**
 * Store the commands for a given (frequency, repeatability) couple.
 */
const SHT35::Command  SHT35::_FREQUENCY_REPEATABILITY_TO_COMMAND[15] =
{
    CMD_MEAS_PERI_05_L,  CMD_MEAS_PERI_1_L, CMD_MEAS_PERI_2_L, CMD_MEAS_PERI_4_L, CMD_MEAS_PERI_10_L,
    CMD_MEAS_PERI_05_M,  CMD_MEAS_PERI_1_M, CMD_MEAS_PERI_2_M, CMD_MEAS_PERI_4_M, CMD_MEAS_PERI_10_M,
    CMD_MEAS_PERI_05_H,  CMD_MEAS_PERI_1_H, CMD_MEAS_PERI_2_H, CMD_MEAS_PERI_4_H, CMD_MEAS_PERI_10_H
};


const char *SHT35::TYPE_NAME = "SHT35";

const char *SHT35::_CSV_HEADER_VALUES[] =
{
    "temperatureDegC", "airHumidityRelPercent", NULL
};


SHT35::SHT35() : SensorInternal(SHT35_I2C_ADDRESS, BIG_ENDIAN)
{
  this->_temperature  = this->_humidity = 0.0;
  this->_serialNumber = 0;
}

Sensor *SHT35::getNewInstance()
{
  return new SHT35();
}

const char *SHT35::type()
{
  return TYPE_NAME;
}


/**
 * Return the device serial number.
 *
 * @note If the device serial number has not been read yet then this function reads it.
 *
 * @return the serial number.
 * @return 0 in case of error.
 */
uint32_t SHT35::serialNumber()
{
  if(!this->_serialNumber)
  {
    log_info_sensor(logger, "Reading serial number...");
    if(open()) { log_info_sensor( logger, "Serial number is: %d.", this->_serialNumber); }
    else       { log_error_sensor(logger, "Failed to read serial number.");              }
    close();
  }

  return this->_serialNumber;
}

const char *SHT35::uniqueId()
{
  char hex[9];
  char buffer[SENSOR_UNIQUE_ID_SIZE_MAX];
  const char *psValue;

  if(*Sensor::uniqueId() || !*(psValue = SensorInternal::uniqueId()) || !serialNumber()) { goto exit; }

  // Append serial number to current uniqueId.
  strcpy(  buffer, psValue);
  binToHex(hex,   (const uint8_t *)&this->_serialNumber, 4, true);
  if( strlcat(buffer, SENSOR_INTERNAL_UNIQUE_ID_SEP_STR, sizeof(buffer)) >= sizeof(buffer) ||
      strlcat(buffer, hex,                               sizeof(buffer)) >= sizeof(buffer))
  { goto exit; }

  // Set the id
  setUniqueId(buffer);

  exit:
  return Sensor::uniqueId();
}


bool SHT35::openSpecific()
{
  return SensorI2C::openSpecific()         &&
      readSerialNumber()                   &&
      setAlertLimits(&_DISABLE_THRESHOLDS) &&
      clearAllAlertFlags();
}


bool SHT35::readSpecific()
{
  bool res;

  if((res = readPolling())) { setHasNewData(); }
  return res;
}

//=============================================================================
// Puts SHT in AlertMode by setting thresholds and activate Periodic Measurement.
//-----------------------------------------------------------------------------
bool SHT35::configAlertSpecific()
{
  return open()                          &&
      startPeriodicMeasurement()         &&
      setAlertLimits(&this->_thresholds) &&
      clearAllAlertFlags()               &&
      startPeriodicMeasurement();
}

bool SHT35::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  this->_thresholds.humidityHighSet      = json["humidityHighSetPercent"]  .as<float>();
  this->_thresholds.humidityHighClear    = json["humidityHighClearPercent"].as<float>();
  this->_thresholds.humidityLowSet       = json["humidityLowSetPercent"]   .as<float>();
  this->_thresholds.humidityLowClear     = json["humidityLowClearPercent"] .as<float>();
  this->_thresholds.temperatureHighSet   = json["temperatureHighSetDegC"]  .as<float>();
  this->_thresholds.temperatureHighClear = json["temperatureHighClearDegC"].as<float>();
  this->_thresholds.temperatureLowSet    = json["temperatureLowSetDegC"]   .as<float>();
  this->_thresholds.temperatureLowClear  = json["temperatureLowClearDegC"] .as<float>();

  bool noAlarm = this->_thresholds.humidityHighClear    == 0.0 && this->_thresholds.humidityHighSet    == 0.0  &&
                 this->_thresholds.humidityLowClear     == 0.0 && this->_thresholds.humidityLowSet     == 0.0  &&
                 this->_thresholds.temperatureHighClear == 0.0 && this->_thresholds.temperatureHighSet == 0.0  &&
                 this->_thresholds.temperatureLowClear  == 0.0 && this->_thresholds.temperatureLowSet  == 0.0;
  bool alarmOk = false;
  if(!noAlarm)
  {
    alarmOk    = this->_thresholds.humidityHighSet      >         this->_thresholds.humidityHighClear          &&
                 this->_thresholds.humidityLowSet       <         this->_thresholds.humidityLowClear           &&
                 this->_thresholds.temperatureHighSet   >         this->_thresholds.temperatureHighClear       &&
                 this->_thresholds.temperatureLowSet    <         this->_thresholds.temperatureLowClear;
  }

  *pvAlarmInts = alarmOk ? CNSSInt::SHT35_FLAG : CNSSInt::INT_FLAG_NONE;
  return noAlarm || alarmOk;
}


bool SHT35::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_temperature_write_degc_to_frame(         pvFrame, this->_temperature, CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE) &&
         cnssrf_dt_humidity_write_air_relpercents_to_frame( pvFrame, this->_humidity);
}



const char **SHT35::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t SHT35::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.1f%c%.1f",
		 this->_temperature, OUTPUT_DATA_CSV_SEP,
		 this->_humidity);

  return len >= size ? -1 : (int32_t)len;
}



/**
 * Introduce a delay between commands.
 *
 * @return true  always.
 */
bool SHT35::interFrameDelay()
{
  uint32_t i;

  for(i = 0; i < 500; i++);  // Delay
  return true;
}

/**
 * Read the  device's serial number.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::readSerialNumber()
{
  bool     res;
  uint16_t serialNumWords[2];

  // write "read serial number" command
  if((res = writeCommand(CMD_READ_SERIALNBR) && read2NBytesAndCRC(serialNumWords, 2)))
  {
    this->_serialNumber = ((uint32_t)serialNumWords[0] << 16) | serialNumWords[1];
  }

  return res;
}

/**
 * Read the sensor's status register.
 *
 * @param[out] pvStatus where the status is written to. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::readStatus(Status *pvStatus)
{
  return writeCommand(CMD_READ_STATUS) && read2NBytesAndCRC(&pvStatus->u16, 1);
}

/**
 * Start periodic measurements.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::startPeriodicMeasurement()
{

  return writeCommand(_FREQUENCY_REPEATABILITY_TO_COMMAND[frequency * (repeatability + 1)]);
}

/**
 * Read last measurement from the sensor buffer.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::readPeriodic()
{
  bool     res;
  uint16_t data[2];

  // if no error, read measurements
  if((res = writeCommand(CMD_FETCH_DATA) && read2NBytesAndCRC(data, 2)))
  {
    this->_temperature = calcTemperature(data[0]);
    this->_humidity    = calcHumidity(   data[1]);
  }

  return res;
}

/**
 * Read humidity and temperature from sensor.
 */
bool SHT35::readPolling()
{
  bool res;

  switch(mode)
  {
    case MODE_CLKSTRETCH: res = getTempAndHumiClkStretch(); break;
    case MODE_POLLING:    res = getTempAndHumiPolling();    break;
    default:              res = false;                      break;
  }

  return res;
}

/**
 * Gets the temperature [°C] and the relative humidity [%RH] from the sensor.
 *
 * This function uses the i2c clock stretching for waiting until measurement is
 * ready.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::getTempAndHumiClkStretch()
{
  Command  cmd;
  uint16_t data[2];

  // start measurement in clock stretching mode
  // use depending on the required repeatability, the corresponding command
  switch(repeatability)
  {
    case REPEATAB_LOW:    cmd = CMD_MEAS_CLOCKSTR_L; break;
    case REPEATAB_MEDIUM: cmd = CMD_MEAS_CLOCKSTR_M; break;
    case REPEATAB_HIGH:   cmd = CMD_MEAS_CLOCKSTR_H; break;
    default: goto error_exit;
  }
  if(!writeCommand(cmd)) { goto error_exit; }


    if(read2NBytesAndCRC(data, 2))
    {
      this->_temperature = calcTemperature(data[0]);
      this->_humidity    = calcHumidity(   data[1]);
      return true;
    }

  error_exit:
  return false;
}

/**
 * Gets the temperature [°C] and the relative humidity [%RH] from the sensor.
 *
 * This function polls every 1ms until measurement is ready.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::getTempAndHumiPolling()
{
  Command  cmd;
  uint8_t  to;
  uint16_t data[2];

  // start measurement in polling mode
  // use depending on the required repeatability, the corresponding command
  switch(repeatability)
  {
    case REPEATAB_LOW:    cmd = CMD_MEAS_POLLING_L; break;
    case REPEATAB_MEDIUM: cmd = CMD_MEAS_POLLING_M; break;
    case REPEATAB_HIGH:   cmd = CMD_MEAS_POLLING_H; break;
    default: goto error_exit;
  }
  if(!writeCommand(cmd)) { goto error_exit; }

  // Poll every 1ms for measurement ready until timeout
  for(to = SHT35_TIMEOUT; to--; )
  {
    if(read2NBytesAndCRC(data, 2))
    {
      this->_temperature = calcTemperature(data[0]);
      this->_humidity    = calcHumidity(   data[1]);
      return true;
    }

    HAL_Delay(1);
  }

  error_exit:
  return false;
}

/**
 * Set the alert limit values.
 *
 * @param[in] pvThs the threshold values to set. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::setAlertLimits(const AlertLimits *pvThs)
{
  return writeAlertLimitData(CMD_W_AL_LIM_HS, pvThs->humidityHighSet,   pvThs->temperatureHighSet)   &&
      interFrameDelay() &&
      writeAlertLimitData(   CMD_W_AL_LIM_HC, pvThs->humidityHighClear, pvThs->temperatureHighClear) &&
      interFrameDelay() &&
      writeAlertLimitData(   CMD_W_AL_LIM_LC, pvThs->humidityLowClear,  pvThs->temperatureLowClear)  &&
      interFrameDelay() &&
      writeAlertLimitData(   CMD_W_AL_LIM_LS, pvThs->humidityLowSet,    pvThs->temperatureLowSet);
}

/**
 * Get the currently set alarm limits from the sensor.
 *
 * @param[out] pvThs where the thershold values are written to. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::getAlertLimits(AlertLimits *pvThs)
{
  return readAlertLimitData(CMD_R_AL_LIM_HS, &pvThs->humidityHighSet,   &pvThs->temperatureHighSet)   &&
      readAlertLimitData(   CMD_R_AL_LIM_HC, &pvThs->humidityHighClear, &pvThs->temperatureHighClear) &&
      readAlertLimitData(   CMD_R_AL_LIM_LC, &pvThs->humidityLowClear,  &pvThs->temperatureLowClear)  &&
      readAlertLimitData(   CMD_R_AL_LIM_LS, &pvThs->humidityLowSet,    &pvThs->temperatureLowSet);
}

/**
 * Clear all alert flags in status register from sensor.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::clearAllAlertFlags(void)
{
  return writeCommand(CMD_CLEAR_STATUS);
}

/**
 * Enable the sensor's heater.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::enableHeater(void)
{
  return writeCommand(CMD_HEATER_ENABLE);
}

/**
 * Disable the sensor's heater.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::disableHeater(void)
{
  return writeCommand(CMD_HEATER_DISABLE);
}

/**
 * Call the soft reset mechanism that forces the sensor into a well-defined
 * state without removing the power supply.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::softReset(void)
{
  bool res;

  if((res = writeCommand(CMD_SOFT_RESET))) { HAL_Delay(50); } // Wait 50 ms after reset.
  return res;
}

/**
 * Compute the CRC and return it.
 *
 * @param[in] pu8Data the data. MUST be NOT NULL.
 * @param[in] nBytes  the number of data bytes.
 *
 * @return the CRC.
 */
uint8_t SHT35::calcCRC(uint8_t *pu8Data, uint8_t nBytes)
{
  uint8_t bit;        // bit mask
  uint8_t byteCtr;    // byte counter
  uint8_t crc = 0xFF; // calculated checksum

  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nBytes; byteCtr++)
  {
    crc ^= (pu8Data[byteCtr]);
    for(bit = 8; bit > 0; --bit)
    {
      if(crc & 0x80) crc = (crc << 1) ^ 0x31;
      else crc = (crc << 1);
    }
  }

  return crc;
}


/**
 * Send an SHT35 command on the I2C bus.
 *
 * @param[in] command the command to send.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::writeCommand(Command command)
{
  return i2cSend2Bytes(command);
}

/**
 * Read data from the device, get the cRC and check the CRC?
 *
 * @param[out] pu16Data where the data are written to. MUST be NOT NULL.
 * @param[in]  nb       the number of 16 bits word to read.
 */
bool SHT35::read2NBytesAndCRC(uint16_t *pu16Data, uint8_t nb)
{
  uint8_t i;
  uint8_t nbBytesToRead = nb * 2 + nb;  // One CRC byte for 2 data bytes
  uint8_t data[9];                      // No command should generate more than 9 data bytes + CRCs.

  // Get data
  if(!i2cReceive(data, nbBytesToRead)) { goto error_exit; }

  // Check CRC(s)
  for(i = 0; i < nbBytesToRead; i += 3, pu16Data++)
  {
    if(data[i + 2] != calcCRC(data + i, 2)) { goto error_exit; }
    *pu16Data = (data[i] << 8) | data[i + 1];
  }

  return true;

  error_exit:
  return false;
}

/**
 * Send a 16 bits value to the sensor.
 *
 * @param[in] command the command associated with the value.
 * @param[in] value   the value.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::write2BytesAndCRC(Command command, uint16_t value)
{
  uint8_t data[5];

  data[0] = command >> 8; data[1] = command & 0xFF;
  data[2] = value   >> 8; data[3] = value   & 0xFF;
  data[4] = calcCRC(&data[2], 2);

  return i2cSend(data, sizeof(data));
}

/**
 * Write a couple of alert values.
 *
 * @param[in] command     the command associated with the values.
 * @param[in] humidity    the humidity threshold value, in %.
 * @param[in] temperature the temperature threshold value, in °C.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::writeAlertLimitData(Command command, float humidity, float temperature)
{
  uint16_t data;

  if(humidity >= 0.0f && humidity <= 100.0f && temperature >= -45.0f && temperature <= 130.0f)
  {
    data = (calcRawHumidity(humidity) & 0xFE00) | ((calcRawTemperature(temperature) >> 7) & 0x001FF);
    return write2BytesAndCRC(command, data);
  }

  return false;
}

/**
 * Read a couple of alert values
 *
 * @param[in]  command       the command to get the values.
 * @param[out] pfHumidity    where the humidity value, in %, is written to. MUSt be NOT NULL.
 * @param[out] pvTemperature where the temperature value, in °C, is written to. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SHT35::readAlertLimitData(Command command, float *pfHumidity, float *pfTemperature)
{
  bool     res;
  uint16_t data;

  if((res = writeCommand(command) && read2NBytesAndCRC(&data, 1)))
  {
    *pfHumidity    = calcHumidity(   data & 0xFE00);
    *pfTemperature = calcTemperature(data << 7);
  }

  return res;
}

/**
 * Convert raw temperature to °C
 *
 * @param[in] rawValue the raw value.
 *
 * @return the temperature, in °C.
 */
float SHT35::calcTemperature(uint16_t rawValue)
{
  return 175.0f * (float)rawValue / 65535.0f - 45.0f;
}

/**
 * Convert raw temperature to °%RH
 *
 * @param[in] rawValue the raw value.
 *
 * @return the humidity, in %RH.
 */
float SHT35::calcHumidity(uint16_t rawValue)
{
  return 100.0f * (float)rawValue / 65535.0f;
}

/**
 * Convert temperature in °C to a raw value
 *
 * @param[in] temperature the temperature, in °C.
 *
 * @return the raw value.
 */
uint16_t SHT35::calcRawTemperature(float temperature)
{
  return (temperature + 45.0f) / 175.0f * 65535.0f;
}

/**
 * Convert humidity in %RH to a raw value
 *
 * @param[in] humidity the humidity, in %RH.
 *
 * @return the raw value.
 */
uint16_t SHT35::calcRawHumidity(float humidity)
{
  return humidity / 100.0f * 65535.0f;
}
