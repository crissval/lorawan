/*
 * Reading of one of the node's digital inputs.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2019
 */
#include "digitalinput.hpp"
#ifdef USE_SENSOR_DIGITAL_INPUT
#include <string.h>
#include "gpio.h"
#include "board.h"
#include "cnssrf-dt_gpio.h"


  CREATE_LOGGER(digitalinput);
#undef  logger
#define logger  digitalinput


const char *DigitalInput::TYPE_NAME = "DigitalInput";

const char *DigitalInput::_CSV_HEADER_VALUES[] =
{
    "digitalInput-id",
    "digitalInput-state",
    NULL
};



DigitalInput::DigitalInput() : Sensor(POWER_NONE, POWER_NONE, FEATURE_BASE)
{
  this->_id        = 0;
  this->_state     = false;
  this->_pvExtPort = NULL;
}

DigitalInput::~DigitalInput()
{
  // Do nothing
}

Sensor *DigitalInput::getNewInstance()
{
  return new DigitalInput();
}

const char *DigitalInput::type()
{
  return TYPE_NAME;
}


bool DigitalInput::jsonSpecificHandler(const JsonObject& json)
{
  const char *ps;
  int32_t     i32;

  // Get the GPIO pin to use
  if(!(ps = json["useEXTPin"].as<const char *>()))
  {
    log_error_sensor(logger, "You must specify the configuration parameter 'useEXTPin'.");
    goto error_exit;
  }
  if(!(this->_pvExtPort = extport_get_pininfos_with_functions(ps, EXTPORT_PIN_FN_DIGITAL_INPUT)))
  {
    log_error_sensor(logger, "There is no GPIO called '%s' that can be used as a digital input.", ps);
    goto error_exit;
  }
  // Enable interruptions power supply
  if(this->_pvExtPort->functions & EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT)
  {
    addPower(POWER_EXTERNAL_INT);
  }

  // Get the identifier
  if(  !json["inputId"].success()) { this->_id = 0; }
  else
  {
    if(!json["inputId"].is<int32_t>() || (i32 = json["inputId"].as<int32_t>()) < 0 || i32 > 127)
    {
      log_error_sensor(logger, "The digital input identifier must be an integer in range [0..127].");
      goto error_exit;
    }
    this->_id = (uint8_t)i32;
  }

  return true;

  error_exit:
  return false;
}


bool DigitalInput::openSpecific()
{
  GPIO_InitTypeDef init;
  GPIO_TypeDef    *pvPort;
  uint32_t         pin;
  bool             ok = false;

  if(!_pvExtPort) { goto exit; }

  // Configure GPIO as input
  pvPort         = gpio_hal_port_and_pin_from_id(this->_pvExtPort->gpio, &pin);
  init.Alternate = 0;
  init.Mode      = GPIO_MODE_INPUT;
  init.Pin       = pin;
  init.Pull      = GPIO_NOPULL;
  init.Speed     = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(pvPort, &init);
  ok             = true;

  exit:
  return ok;
}

void DigitalInput::closeSpecific()
{
  GPIO_TypeDef *pvPort;
  uint32_t      pin;

  if(!_pvExtPort) { goto exit; }

  // Configure GPIO as input
  pvPort = gpio_hal_port_and_pin_from_id(this->_pvExtPort->gpio, &pin);
  HAL_GPIO_DeInit(pvPort, pin);

  exit:
  return;
}

bool DigitalInput::readSpecific()
{
  GPIO_TypeDef *pvPort;
  uint32_t      pin;
  bool          ok = false;

  if(!_pvExtPort) { goto exit; }

  // Read the value
  pvPort       = gpio_hal_port_and_pin_from_id(this->_pvExtPort->gpio, &pin);
  this->_state = (bool)HAL_GPIO_ReadPin(pvPort, pin);
  ok           = true;

  exit:
  return ok;
}


bool DigitalInput::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame  *pvFrame)
{
  return cnssrf_dt_gpio_write_state_single_with_id(pvFrame, this->_id, this->_state);
}


const char **DigitalInput::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t DigitalInput::csvDataSpecific(char *ps_data, uint32_t size)
{
  int32_t len;

  len = snprintf(ps_data, size,
		 "%u%c%u",
		 this->_id, OUTPUT_DATA_CSV_SEP, (uint8_t)this->_state);

  return (uint32_t)len >= size ? -1 : len;
}

#endif // USE_SENSOR_DIGITAL_INPUT
