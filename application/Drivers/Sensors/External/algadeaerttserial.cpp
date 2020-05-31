/*
 * Driver for the Alagade AER TT radon sensor with serial interface.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include <string.h>
#include "algadeaerttserial.hpp"
#include "utils.h"
#include "connecsens.hpp"

#ifdef USE_SENSOR_ALGADE_AERTT_SERIAL

#define UART_MSG_FIELDS_SEP  ';'

#define UART_ONE_BYTE_TIMEOUT_MS 10
#define UART_MSG_TIMEOUT_MS      3000


#define MEASUREMENT_DURATION_MS  10000  // Actually, I don't known how long it takes; the documentation does not specify it.

#define UART_MSG_STATUS_BATT_LOW   0x01
#define UART_MSG_STATUS_HEATER_ON  0x02

#define STATE_VERSION  1


  CREATE_LOGGER(algadeaerttserial);
#undef  logger
#define logger  algadeaerttserial


const char *AlgadeAERTTSerial::TYPE_NAME = "AlgadeAERTTSerial";

const char *AlgadeAERTTSerial::_CSV_HEADER_VALUES[] =
{
    "radioactivityBqM3", "airTempratureDegC", "airHumidityRelPercent",
    "batteryV",          "heater",            NULL
};


AlgadeAERTTSerial::AlgadeAERTTSerial() :
    SensorUART(POWER_NONE, POWER_NONE,
	       9600,
	       USART_PARAM_WORD_LEN_8BITS |
	       USART_PARAM_PARITY_NONE    |
	       USART_PARAM_STOP_BITS_1    |
	       USART_PARAM_MODE_RX_TX,
	       FEATURE_BASE | FEATURE_CAN_FOLLOW_SENSOR_S_PACE)
{
  this->_hasDataToProcess = false;
  this->_dataBuffer[0]    = '\0';
  this->_nbDataBytes      = 0;

  clearAlarms();
  clearReadings();
}

AlgadeAERTTSerial::~AlgadeAERTTSerial()
{
  // Do nothing
}

Sensor *AlgadeAERTTSerial::getNewInstance()
{
  return new AlgadeAERTTSerial();
}

const char *AlgadeAERTTSerial::type()
{
  return TYPE_NAME;
}


bool AlgadeAERTTSerial::openSpecific()
{
  bool res;

  if((res = SensorUART::openSpecific()) && periodType() == PERIOD_TYPE_AT_SENSOR_S_FLOW)
  {
    // Start reading UART now.
    // We have <CR><LF> at the end of the message. So wait for <LF>, do not copy it.
    uartReadToContinuous(this->_workingBuffer, sizeof(this->_workingBuffer), '\n', false);
  }

  return res;
}


bool AlgadeAERTTSerial::jsonAlarmSpecific(const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts)
{
  bool res = true;

  // Get values from configuration
  this->_alarmLowSetBqm3    = json["radonLowSetBqm3"]   .as<uint32_t>();
  this->_alarmLowClearBqm3  = json["radonLowClearBqm3"] .as<uint32_t>();
  this->_alarmHighSetBqm3   = json["radonHighSetBqm3"]  .as<uint32_t>();
  this->_alarmHighClearBqm3 = json["radonHighClearBqm3"].as<uint32_t>();

  // Check values
  // Low alarm
  if(this->_alarmLowClearBqm3)
  {
    if(this->_alarmLowClearBqm3 < this->_alarmLowSetBqm3)
    {
      log_error_sensor(logger,
		       "Alarm configuration parameter 'radonLowSetBqm3' "
		       "MUST be lower than 'radonLowClearBqm3'.");
      res = false;
    }
  }
  else { this->_alarmLowClearBqm3 = this->_alarmLowSetBqm3; }

  // High alarm
  if(this->_alarmHighClearBqm3)
  {
    if(this->_alarmHighClearBqm3 > this->_alarmHighSetBqm3)
    {
      log_error_sensor(logger,
      		       "Alarm configuration parameter 'radonHighSetBqm3' "
		       "MUST be higher than 'radonHighClearBqm3'.");
      res = false;
    }
  }
  else { this->_alarmHighClearBqm3 = this->_alarmHighSetBqm3; }

  // Look at low and high alarm
  if( this->_alarmHighSetBqm3 && this->_alarmLowSetBqm3 &&
      this->_alarmHighSetBqm3 <= this->_alarmLowSetBqm3)
  {
    log_error_sensor(logger,
		     "Alarm configuration parameter 'radonHighSetBqm3' "
		     "MUST be strictly greater than 'radonLowSetBqm3'.");
    res = false;
  }

  if(res) { this->_alarmIsSet =  this->_alarmLowSetBqm3 || this->_alarmHighSetBqm3; }
  else    { clearAlarms(); }

  if(!this->_alarmHighSetBqm3) { this->_alarmHighSetBqm3 = this->_alarmHighClearBqm3 = UINT32_MAX; }

  return res;
}

void AlgadeAERTTSerial::clearAlarms()
{
  this->_alarmLowSetBqm3  = this->_alarmLowClearBqm3  = 0;
  this->_alarmHighSetBqm3 = this->_alarmHighClearBqm3 = UINT32_MAX;
  this->_alarmIsSet       = false;
}


Sensor::State *AlgadeAERTTSerial::defaultState(uint32_t *pu32_size)
{
  this->_state.version = STATE_VERSION;

  *pu32_size = sizeof(this->_state);
  return &this->_state;
}


void AlgadeAERTTSerial::clearReadings()
{
  this->_radioactivityBqm3     = 0;
  this->_airTemperatureDegC    = 0.0;
  this->_airRelHumidityPercent = 0;
  this->_voltageV              = 0;
  this->_battIsLow             = false;
  this->_heaterIsOn            = false;
}

bool AlgadeAERTTSerial::readSpecific()
{
  uint32_t tref;
  uint8_t  cmd;
  bool     res = false;

  clearReadings();

  // If reading is not periodic by at the sensor's pace then the read function is not used.
  if(periodType() == PERIOD_TYPE_AT_SENSOR_S_FLOW)
  {
    log_info_sensor(logger, "Sensor is configured to follow the sensor's pace; periodic reading is not possible.");
    goto exit;
  }

  // Ask the sensor to send last measurements
  // The documentation says that the sensor does not respond to commands while it
  // is performing a measurement, so we may have to send the command several times
  // if we don't get a response. The documentation does not say how long a measurement takes.
  tref = board_ms_now();
  while(!board_is_timeout(tref, MEASUREMENT_DURATION_MS))
  {
    // Set buffer to 0 so that we can detect end of data
    memset(this->_dataBuffer, 0, sizeof(this->_dataBuffer));

    // Send command and wait for first response byte
    cmd  = 'M';  // Actually, any byte sent to the sensor will trigger a message.
    while(1)
    {
      if(!uartWrite(&cmd, 1, UART_ONE_BYTE_TIMEOUT_MS))
      {
	log_error_sensor(logger, "Failed to send command.");
	goto exit;
      }

      // Wait for response. It ends with <CR><LF>.
      if(uartReadTo(this->_dataBuffer, sizeof(this->_dataBuffer),
		    '\n',              false,
		    UART_MSG_TIMEOUT_MS))
      { break; }
      if(board_is_timeout(tref, MEASUREMENT_DURATION_MS)) { goto exit; }
    };

    this->_hasDataToProcess = false;  // To avoid that the data will be processed again.
    if((res = parseDataFrame(this->_dataBuffer, sizeof(this->_dataBuffer)))) { break; }
  }
  if(!res) { log_error_sensor(logger, "Send command and wait for response timed out."); }

  exit:
  return res;
}

void AlgadeAERTTSerial::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
{
  UNUSED(pv_usart);
  //SensorUART::receivedData(pv_usart, pu8_data, size);  // Commented-out because it does nothing.

  size--;  // Do not count the <CR> that is left at the end of data frame, that was before the <LF>..
  // If data where written to the working buffer then copy it to the storage buffer.
  if(pu8_data == this->_workingBuffer)
  {
    memcpy(this->_dataBuffer, this->_workingBuffer, size);
  }

  this->_dataBuffer[size] = '\0';  // To make sure that the data can be read as a string.
  this->_nbDataBytes      = size;
  this->_hasDataToProcess = true;
  setInterruptionTimestamp();
}

bool AlgadeAERTTSerial::process()
{
  ts2000_t ts2000;
  bool     res = SensorUART::process();

  if(this->_hasDataToProcess)
  {
    // Save the interruption timestamp as soon as possible to avoid it to be overwritten.
    ts2000 = interruptionTimestamp();
    if(parseDataFrame(this->_dataBuffer, sizeof(this->_dataBuffer)))
    {
      setHasNewData();
      ConnecSenS::instance()->saveSensorData(this, ts2000);
      log_debug_sensor(logger,
		       "Received new data. Interruption timestamp = %lu, readings timestamp = %lu.",
		       ts2000, readingsTimestamp());
    }
    this->_hasDataToProcess = false;
  }

  return res;
}

/**
 * Parse a sensor's data frame and set up the instance's variables.
 *
 * @param[in] pu8_data The data received. Is NOT NULL.
 *                     MUST be NUL terminated, be a valid string.
 * @param[in] size     The number of bytes received.
 *
 * @return true  If the frame was valid and we could extract all the needed values.
 * @return false Otherwise.
 */
bool AlgadeAERTTSerial::parseDataFrame(const uint8_t *pu8_data, uint32_t size)
{
  const uint8_t *pu8;
  uint8_t        cnt;
  bool           res = false;

  log_info_sensor(logger, "Candidate response is: %s", pu8_data);

  // Try to see if the message looks valid using known facts
  // First there should always be the same number of fields separators
  for(pu8 = pu8_data, cnt = 0; *pu8; pu8++) { if(*pu8 == UART_MSG_FIELDS_SEP) cnt++; }
  if(cnt != ALGADEAERTTSERIAL_UART_MSG_SEP_COUNT)
  {
    log_debug_sensor(logger, "Frame data error: not enough data separator characters.");
    goto exit;
  }
  // Then check fixed characters
  if( this->_dataBuffer[2]  != ':' ||
      this->_dataBuffer[8]  != '/' ||
      this->_dataBuffer[11] != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: cannot find all the expected characters.");
    goto exit;
  }

  // Well, if we're here then I'm confident enough that we have a valid message.
  // Process the message.

  // Skip the date, I would have used it but it does not contain the year...
  for(pu8 = this->_dataBuffer; *pu8 != UART_MSG_FIELDS_SEP; pu8++) ;  // Look for first separator
  pu8++;  // And jump over it.

  // Get the radiation value
  this->_radioactivityBqm3 = strn_to_uint_with_default_and_sep((const char *)pu8,
							       size - (pu8 - pu8_data),
							       UART_MSG_FIELDS_SEP,
							       0,
							       (char **)&pu8);
  if(*pu8 != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: failed to read radiation value.");
    goto exit;
  }
  pu8++; // Skip separator

  // Get air temperature
  this->_airTemperatureDegC = strn_to_float_with_default_and_sep((const char *)pu8,
								 size - (pu8 - pu8_data),
								 UART_MSG_FIELDS_SEP,
								 0.0,
								 (char **)&pu8);
  if(*pu8 != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: failed to read temperature value.");
    goto exit;
  }
  pu8++; // Skip separator

  // Get air humidity
  this->_airRelHumidityPercent = strn_to_uint_with_default_and_sep((const char *)pu8,
								   size - (pu8 - pu8_data),
								   UART_MSG_FIELDS_SEP,
								   0,
								   (char **)&pu8);
  if(*pu8 != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: failed to read air humidity value.");
    goto exit;
  }
  pu8++; // Skip separator

  // Get status byte
  this->_battIsLow  = (*pu8 & UART_MSG_STATUS_BATT_LOW)  != 0;
  this->_heaterIsOn = (*pu8 & UART_MSG_STATUS_HEATER_ON) != 0;
  pu8++;
  if(*pu8 != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: failed to read status value.");
    goto exit;
  }
  pu8++;

  // Get the battery voltage
  this->_voltageV = strn_to_float_with_default_and_sep((const char *)pu8,
						       size - (pu8 - pu8_data),
						       UART_MSG_FIELDS_SEP,
						       0.0,
						       (char **)&pu8);
  if(*pu8 != 0 && *pu8 != UART_MSG_FIELDS_SEP)
  {
    log_debug_sensor(logger, "Frame data error: failed to read battery voltage value.");
    goto exit;
  }

  // We have all we need
  res = true;

  exit:
  if(res) { updateAlarmStatus(); }
  return res;
}

/**
 * Update he alarm status using the last readings.
 */
void AlgadeAERTTSerial::updateAlarmStatus()
{
  if(!this->_alarmIsSet) { return; }

  if(isInAlarm())
  {
    // Currently is in alarm
    if( this->_radioactivityBqm3 >= this->_alarmLowClearBqm3 &&
	this->_radioactivityBqm3 <= this->_alarmHighClearBqm3)
    {
      setIsInAlarm(false);
    }
  }
  else
  {
    // Not currently in alarm
    if( (this->_alarmLowSetBqm3  && this->_radioactivityBqm3 <= this->_alarmLowSetBqm3) ||
	(this->_alarmHighSetBqm3 && this->_radioactivityBqm3 >= this->_alarmHighSetBqm3))
    {
      setIsInAlarm(true);
    }
  }
}


bool AlgadeAERTTSerial::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  bool res = true;

  res &= cnssrf_dt_radioactivity_write_bqm3_to_frame(      pvFrame, this->_radioactivityBqm3);
  res &= cnssrf_dt_temperature_write_degc_to_frame(        pvFrame,
							   this->_airTemperatureDegC,
							   CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE);
  res &= cnssrf_dt_humidity_write_air_relpercents_to_frame(pvFrame, this->_airRelHumidityPercent);
  res &= cnssrf_dt_battvoltageflags_write_volts_to_frame(  pvFrame,
							   this->_battIsLow ?
							       CNSSRF_BATTVOLTAGE_FLAG_BATT_LOW :
							       CNSSRF_BATTVOLTAGE_FLAG_NONE,
							   this->_voltageV);
  res &= cnssrf_dt_heater_write_status_to_frame(           pvFrame, this->_heaterIsOn);

  return res;
}


const char **AlgadeAERTTSerial::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}


int32_t AlgadeAERTTSerial::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%d%c%.1f%c%d%c%.2f%c%s",
		 (int)this->_radioactivityBqm3, OUTPUT_DATA_CSV_SEP,
		 this->_airTemperatureDegC,     OUTPUT_DATA_CSV_SEP,
		 this->_airRelHumidityPercent,  OUTPUT_DATA_CSV_SEP,
		 this->_voltageV,               OUTPUT_DATA_CSV_SEP,
		 this->_heaterIsOn ? "ON" : "OFF");

  return len >= size ? -1 : (int32_t)len;
}


#endif // USE_SENSOR_ALGADE_AERTT_SERIAL
