/*
 * max31865.cpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */



//max31865.cpp
#include "max31865.hpp"
#ifdef USE_SENSOR_MAX31865
#include "logger.h"

CREATE_LOGGER(  max31865);
#undef  _logger
#define _logger max31865

#define SPI_SCK_FREQ_HZ  500000   // 500kHz

// Is the name used in the node's configuration file.
const char *MAX31865::TYPE_NAME = "MAX31865";


MAX31865::MAX31865()
: RTDSensor(POWER_NONE, POWER_NONE, FEATURE_BASE),
  _spi(SPI_SCK_FREQ_HZ,
       SPI::OPTION_MASTER   | SPI::OPTION_USE_MOSI        |
       SPI::OPTION_USE_MISO | SPI::OPTION_FULL_DUPLEX     |
       SPI::OPTION_MODE_1   | SPI::OPTION_SHIFT_MSB_FIRST |
       SPI::OPTION_DATA_SIZE_8BITS,
       SPI::DEFAULT_SS_INFOS) {  // Use default SPI chip select .
  this->_regConfig =
    SPI_REG_CONFIG_VBIAS_ON        | SPI_REG_CONFIG_CONV_NORMALLY_OFF |
    SPI_REG_CONFIG_FILTER_50_HZ    | SPI_REG_CONFIG_2_4_WIRES         |
    SPI_REG_CONFIG_FAULT_NO_ACTION |
    SPI_REG_CONFIG_FAULT_STATUS_CLEAR;
}
MAX31865::~MAX31865() { /* Do nothing */ }

Sensor     *MAX31865::getNewInstance() { return new MAX31865(); }
const char *MAX31865::type()           { return TYPE_NAME;      }


bool MAX31865::jsonSpecificHandler(const JsonObject& json) {
  int32_t i32;
  bool    res;

  if(!(res = this->_spi.initUsingJSON(json))) {
    log_error_sensor(_logger,
		     "Failed to initialise SPI interface using "
		     "configuration.");
  }

  // Make parent class get it's configuration parameters.
  // It includes the number of wires.
  res &= RTDSensor::jsonSpecificHandler(json);
  switch(nbWires())
  {
    case NBWIRES_2:
    case NBWIRES_4:
      this->_regConfig &= ~SPI_REG_CONFIG_3_WIRES; break;
      break;
    case NBWIRES_3:
      this->_regConfig |=  SPI_REG_CONFIG_3_WIRES; break;
      break;
    default:
      log_error_sensor(_logger, "Invalid number of wires.");
      res = false; break;
  }

  // Get filter frequency
  if(json[     "filterFreqHz"].success()) {
    i32 = json["filterFreqHz"].as<int32_t>();
    switch(i32) {
      case 50: this->_regConfig |=  SPI_REG_CONFIG_FILTER_50_HZ; break;
      case 60: this->_regConfig &= ~SPI_REG_CONFIG_FILTER_50_HZ; break;
      default:
	log_error_sensor(_logger,
			 "'%li' is not a valid value for "
			 "configuration parameter 'filterFreqHz'.",
			 i32);
	res = false; break;
    }
  }

  return res;
}


bool MAX31865::readReg8(SPIReg reg, uint8_t *pu8) {
  uint8_t u8; bool res;

  u8  = reg & 0x7F;  // Make sure to clear an eventual write bit.
  this->_spi.selectSlave();
  res = this->_spi.write(&u8, 1, 100) && this->_spi.read(pu8, 1, 100);
  this->_spi.selectSlave(SPI::SS_ID_NONE);
  return res;
}

bool MAX31865::writeReg8(SPIReg reg, uint8_t u8) {
  uint8_t data[2]; bool res;

  data[0] = reg | 0x80;  // Add write bit.
  data[1] = u8;

  this->_spi.selectSlave();
  res = this->_spi.write(data, 2, 100);
  this->_spi.selectSlave(SPI::SS_ID_NONE);
  return res;
}


bool MAX31865::readReg16(SPIReg firstReg, uint16_t *pu16) {
  uint8_t u8, buffer[2];
  bool    res;

  u8  = firstReg & 0x7F;  // Make sure to clear an eventual write bit.
  this->_spi.selectSlave();
  res = this->_spi.write(&u8,    1, 100) &&
        this->_spi.read( buffer, 2, 100);
  this->_spi.selectSlave(SPI::SS_ID_NONE);

  if(res) { *pu16  = (((uint16_t)buffer[0]) << 8) | buffer[1]; }

  return res;
}


bool MAX31865::openSpecific() {
  return this->_spi.open();
}

void MAX31865::closeSpecific() {
  this->_spi.close();
}


bool MAX31865::readSpecific() {
  uint8_t  u8, tryNum;
  uint16_t u16;
  float    rtdOhms;

  // Turn on VBias and clear faults
  u8 = this->_regConfig;
  if(!writeReg8(SPI_REG_CONFIG, u8)) { goto error_comm_exit; }

  // Read back the register value to check that the SPI com is working.
  u8 = 0;
  if(!readReg8(SPI_REG_CONFIG, &u8) ||
      (u8               & ~SPI_REG_CONFIG_FAULT_STATUS_CLEAR) !=
	  (this->_regConfig & ~SPI_REG_CONFIG_FAULT_STATUS_CLEAR))
  { goto error_comm_exit; }
  // Give time for bias to settle.
  // Delay depends on the Rref/Cfilter time constant.
  // To avoid asking the user about the RTDIN filter capacitor,
  // use a generous delay.
  board_delay_ms(10);  //  This value should be high enough

  // Start a single shot ADC conversion.
  u8 |= SPI_REG_CONFIG_1SHOT;
  if(!writeReg8(SPI_REG_CONFIG, u8)) { goto error_comm_exit; }
  // Wait for the ADC to finish.
  // Takes about 52 ms with 60 Hz filter and 62.5 ms with 50 Hz filter.
  // Use the highest value plus some extra time.
  board_delay_ms(70);
  // Check that the ADC is done
  for(tryNum = 3; tryNum; tryNum--) {
    if(!readReg8(SPI_REG_CONFIG, &u8)) { goto error_comm_exit; }
    if(!(u8 & SPI_REG_CONFIG_1SHOT)) break;  // ADC is done.
    board_delay_ms(10);
  }
  if(!tryNum) {
    log_error_sensor(_logger, "ADC conversion failed.");
    goto error_exit;
  }
  // Get the ADC value
  if(!readReg16(SPI_REG_RTD_MSB, &u16)) { goto error_comm_exit; }
  // Check if fault
  if(u16 & 0x1) {
    // Indicate that there is a fault.
    goto error_exit;
  }
  // Compute RTD resistance
  u16   >>= 1; // Remove fault bit.
  rtdOhms = (((float)u16) / 32768.0) * refOhms();
  log_debug_sensor(_logger, "RTD resistance is: %f Ohms.", rtdOhms);
  // Convert resistance to temperature.
  if(!setRTDResistance(rtdOhms))  // Function is from parent class.
  {
    log_error_sensor(_logger, "Failed to convert RTD resistance of "
		     "%f Ohms to temperature.", rtdOhms);
    goto error_exit;
  }

  return true;

  error_comm_exit:
  log_error_sensor(_logger, "Failed to communicate with chip.");

  error_exit:
  // Try to turn OFF VBIAS to reduce power consumption.
  writeReg8(SPI_REG_CONFIG,
	    this->_regConfig & ~SPI_REG_CONFIG_VBIAS_ON);
  return false;
}
#endif // USE_SENSOR_MAX31865

