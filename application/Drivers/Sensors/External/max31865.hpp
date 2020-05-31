/*
 * max31865.hpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#ifndef SENSORS_EXTERNAL_MAX31865_HPP_
#define SENSORS_EXTERNAL_MAX31865_HPP_

#include "config.h"
#ifdef USE_SENSOR_MAX31865
#include "rtdsensor.hpp"
#include "spi.hpp"


class MAX31865 : RTDSensor
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


private:
  typedef enum SPIReg  // Define the identifiers for the registers.
  {
    SPI_REG_CONFIG             = 0x00,  ///< Configuration.
    SPI_REG_RTD_MSB            = 0x01,  ///< RTD MSBs.
    SPI_REG_RTD_LSB            = 0x02,  ///< RTD LSBs.
    SPI_REG_HIGH_FAULT_THR_MSB = 0x03,  ///< High Fault Threshold_MSB.
    SPI_REG_HIGH_FAULT_THR_LSB = 0x04,  ///< High Fault Threshold_LSB.
    SPI_REG_LOW_FAULT_THR_MSB  = 0x05,  ///< Low Fault Threshold_MSB.
    SPI_REG_LOW_FAULT_THR_LSB  = 0x06,  ///< Low Fault Threshold_LSB.
    SPI_REG_FAULT_STATUS       = 0x07   ///< Fault Status.
  }
  SPIReg;

  typedef enum SPIRegConfigBits
  {
    SPI_REG_CONFIG_FILTER_50_HZ               = 1u  << 0,
    SPI_REG_CONFIG_FILTER_60_HZ               = 0,
    SPI_REG_CONFIG_FAULT_STATUS_CLEAR         = 1u  << 1,
    SPI_REG_CONFIG_FAULT_NO_ACTION            = 0,
    SPI_REG_CONFIG_FAULT_AUTOMATIC_DELAY      = 0x1 << 2,
    SPI_REG_CONFIG_FAULT_MANUAL_DELAY_CYCLE_1 = 0x2 << 2,
    SPI_REG_CONFIG_FAULT_MANUAL_DELAY_CYCLE_2 = 0x3 << 2,
    SPI_REG_CONFIG_3_WIRES                    = 1u  << 4,
    SPI_REG_CONFIG_2_4_WIRES                  = 0,
    SPI_REG_CONFIG_1SHOT                      = 1u  << 5,
    SPI_REG_CONFIG_CONV_MODE_AUTO             = 1u  << 6,
    SPI_REG_CONFIG_CONV_NORMALLY_OFF          = 0,
    SPI_REG_CONFIG_VBIAS_ON                   = 1u  << 7,
    SPI_REG_CONFIG_VBIAS_OFF                  = 0
  }
  SPIRegConfigBits;


public:
  MAX31865();
  ~MAX31865();
  static Sensor *getNewInstance();
  const char    *type();

  bool openSpecific();
  void closeSpecific();
  bool readSpecific();
  bool jsonSpecificHandler(const JsonObject& json);


private:
  bool readReg8( SPIReg reg,      uint8_t  *pu8);
  bool writeReg8(SPIReg reg,      uint8_t   u8);
  bool readReg16(SPIReg firstReg, uint16_t *pu16);


private:
  SPI     _spi;       ///< The SPI instance.
  uint8_t _regConfig; ///< The value of the SPI Config register to use.
};
#endif // USE_SENSOR_MAX31865





#endif /* SENSORS_EXTERNAL_MAX31865_HPP_ */
