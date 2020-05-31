/*
 * Base class for sensors using I2C communication
 *
 *  Created on: 5 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "sensor_i2c.hpp"
#include "utils.h"

#define ADDRESS_MAX_7BITS   0x7F
#define ADDRESS_MAX_10BITS  0x3FF


  CREATE_LOGGER(sensor_i2c);
#undef  logger
#define logger  sensor_i2c


/**
 * Constructor.
 *
 * @param[in] power       the power configuration required when the node is active.
 * @param[in] powerSleep  the power configuration required when the node is asleep.
 * @param[in] interface   the I2C interface to use.
 * @param[in] baseAddress the sensor's base address. If there is none then set it to 0.
 * @param[in] endianness  the endianness used by the I2C slave address.
 * @param[in] options     the I2C options to use.
 * @param[in] features    the sensor's features.
 */
SensorI2C::SensorI2C(Power      power,
		     Power      powerSleep,
		     Interface  interface,
		     I2CAddress baseAddress,
		     Endianness endianness,
		     Options    options,
		     Features   features) :
Sensor(power, powerSleep, features)
{
  this->_interface  = interface;
  this->_address    = baseAddress;
  this->_endianness = endianness;
  this->_options    = options;
}

/**
 * Destructor.
 */
SensorI2C::~SensorI2C()
{
  // Do nothing
}


/**
 * Set the I2C slave device's address.
 *
 * @param[in] address the address to use.
 *
 * @return true  on success.
 * @return false if the sensor's address cannot be changed.
 */
bool SensorI2C::setAddress(I2CAddress address)
{
  if(this->_options & OPTION_FIXED_ADDR) { return false; }

  this->_address = address;
  return true;
}


/**
 * Open the sensor's I2C specific part.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::openSpecific()
{
  GPIO_InitTypeDef init;

  // Configure GPIOs
  init.Mode  = GPIO_MODE_AF_OD;
  init.Pull  = GPIO_NOPULL;
  init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  // Do the configuration specific to an interface
  switch(this->_interface)
  {
    case INTERFACE_INTERNAL:
      // Reset I2C module and enable clock
      PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _FORCE_RESET)();
      PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _RELEASE_RESET)();
      PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _CLK_ENABLE)();

      // Configure GPIO
      gpio_use_gpios_with_ids(I2C_INTERNAL_SDA_GPIO, I2C_INTERNAL_SCL_GPIO);
      init.Alternate = I2C_INTERNAL_SDA_AF;
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_INTERNAL_SDA_GPIO, &init.Pin), &init);
      init.Alternate = I2C_INTERNAL_SCL_AF;
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_INTERNAL_SCL_GPIO, &init.Pin), &init);

      // Configure I2C module stuff
      HAL_I2CEx_ConfigAnalogFilter( &this->_hi2c, I2C_INTERNAL_FILTER_ANALOG);
      HAL_I2CEx_ConfigDigitalFilter(&this->_hi2c, I2C_INTERNAL_FILTER_DIGITAL);
      this->_hi2c.Instance    = PASTER2(I2C, I2C_INTERNAL_ID);
      this->_hi2c.Init.Timing = I2C_INTERNAL_TIMING;
      break;

    case INTERFACE_EXTERNAL:
      // Reset I2C module and enable clock
      PASTER3(__HAL_RCC_I2C, I2C_EXTERNAL_ID, _FORCE_RESET)();
      PASTER3(__HAL_RCC_I2C, I2C_EXTERNAL_ID, _RELEASE_RESET)();
      PASTER3(__HAL_RCC_I2C, I2C_EXTERNAL_ID, _CLK_ENABLE)();

      // Configure GPIO
      gpio_use_gpios_with_ids(I2C_EXTERNAL_SDA_GPIO, I2C_EXTERNAL_SCL_GPIO);
      init.Alternate = I2C_EXTERNAL_SDA_AF;
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_EXTERNAL_SDA_GPIO, &init.Pin), &init);
      init.Alternate = I2C_EXTERNAL_SCL_AF;
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_EXTERNAL_SCL_GPIO, &init.Pin), &init);

      // Configure I2C module stuff
      HAL_I2CEx_ConfigAnalogFilter( &this->_hi2c,  I2C_EXTERNAL_FILTER_ANALOG);
      HAL_I2CEx_ConfigDigitalFilter(&this->_hi2c,  I2C_EXTERNAL_FILTER_DIGITAL);
      this->_hi2c.Instance    = PASTER2(I2C, I2C_EXTERNAL_ID);
      this->_hi2c.Init.Timing = I2C_EXTERNAL_TIMING;
      break;

    default:
      log_error_sensor(logger, "Unknown I2C interface '%d'.", this->_interface);
      goto error_exit;
  }


  // Init I2C module
  this->_hi2c.Init.OwnAddress1      = 0;
  this->_hi2c.Init.AddressingMode   = EXT_I2C_BUS_NBIT_ADDRESS;
  this->_hi2c.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
  this->_hi2c.Init.OwnAddress2      = 0;
  this->_hi2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  this->_hi2c.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
  this->_hi2c.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
  if(HAL_I2C_Init(&this->_hi2c)    != HAL_OK) { goto error_exit; }

  return true;

  error_exit:
  return false;
}

/**
 * Close the sensor's I2C specific part.
 */
void SensorI2C::closeSpecific()
{
  HAL_I2C_DeInit(&this->_hi2c);
  switch(this->_interface)
  {
    case INTERFACE_INTERNAL:
      PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _CLK_DISABLE)();
      gpio_free_gpios_with_ids(I2C_INTERNAL_SDA_GPIO, I2C_INTERNAL_SCL_GPIO);
      break;

    case INTERFACE_EXTERNAL:
      PASTER3(__HAL_RCC_I2C, I2C_EXTERNAL_ID, _CLK_DISABLE)();
      gpio_free_gpios_with_ids(I2C_EXTERNAL_SDA_GPIO, I2C_EXTERNAL_SCL_GPIO);
      break;

    default:
      ; // Do nothing
  }
}


/**
 * Parse the configuration for I2C sensor general configuration parameters.
 */
bool SensorI2C::jsonSpecificHandler(const JsonObject& json)
{
  I2CAddress addr;
  bool       ok   = true;

  // Get the I2C address.
  if(!(this->_options & OPTION_FIXED_ADDR))
  {
    const JsonVariant &value = json["address"];
    if(value.success())
    {
      if(     value.is<int>()) { addr = value.as<I2CAddress>(); }
      else if(value.is<const char *>())
      {
	addr = inthexbinarystr_to_uint32(value.as<const char *>(), ADDRESS_MAX_10BITS + 1, NULL);
      }
      if(addr > ADDRESS_MAX_10BITS)
      {
	log_error_sensor(logger, "Invalid I2C address set in configuration.");
	ok = false;
      }
      else
      {
	if(!setAddress(addr))
	{
	  log_error_sensor(logger, "Failed to set I2C address to '%#05x'.", addr);
	  ok = false;
	}
      }
    }
    else
    {
      log_error_sensor(logger, "No I2C address is set in configuration.");
      ok = false;
    }
  }

  return ok;
}


/**
 * Writes and read from an I2C device.
 *
 * @param[in]  pu8TxBuffer    the data to send to the device. Can be NULL if nothing to send.
 * @param[in]  txSize         the number of data to send.
 * @param[out] pu8RxBuffer    where the data read from the device are written to.
 *                            Can be NULL if we do not want to read data.
 * @param[in]  nbRx           the number of data bytes to read from the I2C device.
 * @param[in]  waitBeforeRxMs time to wait, in milliseconds, between sending the data the data and reading the data.
 */
bool SensorI2C::i2cTransfert(const uint8_t *pu8TxBuffer,    uint32_t txSize,
			     uint8_t       *pu8RxBuffer,    uint32_t nbRx,
			     uint32_t       waitBeforeRxMs, uint32_t timeoutMs)
{
  // Send data
  if(pu8TxBuffer && txSize)
  {
    HAL_StatusTypeDef status;
    if((status = HAL_I2C_Master_Transmit(&this->_hi2c, address(), (uint8_t *)pu8TxBuffer, txSize, timeoutMs)) != HAL_OK)
    {
      goto error_exit;
    }
  }

  // Receive data
  if(pu8RxBuffer && nbRx)
  {
    if(waitBeforeRxMs) { HAL_Delay(waitBeforeRxMs); }
    if(HAL_I2C_Master_Receive(&this->_hi2c, address(), pu8RxBuffer, nbRx, timeoutMs) != HAL_OK)
    {
      goto error_exit;
    }
  }

  return true;

  error_exit:
  return false;
}


/**
 * Write a byte to a register from the I2C device.
 *
 * @param[in] reg       the register's address.
 * @param[in] value     the value to write.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cWriteByteToRegister(uint8_t reg, uint8_t value, uint32_t timeoutMs)
{
  return HAL_I2C_Mem_Write(&this->_hi2c, address(), reg, 1, &value, 1, timeoutMs) == HAL_OK;
}

/**
 * Read a byte from a register from the I2C device.
 *
 * @param[in]  reg       the register's address.
 * @param[out] pu8Value  where the register's value is written to. MUST be NOT NULL.
 * @param[in]  timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cReadByteFromRegister( uint8_t reg, uint8_t *pu8Value, uint32_t timeoutMs)
{
  return HAL_I2C_Mem_Read(&this->_hi2c, address(), reg, 1, pu8Value, 1, timeoutMs) == HAL_OK;
}

/**
 * Write a two bytes value to the I2C device.
 *
 * @param[in] reg       the device's registers address.
 * @param[in] value     the value to write.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cWrite2BytesToRegister(uint8_t  reg,
					 uint16_t value,
					 uint32_t timeoutMs)
{
  value = (this->_endianness == LITTLE_ENDIAN) ? TO_LITTLE_ENDIAN_16(value) : TO_BIG_ENDIAN_16(value);

  return HAL_I2C_Mem_Write(&this->_hi2c, address(),
			   reg, 1, (uint8_t *)&value, 2,
			   timeoutMs) == HAL_OK;
}

/**
 * Read a two bytes value from the I2C device.
 *
 * @param[in] reg       the device's registers address.
 * @param[in] pu16Value Where to write the value read from the register. MUST be NOT NULL.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cRead2BytesFromRegister(uint8_t   reg,
					  uint16_t *pu16Value,
					  uint32_t  timeoutMs)
{
  bool res;

  if((res = (HAL_I2C_Mem_Read(&this->_hi2c, address(),
			      reg, 1, (uint8_t *)pu16Value, 2,
			      timeoutMs) == HAL_OK)))
  {
    *pu16Value = (this->_endianness == LITTLE_ENDIAN) ?
	FROM_LITTLE_ENDIAN_16(*pu16Value) : FROM_BIG_ENDIAN_16(*pu16Value);
  }

  return res;
}

/**
 * Write a 3 bytes value to the I2C device.
 *
 * @param[in] reg       the device's registers address.
 * @param[in] value     the value to write.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cWrite3BytesToRegister(uint8_t  reg,
					 uint32_t value,
					 uint32_t timeoutMs)
{
  value = (this->_endianness == LITTLE_ENDIAN) ? TO_LITTLE_ENDIAN_24(value) : TO_BIG_ENDIAN_24(value);

  return HAL_I2C_Mem_Write(&this->_hi2c, address(),
			   reg, 1, (uint8_t *)&value, 3,
			   timeoutMs) == HAL_OK;
}

/**
 * Read a 3 bytes value from the I2C device.
 *
 * @param[in] reg       the device's registers address.
 * @param[in] pu32Value Where to write the value read from the register. MUST be NOT NULL.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cRead3BytesFromRegister(uint8_t   reg,
					  uint32_t *pu32Value,
					  uint32_t  timeoutMs)
{
  bool res;

  if((res = (HAL_I2C_Mem_Read(&this->_hi2c, address(),
			      reg, 1, (uint8_t *)pu32Value, 3,
			      timeoutMs) == HAL_OK)))
  {
    *pu32Value = (this->_endianness == LITTLE_ENDIAN) ?
	FROM_LITTLE_ENDIAN_24(*pu32Value) : FROM_BIG_ENDIAN_24(*pu32Value);
  }

  return res;
}

/**
 * Send 2 bytes over I2C to the slave device.
 *
 * @param[in] value the value to send.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cSend2Bytes(uint16_t value, uint32_t timeoutMs)
{
  value = (this->_endianness == LITTLE_ENDIAN) ? TO_LITTLE_ENDIAN_16(value) : TO_BIG_ENDIAN_16(value);

    return HAL_I2C_Master_Transmit(&this->_hi2c, address(), (uint8_t *)&value, 2, timeoutMs) == HAL_OK;
}

/**
 * Get data bytes from the slave device.
 *
 * @param[out] pu8Data   where the dreceived data are written to. MUST be NOT NULL and big enough.
 * @param[in]  nbData    the number of data to get from the slave device.
 * @param[in]  timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cReceive(uint8_t *pu8Data, uint32_t nbData, uint32_t timeoutMs)
{
  return HAL_I2C_Master_Receive(&this->_hi2c, address(), pu8Data, nbData, timeoutMs) == HAL_OK;
}

/**
 * Send data to the slave device.
 *
 * @param[in] pu8Data   the data to send. MUST be NOT NULL.
 * @param[in] nbdata    the number of bytes to send.
 * @param[in] timeoutMs the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cSend(const uint8_t *pu8Data, uint32_t nbData, uint32_t timeoutMs)
{
  return HAL_I2C_Master_Transmit(&this->_hi2c, address(), (uint8_t *)pu8Data, nbData, timeoutMs) == HAL_OK;
}

/**
 * Memory write through I2C.
 *
 * @param[in] memAddress     the memory address to write to.
 * @param[in] memAddressSize size of the memory address.
 * @param[in] pu8Data        the data to write. MUST be NOT NULL.
 * @param[in] size           the number of data bytes to write.
 * @param[in] timeoutMs      the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cMemWrite(uint32_t       memAddress,
			    uint8_t        memAddressSize,
			    const uint8_t *pu8Data,
			    uint32_t       size,
			    uint32_t       timeoutMs)
{
  return HAL_I2C_Mem_Write(&this->_hi2c,       address(),
			   memAddress,         memAddressSize,
			   (uint8_t *)pu8Data, size,
			   timeoutMs) == HAL_OK;
}

/**
 * Memory read through I2C.
 *
 * @param[in]  memAddress     the memory address to read from.
 * @param[in]  memAddressSize size of the memory address.
 * @param[out] pu8Data        where the data read from the I2C slave are written to. MUST be NOT NULL.
 * @param[in]  nb             the number of data bytes to read.
 * @param[in]  timeoutMs      the timeout, in milliseconds.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorI2C::i2cMemRead(uint32_t memAddress,
			   uint8_t  memAddressSize,
			   uint8_t *pu8Data,
			   uint32_t nb,
			   uint32_t timeoutMs)
{
  return HAL_I2C_Mem_Read(&this->_hi2c, address(),
			  memAddress,   memAddressSize,
			  pu8Data,      nb,
			  timeoutMs) == HAL_OK;
}

