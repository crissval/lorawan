/*
 * cnssit.cpp
 *
 *  Created on: 23 mai 2018
 *      Author: jfuchet
 */
#include <string.h>
#include "cnssint.hpp"
#include "board.h"
#include "cnssintclient.hpp"
#include "logger.h"
#include "rtc.h"


#ifndef CNSSINT_DEBOUNCE_TIME_MS_DEFAULT
#define CNSSINT_DEBOUNCE_TIME_MS_DEFAULT  0
#endif


#define CNSSINT_SPECIFIC_MASK  0x3FFFFFFF

#define CNSSINT_USB_IRQ_PRIORITY    15  // Lowest priority. To be sure that other IRQs can be served while in USB IRQ handler.
#define CNSSINT_WAKEUP_IRQ_PRIORITY 14


  CREATE_LOGGER(cnssint);
#undef  logger
#define logger  cnssint


const CNSSInt::Alias CNSSInt::_aliases[] =
{
    { "SDI_INT", "SDI12_INT" },
    { "OPTO_1",  "OPTO1_INT" },
    { "OPTO_2",  "OPTO2_INT" },
    { "INT_1",   "INT1_INT"  },
    { "INT_2",   "INT2_INT"  },
    { NULL, NULL }  // End of list
};

const CNSSInt::IntConfig CNSSInt::_intConfigs[INTERRUPTION_COUNT + 1] =
{
    { "LIS3DH_INT",  LIS3DH,  LIS3DH_FLAG,  true,  LIS3DH_INTERRUPT_GPIO  },
    { "LPS25_INT",   LPS25,   LPS25_FLAG,   true,  LPS25_INTERRUPT_GPIO   },
    { "OPT3001_INT", OPT3001, OPT3001_FLAG, true,  OPT3001_INTERRUPT_GPIO },
    { "SHT35_INT",   SHT35,   SHT35_FLAG,   true,  SHT35_INTERRUPT_GPIO   },
    { "SPI_INT",     SPI,     SPI_FLAG,     false, SPI_INTERRUPT_GPIO     },
    { "I2C_INT",     I2C,     I2C_FLAG,     false, I2C_INTERRUPT_GPIO     },
    { "USART_INT",   USART,   USART_FLAG,   false, USART_INTERRUPT_GPIO   },
    { "SDI12_INT",   SDI12,   SDI12_FLAG,   false, SDI12_INTERRUPT_GPIO   },
    { "OPTO1_INT",   OPTO1,   OPTO1_FLAG,   false, OPTO1_INTERRUPT_GPIO   },
    { "OPTO2_INT",   OPTO2,   OPTO2_FLAG,   false, OPTO2_INTERRUPT_GPIO   },
    { "INT1_INT",    INT1,    INT1_FLAG,    false, INT1_INTERRUPT_GPIO    },
    { "INT2_INT",    INT2,    INT2_FLAG,    false, INT2_INTERRUPT_GPIO    },
    { NULL, INTERRUPTION_COUNT, INT_FLAG_NONE, false, GPIO_NONE           }
};


CNSSInt *CNSSInt::_pvInstance = NULL;


/**
 * Constructor.
 */
CNSSInt::CNSSInt()
{
  this->_sensitiveToExternalInterruption = false;
  this->_sensitiveToInternalInterruption = false;

  // Initialise client registration list
  this->_nbClients = 0;
  for(uint16_t i = 0; i < CNSSIT_CLIENT_COUNT_MAX; i++)
  {
    Client *pvClient          = &this->_clients[i];
    pvClient->pvClient        = NULL;
    pvClient->sensitivityMask = INT_FLAG_NONE;
  }

  // Initialise interruption parameters
  for(uint8_t i = 0; i < INTERRUPTION_COUNT; i++)
  {
    IntParams *pv_ip          = &this->_intParams[i];
    pv_ip->flag               = (IntFlag)(1u << i);
    pv_ip->debounceMs         = CNSSINT_DEBOUNCE_TIME_MS_DEFAULT;
    pv_ip->lastActivationTsMs = 0;
  }

  // Initialise physical lines
  initInterruptLines();
}

/**
 * Return the unique instance of the class.
 *
 * @return the instance.
 */
CNSSInt *CNSSInt::instance()
{
  if(!_pvInstance) { _pvInstance = new CNSSInt(); }

  return _pvInstance;
}


void CNSSInt::initInterruptLines()
{
  GPIO_InitTypeDef init;

  gpio_use_gpios_with_ids(WAKEUP_EXT_SENSOR_GPIO,
			  WAKEUP_INT_SENSOR_GPIO,
			  WAKEUP_USB_GPIO);
//  gpio_enable_sclock_using_gpio_id(WAKEUP_EXT_SENSOR_GPIO);
//  gpio_enable_sclock_using_gpio_id(WAKEUP_INT_SENSOR_GPIO);
//  gpio_enable_sclock_using_gpio_id(WAKEUP_USB_GPIO);

  init.Alternate = 0;
  init.Speed     = GPIO_SPEED_FREQ_LOW;

  // Configure the external sensor interruption
  init.Pull      = WAKEUP_EXT_SENSOR_PULL;
  init.Mode      = GPIO_MODE_IT | WAKEUP_EXT_SENSOR_EDGE_SENSITIVITY;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(WAKEUP_EXT_SENSOR_GPIO, &init.Pin), &init);
  // Do not activate interruption yet (register handler), only do it when we have a client for external interruptions

  // Configure the internal sensor interruption
  init.Pull      = WAKEUP_INT_SENSOR_PULL;
  init.Mode      = GPIO_MODE_IT | WAKEUP_INT_SENSOR_EDGE_SENSITIVITY;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(WAKEUP_INT_SENSOR_GPIO, &init.Pin), &init);
  // Do not activate interruption yet (register handler), only do it when we have a client for internal interruptions

  // Configure USB interruption
  // We'll set up an IRQ handler for USB
  init.Pull      = WAKEUP_USB_PULL;
  init.Mode      = GPIO_MODE_IT | WAKEUP_USB_EDGE_SENSITIVITY;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(WAKEUP_USB_GPIO, &init.Pin), &init);
  gpio_set_irq_handler(WAKEUP_USB_GPIO, CNSSINT_USB_IRQ_PRIORITY, USBIrqHandler);

  // Set all the sensor interruption lines as inputs
  init.Mode      = GPIO_MODE_INPUT;
  for(const IntConfig *pvItCfg = _intConfigs; pvItCfg->psName ; pvItCfg++)
  {
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(pvItCfg->gpio, &init.Pin), &init);
  }

  // Set up the lines used to clear the interruption memories
  init.Mode   = GPIO_MODE_OUTPUT_OD;
  // For the external sensors
  // First set the pin level to the not clear level before setting up the pin as an output
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(SENSOR_EXT_CLEAR_INTERRUPT_GPIO, &init.Pin),
		    init.Pin,
		    (GPIO_PinState)!SENSOR_EXT_CLEAR_INTERRUPT_CLEAR_LEVEL);
  // Configure pin as output
  HAL_GPIO_Init(    gpio_hal_port_and_pin_from_id(SENSOR_EXT_CLEAR_INTERRUPT_GPIO, &init.Pin), &init);
  // For the internal sensors
  // First set the pin level to the not clear level before setting up the pin as an output
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(SENSOR_INT_CLEAR_INTERRUPT_GPIO, &init.Pin),
		    init.Pin,
		    (GPIO_PinState)!SENSOR_INT_CLEAR_INTERRUPT_CLEAR_LEVEL);
  // Configure pin as output
  HAL_GPIO_Init(    gpio_hal_port_and_pin_from_id(SENSOR_INT_CLEAR_INTERRUPT_GPIO, &init.Pin), &init);

  // Make sure the interruption flags are clear.
  // We do not use interruptions, but events, so these flags should not be set.
  /*__HAL_GPIO_EXTI_CLEAR_FLAG(WAKEUP_EXT_SENSOR_GPIO_PIN);
  __HAL_GPIO_EXTI_CLEAR_FLAG(WAKEUP_INT_SENSOR_GPIO_PIN);
  __HAL_GPIO_EXTI_CLEAR_FLAG(WAKEUP_USB_PIN);
  */
}


/**
 * Function to call before going to sleep.
 */
void CNSSInt::sleep()
{
  GPIO_InitTypeDef init;

  // Well, the external interruption line and the LoRa DIO1 signal share the
  // same EXTI line, so we have to make sure to set the configuration for the
  // external interruption signal again; it may have been overwritten by the LoRa layer.
  if(this->_sensitiveToExternalInterruption)
  {
    init.Alternate = 0;
    init.Speed     = GPIO_SPEED_FREQ_LOW;
    init.Pull      = WAKEUP_EXT_SENSOR_PULL;
    init.Mode      = GPIO_MODE_IT | WAKEUP_EXT_SENSOR_EDGE_SENSITIVITY;
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(WAKEUP_EXT_SENSOR_GPIO, &init.Pin), &init);
    gpio_set_irq_handler(WAKEUP_EXT_SENSOR_GPIO, CNSSINT_WAKEUP_IRQ_PRIORITY, dummyIrqHandler);
  }
}


/**
 * Configure the interruptions using JSON data.
 *
 * @param[in] json the JSON data to use.
 *
 * @return true  if the configuration has been done.
 * @return true  if there was nothing to do.
 * @return false otherwise.
 */
bool CNSSInt::setConfiguration(const JsonArray& json)
{
  const char *psName;
  bool        res = true;

  // Check that we have something to set up
  if(!json.success()) { return true; }

  uint8_t count = json.size();
  for(uint8_t i = 0; i < count; i++)
  {
    if(json[i]["name"].success())
    {
      psName = json[i]["name"].as<const char *>();
      if(!psName) continue;

      setConfigurationForInterruption(psName, json[i]);
    }

    if(json[i]["names"].success())
    {
      uint8_t icCount = json[i]["names"].size();
      for(uint8_t j = 0; j < icCount; j++)
      {
	psName = json[i]["names"][j].as<const char *>();
	setConfigurationForInterruption(psName, json[i]);
      }
    }
  }

  return res;
}

/**
 * Configure an interruption using JSON data
 *
 * @param[in] psName the interruption's name. Can be NULL or empty.
 * @param[in] json   the JSON data to use.
 *
 * @return true  on success.
 * @return false otherwise.
 * @return false if psName is NULL or empty.
 */
bool CNSSInt::setConfigurationForInterruption(const char *psName, const JsonObject &json)
{
  if(!psName || !*psName) { return false; }

  IntId id = getIdUsingName(psName);
  if(id == INT_NONE)
  {
    log_error(logger, "Cannot configure unknown interruption '%s'", psName);
    return false;
  }

  bool res = true;
  if(json["debounceMs"].success())
  {
    uint32_t ms = json["debounceMs"].as<uint32_t>();
    if(setDebounce(id, ms))
    {
      log_debug(logger, "Interruption '%s' is set up with a debounce time of  %d ms.", psName, ms);
    }
    else { res = false; }
  }

  return res;
}

/**
 * Set an interruption's debounce time.
 *
 * @param[in] id the interruption's identifier.
 * @param[in] ms the debounce time, in milliseconds. O means de-activate debouncing.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool CNSSInt::setDebounce(IntId id, uint32_t ms)
{
  if(id >= INTERRUPTION_COUNT) { return false; }

  this->_intParams[id].debounceMs = ms;
  return true;
}


/**
 * Register an interruption client or update the list of interruption he is listening to.
 *
 * @param[in] client          the client to register.
 * @param[in] sensitivityMask a ORed combination of IntFlag the client want to listens to.
 * @param[in] append          add sensivityMash to the current one (true) or replace current one (false).
 *
 * @return true  one success.
 * @return false if no more clients can be registered; the client list is full.
 */
bool CNSSInt::registerClient(CNSSIntClient &client, Interruptions sensitivityMask, bool append)
{
  if(this->_nbClients >= CNSSIT_CLIENT_COUNT_MAX) { return false; }

  // See if this client already is registered
  Client *pvClient = NULL;
  for(uint16_t i = 0; i < this->_nbClients; i++)
  {
    if(this->_clients[i].pvClient == &client)
    {
      pvClient = &this->_clients[i];
      break;
    }
  }

  if(pvClient)
  {
    // Yes, it already is registered; only update it's sensitivity.
    if(append) { pvClient->sensitivityMask |= sensitivityMask; }
    else
    {
      if(sensitivityMask == INT_FLAG_NONE) { unregisterClient(client);                    }
      else                                 { pvClient->sensitivityMask = sensitivityMask; }
    }
  }
  else
  {
    // New client; register it.
    pvClient = &this->_clients[this->_nbClients++];
    pvClient->pvClient        = &client;
    pvClient->sensitivityMask = sensitivityMask;
  }

  // Activate hardware interruptions if we need to
  if(containsExternalInterrupt(sensitivityMask) && !this->_sensitiveToExternalInterruption)
  {
    log_debug(logger, "Is sensitive to external interruptions");
    gpio_set_irq_handler(WAKEUP_EXT_SENSOR_GPIO, CNSSINT_WAKEUP_IRQ_PRIORITY, dummyIrqHandler);
    this->_sensitiveToExternalInterruption = true;
  }
  if(containsInternalInterrupt(sensitivityMask) && !this->_sensitiveToInternalInterruption)
  {
    log_debug(logger, "Is sensitive to internal interruptions");
    gpio_set_irq_handler(WAKEUP_INT_SENSOR_GPIO, CNSSINT_WAKEUP_IRQ_PRIORITY, dummyIrqHandler);
    this->_sensitiveToInternalInterruption = true;
  }

  return true;
}

/**
 * Register an interruption client or update the list of interruption he is listening to.
 *
 * @param[in] client    the client to register.
 * @param[in] psIntName the name of the interruption to listen to. Can be NULL.
 * @param[in] append    add sensivityMash to the current one (true) or replace current one (false).
 *
 * @return the interruption associated with the given interruption name.
 * @return INT_NONE if no more clients can be registered; the client list is full.
 * @return INT_NONE if no interruption with the given name has been found.
 */
CNSSInt::IntFlag CNSSInt::registerClient(CNSSIntClient &client, const char *psIntName, bool append)
{
  IntFlag flag = getFlagUsingName(psIntName);
  if(flag == INT_FLAG_NONE) { return flag; }

  return registerClient(client, flag, append) ? flag : INT_FLAG_NONE;
}

/**
 * Unregister an interruption client.
 *
 * @param[in] client the client to unregister.
 */
void CNSSInt::unregisterClient(CNSSIntClient &client)
{
  uint16_t i;

  // Look for the client
  for(i = 0; i < this->_nbClients; i++)
  {
    if(this->_clients[i].pvClient == &client)
    {
      this->_nbClients--;
      break;
    }
  }

  // Shift all the clients that come after in the list one place down
  for( ; i < this->_nbClients; i++)
  {
    this->_clients[i].pvClient        = this->_clients[i + 1].pvClient;
    this->_clients[i].sensitivityMask = this->_clients[i + 1].sensitivityMask;
  }

  // See if we have to de-activate a hardware interruption
  Interruptions sensitivityMask = INT_FLAG_NONE;
  for(i = 0; i < this->_nbClients; i++)
  {
    sensitivityMask |= this->_clients[i].sensitivityMask;
  }
  if(!containsExternalInterrupt(sensitivityMask) && this->_sensitiveToExternalInterruption)
  {
    log_debug(logger, "Is no longer sensitive to external interruptions");
    gpio_remove_irq_handler(WAKEUP_EXT_SENSOR_GPIO);
    this->_sensitiveToExternalInterruption = false;
  }
  if(!containsInternalInterrupt(sensitivityMask) && this->_sensitiveToInternalInterruption)
  {
    log_debug(logger, "Is no longer sensitive to internal interruptions");
    gpio_remove_irq_handler(WAKEUP_INT_SENSOR_GPIO);
    this->_sensitiveToInternalInterruption = false;
  }
}


/**
 * Return the name corresponding to the alias.
 *
 * @param[in] psName the alias name.
 *
 * @return the interruption's name.
 * @return psName if no alias with the given name has been found.
 */
const char *CNSSInt::resolveAlias(const char *psName)
{
  if(psName && *psName)
  {
    for(const Alias *pvAlias = _aliases; pvAlias->psAlias; pvAlias++)
    {
      if(strcasecmp(psName, pvAlias->psAlias) == 0) { return pvAlias->psIntName; }
    }
  }

  return psName;
}


/**
 * Return the interruption configuration for a given name.
 *
 * The name comparison is not case sensitive.
 *
 * @param[in] psName the name of the interruption. Can be NULL or empty.
 *
 * @return the matching configuration.
 * @return NULL if no configuration could be found using the given name.
 */
const CNSSInt::IntConfig *CNSSInt::getConfigUsingName(const char *psName)
{
  psName = resolveAlias(psName);

  if(psName && *psName)
  {
    for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
    {
      if(strcasecmp(psName, pvIntCfg->psName) == 0) { return pvIntCfg; }
    }
  }

  return NULL;
}

/**
 * Return the interruption flag associated with a name.
 *
 * The name comparison is not case sensitive.
 *
 * @param[in] psName the name of the interruption we want the flag for. Can be NULL or empty.
 *
 * @return the matching interruption flag.
 * @return INT_FLAG_NONE if no interruption flag is associated with the given name.
 */
CNSSInt::IntFlag CNSSInt::getFlagUsingName(const char *psName)
{
  const IntConfig  *pvIntCfg = getConfigUsingName(psName);

  return pvIntCfg ? pvIntCfg->flag : INT_FLAG_NONE;
}

/**
 * Return the interruption identifier associated with a name.
 *
 * The name comparison is not case sensitive.
 *
 * @param[in] psName the name of the interruption. Can be NULL or empty.
 *
 * @return the matching interruption identifier.
 * @return INT_NONE if no interruption identifier is associated with the given name.
 */
CNSSInt::IntId CNSSInt::getIdUsingName(const char *psName)
{
  const IntConfig  *pvIntCfg = getConfigUsingName(psName);

  return pvIntCfg ? pvIntCfg->id : INT_NONE;
}

/**
 * Return the interruption name associated with a flag.
 *
 * @param[in] flag the interruption flag.
 *
 * @return the interruption's name.
 * @retrun NULL if the flag is unknown.
 */
const char *CNSSInt::getNameUsingFlag(IntFlag flag)
{
  for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
  {
    if(flag == pvIntCfg->flag) { return pvIntCfg->psName; }
  }

  return NULL;
}

/**
 * Return the interruption name using it's identifier..
 *
 * @param[in] id the identifier..
 *
 * @return the interruption's name.
 * @retrun NULL if the identifier is unknown.
 */
const char *CNSSInt::getNameUsingId(IntId id)
{
  for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
  {
    if(id == pvIntCfg->id) { return pvIntCfg->psName; }
  }

  return NULL;
}

/**
 * Indicate if an ORed combination of interruption flags contains a flag for an external interrupt.
 *
 * @param[in] ints the ORed combination of interruption flags.
 *
 * @return true  if an external interruption has been found.
 * @return false otherwise.
 */
bool CNSSInt::containsExternalInterrupt(Interruptions ints)
{
  for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
  {
    if(!pvIntCfg->internal && (ints & pvIntCfg->flag)) { return true; }
  }

  return false;
}

/**
 * Indicate if an ORed combination of interruption flags contains a flag for an internal interrupt.
 *
 * @param[in] ints the ORed combination of interruption flags.
 *
 * @return true  if an internal interruption has been found.
 * @return false otherwise.
 */
bool CNSSInt::containsInternalInterrupt(Interruptions ints)
{
  for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
  {
    if(pvIntCfg->internal && (ints & pvIntCfg->flag)) { return true; }
  }

  return false;
}


/**
 * Process the current interruptions.
 *
 * @return true  if there was an interruption to process. Even if there was no client listening to it.
 * @return false otherwise.
 */
bool CNSSInt::processInterruptions()
{
  uint32_t      pin;
  bool          internalInterruption;
  bool          externalInterruption;
  Interruptions ints = INT_FLAG_NONE;

  // First process interruptions in a loop to handle rapid bounces
  do
  {
    internalInterruption = externalInterruption = false;

    // First perform a general check; is there an internal or an external interruption?
    if(HAL_GPIO_ReadPin(gpio_hal_port_and_pin_from_id(WAKEUP_INT_SENSOR_GPIO, &pin), pin) == GPIO_PIN_SET)
    {
      internalInterruption = true;
      log_debug(logger, "Internal interruption");
    }
    if(HAL_GPIO_ReadPin(gpio_hal_port_and_pin_from_id(WAKEUP_EXT_SENSOR_GPIO, &pin), pin) == GPIO_PIN_SET)
    {
      externalInterruption = true;
      log_debug(logger, "External interruption");
    }
    if(!internalInterruption && !externalInterruption) break;

    // Get individual interruptions
    for(const IntConfig *pvIntCfg = _intConfigs; pvIntCfg->psName; pvIntCfg++)
    {
      if(HAL_GPIO_ReadPin(gpio_hal_port_and_pin_from_id(pvIntCfg->gpio, &pin), pin) == GPIO_PIN_SET)
      {
	// Do we need to debounce this interruption?
	IntParams *pvIp = &this->_intParams[pvIntCfg->id];
	if(pvIp->debounceMs)
	{
	  uint32_t tsMs = board_ms_now();
	  if(board_ms_diff(pvIp->lastActivationTsMs, tsMs) < pvIp->debounceMs)
	  {
	    // We still are in the debounce time so ignore this interruption
	    log_info(logger, "Interruption '%s' is ignored because of debounce time.", pvIntCfg->psName);
	    continue;
	  }
	  // We are out of the debounce time, take this interruption into account
	  pvIp->lastActivationTsMs      = tsMs;
	}
	else { pvIp->lastActivationTsMs = 0; }

	ints |= pvIntCfg->flag;
	log_info(logger, "Interruption: %s", pvIntCfg->psName);
      }
    }

    // Clear interruption memories
    if(internalInterruption) { clearInternalInterruptions(); }
    if(externalInterruption) { clearExternalInterruptions(); }
  }
  while(internalInterruption | externalInterruption);
  if(ints == INT_FLAG_NONE) return false;

  // No that everything is debounced, we call call our clients

  // When the node is powered up the physical interruption memories are not properly initialised,
  // so we have false positives. Try to remove them.
  if(rtc_is_first_run_since_power_up())
  {
    log_info(logger, "All current interruptions have been triggered by power up; ignore them.");
    return false;
  }

  // Now transmit interruptions to the clients
  for(uint16_t i = 0; i < this->_nbClients; i++)
  {
    Client *pvClient = &this->_clients[i];
    if(pvClient->sensitivityMask & ints) { pvClient->pvClient->processInterruption(ints); }
  }

  return true;
}

/**
 * Clear the internal interruptions.
 */
void CNSSInt::clearInternalInterruptions()
{
  clearInterruptions(WAKEUP_INT_SENSOR_GPIO,
		     SENSOR_INT_CLEAR_INTERRUPT_GPIO,
		     SENSOR_INT_CLEAR_INTERRUPT_CLEAR_LEVEL);
}

/**
 * Clear the external interruptions.
 */
void CNSSInt::clearExternalInterruptions()
{
  clearInterruptions(WAKEUP_EXT_SENSOR_GPIO,
		     SENSOR_EXT_CLEAR_INTERRUPT_GPIO,
		     SENSOR_EXT_CLEAR_INTERRUPT_CLEAR_LEVEL);
}

/**
 * Clear interruptions (internal or external), using their GPIO identifiers.
 *
 * @param[in] readGPIO   the GPIO used to read the interruption status.
 * @param[in] clearGPIO  the GPIO used to clear the interruptions.
 * @param[in] clearLevel the level (high or low) to use to clear the interruptions.
 */
void CNSSInt::clearInterruptions(GPIOId readGPIO, GPIOId clearGPIO, int32_t clearLevel)
{
  GPIO_TypeDef *pv_read_port, *pv_clear_port;
  uint32_t      read_pin,      clear_pin;

  pv_read_port  = gpio_hal_port_and_pin_from_id(readGPIO,  &read_pin);
  pv_clear_port = gpio_hal_port_and_pin_from_id(clearGPIO, &clear_pin);

  while(HAL_GPIO_ReadPin(pv_read_port, read_pin) == GPIO_PIN_SET)
  {
    HAL_GPIO_WritePin(pv_clear_port, clear_pin, (GPIO_PinState) clearLevel);
    HAL_Delay(1);
    HAL_GPIO_WritePin(pv_clear_port, clear_pin, (GPIO_PinState)!clearLevel);
    HAL_Delay(1);
  }
}

/**
 * Clear all interruptions.
 */
void CNSSInt::clearAllInterruptions()
{
  clearInternalInterruptions();
  clearExternalInterruptions();
}


/**
 * Called when a USB cable has been plugged in.
 */
void CNSSInt::USBIrqHandler(void)
{
  board_reset(BOARD_SOFTWARE_RESET_OK);
}

/**
 * Called when an internal or an external sensor interruption line is activated.
 */
void CNSSInt::dummyIrqHandler(void)
{
  // Do nothing; waking up from sleep is enough.
}

