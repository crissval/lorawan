/*
 * Base class for sensors using I2C communication
 *
 *  Created on: 5 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_SENSOR_I2C_HPP_
#define SENSORS_SENSOR_I2C_HPP_

#include "sensor.hpp"


#define SENSOR_I2C_DEFAULT_TIMEOUT_MS  1000


class SensorI2C : public Sensor
{
protected:
  /**
   * Defines the I2C interface identifiers
   */
  typedef enum Interface
  {
    INTERFACE_INTERNAL,
    INTERFACE_EXTERNAL
  }
  Interface;

  typedef uint16_t I2CAddress; ///< The type for an I2C address.

  /**
   * Defines the I2C sensor configuration
   */
  typedef enum Option
  {
    OPTION_NONE       = 0x00,
    OPTION_FIXED_ADDR = 0x01   ///< The I2C address is fixed; it cannot be changed.
  }
  Option;
  typedef uint8_t Options;


public:
  I2CAddress address() { return this->_address; }


protected:
  SensorI2C(Power      power,
	    Power      powerSleep,
	    Interface  interface,
	    I2CAddress baseAddress,
	    Endianness endianness,
	    Options    options,
	    Features   features = FEATURE_BASE);
  virtual ~SensorI2C();

  virtual bool setAddress(I2CAddress address);

  bool i2cTransfert(             const uint8_t *pu8TxBuffer,       uint32_t txSize,
		                 uint8_t       *pu8RxBuffer,       uint32_t nbRx,
		                 uint32_t       waitBeforeRxMs,    uint32_t timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cWriteByteToRegister(   uint8_t   reg, uint8_t value,     uint32_t timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cReadByteFromRegister(  uint8_t   reg, uint8_t *pu8Value, uint32_t timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cWrite2BytesToRegister( uint8_t   reg,
				 uint16_t  value,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cRead2BytesFromRegister(uint8_t   reg,
				 uint16_t *pu16Value,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cWrite3BytesToRegister( uint8_t   reg,
				 uint32_t  value,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cRead3BytesFromRegister(uint8_t   reg,
				 uint32_t *pu32Value,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cSend2Bytes(            uint16_t  value,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cReceive(               uint8_t  *pu8Data,
				 uint32_t  nbData,
				 uint32_t  timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cSend(                  const uint8_t *pu8Data,
				 uint32_t       nbData,
				 uint32_t       timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cMemWrite(              uint32_t       memAddress,
				 uint8_t        memAddressSize,
				 const uint8_t *pu8Data,
				 uint32_t       size,
				 uint32_t       timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);
  bool i2cMemRead(               uint32_t       memAddress,
				 uint8_t        memAddressSize,
				 uint8_t       *pu8Data,
				 uint32_t       size,
				 uint32_t       timeoutMs = SENSOR_I2C_DEFAULT_TIMEOUT_MS);


protected:
  virtual bool jsonSpecificHandler(const JsonObject& json);


protected:
  virtual bool openSpecific();
  virtual void closeSpecific();


protected:
  I2C_HandleTypeDef _hi2c;


private:
  Interface         _interface;   ///< The I2C interface being used.
  I2CAddress        _address;     ///< The sensor's i2C address.
  Endianness        _endianness;  ///< The order of the data bytes in the device's registers.
  Options           _options;     ///< The I2C sensor's options.
};


#endif /* SENSORS_SENSOR_I2C_HPP_ */
