/**
 * Class for SPI communication.
 *
 * @file spi.hpp
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include <string.h>
#include "spi.hpp"
#include "gpio.h"
#include "logger.h"
#include "powerandclocks.h"


CREATE_LOGGER(  spi);
#undef  _logger
#define _logger spi


#define SPI_DEFAULT_SS_EST_PORT_NAME  "SPI_CS"

const SPI::SSInfos SPI::DEFAULT_SS_INFOS[2] =
{
    { "SS", SS_IDFLAG_DEFAULT, true  },
    { NULL, SS_IDFLAG_NONE,    false }  // End of list
};


/**
 * Initialise the SPI interface.
 *
 * @param[in] sckFreqHz  The frequency of the SPI SCK signal, in Hz. Only used in master mode.
 * @param[in] options    The SPI options to use.
 *                       If your sensor uses several SPI devices then declare the most
 *                       complete signal usage (#OPTION_USE_MOSI and #OPTION_USE_MISO).
 *                       Because the GPIO configuration will be done only once when the
 *                       sensor is opened, so you must declare here all the SPI GPIOs
 *                       than can be used by your sensor.
 * @param[in] ptSSInfos  The list of the slave select signals informations.
 *                       The end of the list is indicated using a SS infos whose
 *                       @p psConfigKeyBaseName is NULL.
 *                       If the sensor does not use SS signal then set this parameter
 *                       to NULL.
 *                       To use a single SS signal active low then use
 *                       SSInfos::_DEFAULT_SS_INFOS.
 */
SPI::SPI(uint32_t       sckFreqHz,
	 Options        options,
	 const SSInfos *ptSSInfos)
{
  uint32_t i;

  this->_spi.Instance   = PASTER2(SPI, EXT_SPI_ID);
  this->_sckFreqHz      = sckFreqHz;
  this->_options        = options;
  this->_ptSSConfigs    = NULL;
  this->_ssConfigsCount = 0;

  // Set up slave select signal(s).
  if(ptSSInfos)
  {
    // First count the number of slave signals to use.
    for(i = 0; ptSSInfos[i].psConfigKeyBaseName ; i++) ;  // Do nothing, just count.

    if(i)
    {
      // Allocate memory to store their configuration.
      this->_ptSSConfigs = new SSConfig[i];
      if(this->_ptSSConfigs)
      {
	this->_ssConfigsCount = i;

	// Set up the configurations
	for(i = 0; ptSSInfos[i].psConfigKeyBaseName; i++)
	{
	  this->_ptSSConfigs[i].pvInfos = &ptSSInfos[i];
	  this->_ptSSConfigs[i].pvPin   = NULL;

	  // Set up the default SS signal if it's used.
	  if(ptSSInfos[i].idFlag & SS_IDFLAG_DEFAULT)
	  {
	    this->_ptSSConfigs[i].pvPin =
		extport_get_pininfos_with_functions(SPI_DEFAULT_SS_EST_PORT_NAME,
						    EXTPORT_PIN_FN_DIGITAL_OUTPUT);
	  }
	}
      }
      else
      {
	log_error(_logger, "Cannot allocate memory to store slave select configurations.");
      }
    }
  }
}

/**
 * Destructor.
 */
SPI::~SPI()
{
  if(this->_ptSSConfigs)
  {
    delete this->_ptSSConfigs;
    this->_ptSSConfigs    = NULL;
    this->_ssConfigsCount = 0;
  }
}


bool SPI::initUsingJSON(const JsonObject& json)
{
  uint8_t     i;
  size_t      len, bufferSize;
  char        buffer[48], *ps;
  const char *psValue;
  SSConfig   *pvSSConfig;
  bool        res = true;

  // Get the slave select configurations
  if(this->_ssConfigsCount)
  {
    // Get slave select configurations
    for(i = 0; i < this->_ssConfigsCount; i++)
    {
      pvSSConfig = &this->_ptSSConfigs[i];

      // Copy JSON key base name to buffer.
      len = strlcpy(buffer, pvSSConfig->pvInfos->psConfigKeyBaseName, sizeof(buffer));
      if(len >= sizeof(buffer))
      {
	log_error(_logger, "SS config key base name '%s' is too long.", pvSSConfig->pvInfos->psConfigKeyBaseName);
	continue;
      }
      ps         = buffer         + len;
      bufferSize = sizeof(buffer) - len;

      // Look for extension port pin that drives the SS signal.
      len = strlcpy(ps, "ExtPin", bufferSize);
      if(len >= bufferSize)
      {
	log_error(_logger, "SS config key base name '%s' is too long.", pvSSConfig->pvInfos->psConfigKeyBaseName);
	continue;
      }
      if(json[buffer].success())
      {
	if(!(psValue = json[buffer].as<const char *>()) || !*psValue)
	{
	  log_error(_logger, "Value for JSON key '%s' must be a not empty string.", buffer);
	  continue;
	}
	if(!(pvSSConfig->pvPin =
	    extport_get_pininfos_with_functions(psValue, EXTPORT_PIN_FN_DIGITAL_OUTPUT)))
	{
	  log_error(_logger, "Cannot use extension pin '%s' for SPI slave select '%s'.",
				     psValue, buffer);
	  continue;
	}
      }

      // Make sure that all slave select has been set up
      for(i = 0; i < this->_ssConfigsCount; i++)
      {
        pvSSConfig = &this->_ptSSConfigs[i];
        if(!pvSSConfig->pvPin)
        {
          log_error(_logger, "No extension port pin set as SPI slave select '%s'.", pvSSConfig->pvInfos->psConfigKeyBaseName);
          res = false;
        }
      }
    }
  }

  return res;
}


bool SPI::open()
{
  uint32_t         i;
  GPIO_InitTypeDef init;
  GPIO_TypeDef    *pvPort;
  bool             res;

  // Enable SPI module clock
  PASTER3(__HAL_RCC_SPI, EXT_SPI_ID, _CLK_ENABLE)();

  // Configure SPI module
  res = initUsingOptions();

  // Set up GPIOs
  init.Mode      = GPIO_MODE_AF_PP;
  init.Pull      = GPIO_NOPULL;
  init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  // SCK
  init.Alternate = EXT_SPI_CLK_AF;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(EXT_SPI_CLK_GPIO, &init.Pin), &init);
  // MOSI
  switchMOSIToGPIOInput(false);
  // MISO
  switchMISOToGPIOInput(false);
  // Slave select signal(s).
  if(this->_options & OPTION_MASTER)
  {
    init.Alternate = 0;
    init.Mode      = GPIO_MODE_OUTPUT_PP;
    for(i = 0; i < this->_ssConfigsCount; i++)
    {
      pvPort = gpio_hal_port_and_pin_from_id(this->_ptSSConfigs[i].pvPin->gpio, &init.Pin);
      HAL_GPIO_Init(    pvPort, &init);
      HAL_GPIO_WritePin(pvPort,
			init.Pin,
			(GPIO_PinState)this->_ptSSConfigs[i].pvInfos->activeLow);
    }
  }

  return res;
}

void SPI::close()
{
  uint32_t i, pin;

  // Make sure to de-select all slaves.
  selectSlave(SS_ID_NONE);

  // Deinit SPI module and stop its clock.
  HAL_SPI_DeInit(&this->_spi);
  PASTER3(__HAL_RCC_SPI, EXT_SPI_ID, _CLK_DISABLE)();

  // Free GPIOs
  if(this->_options & OPTION_MASTER)
  {
    for(i = 0; i < this->_ssConfigsCount; i++)
    {
      HAL_GPIO_DeInit(gpio_hal_port_and_pin_from_id(this->_ptSSConfigs[i].pvPin->gpio,
						    &pin),
		      pin);
    }
  }
  HAL_GPIO_DeInit(  gpio_hal_port_and_pin_from_id(EXT_SPI_CLK_GPIO,  &pin), pin);
  if(this->_options & OPTION_USE_MOSI)
  {
    HAL_GPIO_DeInit(gpio_hal_port_and_pin_from_id(EXT_SPI_MOSI_GPIO, &pin), pin);
  }
  if(this->_options & OPTION_USE_MOSI)
  {
    HAL_GPIO_DeInit(gpio_hal_port_and_pin_from_id(EXT_SPI_MISO_GPIO, &pin), pin);
  }
}

/**
 * Configure the SPI interface for using new communication options.
 *
 * This function is used by the openSpecific() function.
 * It can be used later on, to change the SPI configuration, for example if your
 * sensor is made of several SPI devices with different communication needs,
 * you can then change the configuration before talking with each of theses devices.
 *
 * @param[in] sckFreq The SPI SCK signal frequency to use, in Hz.
 *                    If set to 0 then the one set in the constructor will be used.
 * @param[in] options The SPI options to use.
 *                    If set to #OPTION_NONE then the options set in the constructor
 *                    will be used.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SPI::initUsingOptions(uint32_t sckFreq, Options options)
{
  uint32_t div, d, presc;

  if(!sckFreq)               { sckFreq = this->_sckFreqHz; }
  if(options == OPTION_NONE) { options = this->_options;   }

  // Reset SPI module.
  PASTER3(__HAL_RCC_SPI, EXT_SPI_ID, _FORCE_RESET)();
  PASTER3(__HAL_RCC_SPI, EXT_SPI_ID, _RELEASE_RESET)();

  // Compute the prescaler value to use to achieve a SCK frequency equal or lower
  // to sckFreqHz.
#if EXT_SPI_ID == 1
  div = pwrclk_clock_frequency_hz(PWRCLK_CLOCK_PCLK2);
#else
  div = pwrclk_clock_frequency_hz(PWRCLK_CLOCK_PCLK1);
#endif
  div /= sckFreq;
  for(presc = 0, d = 2;
      div   > d && presc <= SPI_BAUDRATEPRESCALER_256;
      presc++, d <<= 1) ;  // Do nothing
  if( presc > SPI_BAUDRATEPRESCALER_256)
  {
    log_error(_logger, "Cannot set SPI SCK frequency to %ul Hz.", sckFreq);
    goto error_exit;
  }

  // Init the SPI interface
  memset(&this->_spi.Init, 0, sizeof(this->_spi.Init));
  d = SPI_DIRECTION_2LINES;
  if(     options & OPTION_HALF_DUPLEX) { d = SPI_DIRECTION_1LINE;         }
  else if(options & OPTION_SIMPLEX)     { d = SPI_DIRECTION_2LINES_RXONLY; }
  this->_spi.Init.Direction         = d;
  this->_spi.Init.BaudRatePrescaler = presc << SPI_CR1_BR_Pos;
  this->_spi.Init.Mode              =  (options & OPTION_MASTER)          ? SPI_MODE_MASTER   : SPI_MODE_SLAVE;
  this->_spi.Init.CLKPolarity       =  (options & OPTION_CPOL_1)          ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
  this->_spi.Init.CLKPhase          =  (options & OPTION_CPHA_1)          ? SPI_PHASE_2EDGE   : SPI_PHASE_1EDGE;
  this->_spi.Init.FirstBit          =  (options & OPTION_SHIFT_MSB_FIRST) ? SPI_FIRSTBIT_MSB  : SPI_FIRSTBIT_LSB;
  this->_spi.Init.DataSize          = ((options & SENSOR_SPI_OPTION_DATA_SIZE_MASK) >> SENSOR_SPI_OPTION_DATA_SIZE_POS) << SPI_CR2_DS_Pos;
  this->_spi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  this->_spi.Init.NSS               = SPI_NSS_SOFT;
  this->_spi.Init.TIMode            = SPI_TIMODE_DISABLE;
  if(HAL_SPI_Init(&this->_spi) != HAL_OK)
  {
    log_error(_logger, "Failed to initialise SPI interface.");
    goto error_exit;
  }
  __HAL_SPI_ENABLE(&this->_spi);

  return true;

  error_exit:
  return false;
}

/**
 * Switch MOSI signal to a GPIO input or back to SPI module.
 *
 * @param[in] asGPIO Use MOSI pin as GPIO (true) or give control to SPI module (false).
 *
 * @return true  On success.
 * @return false if MOSI pin is not used by the SPI interface.
 */
bool SPI::switchMOSIToGPIOInput(bool asGPIO)
{
  GPIO_InitTypeDef init;
  bool             res = false;

  if(this->_options & OPTION_USE_MOSI)
  {
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    if(asGPIO)
    {
      init.Alternate = 0;
      init.Mode      = GPIO_MODE_INPUT;
    }
    else
    {
      init.Alternate = EXT_SPI_MOSI_AF;
      init.Mode      = GPIO_MODE_AF_PP;
    }
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(EXT_SPI_MOSI_GPIO, &init.Pin), &init);
    res = true;
  }

  return res;
}

/**
 * Switch MISO signal to a GPIO input or back to SPI module.
 *
 * @param[in] asGPIO Use MOSI pin as GPIO (true) or give control to SPI module (false).
 *
 * @return true  On success.
 * @return false if MOSI pin is not used by the SPI interface.
 */
bool SPI::switchMISOToGPIOInput(bool asGPIO)
{
  GPIO_InitTypeDef init;
  bool             res = false;

  if(this->_options & OPTION_USE_MISO)
  {
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    if(asGPIO)
    {
      init.Alternate = 0;
      init.Mode      = GPIO_MODE_INPUT;
    }
    else
    {
      init.Alternate = EXT_SPI_MISO_AF;
      init.Mode      = GPIO_MODE_AF_PP;
    }
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(EXT_SPI_MISO_GPIO, &init.Pin),&init);
    res = true;
  }

  return res;
}

/**
 * Switch MOSI to GPIO input and wait for a given logic level, then
 * give back MOSI to SPI module.
 *
 * @param[in] level     The logic level to wait for.
 * @param[in] timeoutMs The maximum time to wait for the logic level, in milliseconds.
 *
 * @return true  If the logic level has been detected in time.
 * @return false otherwise.
 */
bool SPI::waitForMOSItoGo(LogicLevel level, uint32_t timeoutMs)
{
  GPIO_TypeDef *pvPort;
  uint32_t      pin, msRef;
  bool          res = false;

  if(!switchMOSIToGPIOInput(true)) { goto exit; }

  pvPort = gpio_hal_port_and_pin_from_id(EXT_SPI_MOSI_GPIO, &pin);
  msRef  = board_ms_now();
  while(1)
  {
    if(HAL_GPIO_ReadPin(pvPort, pin) == (GPIO_PinState)level) { res = true; break; }
    if(board_is_timeout(msRef, timeoutMs)) break;
  }
  switchMOSIToGPIOInput(false);

  exit:
  return res;
}

/**
 * Switch MISO to GPIO input and wait for a given logic level, then
 * give back MISO to SPI module.
 *
 * @param[in] level     The logic level to wait for.
 * @param[in] timeoutMs The maximum time to wait for the logic level, in milliseconds.
 *
 * @return true  If the logic level has been detected in time.
 * @return false otherwise.
 */
bool SPI::waitForMISOtoGo(LogicLevel level, uint32_t timeoutMs)
{
  GPIO_TypeDef *pvPort;
  uint32_t      pin, msRef;
  bool          res = false;

  if(!switchMOSIToGPIOInput(true)) { goto exit; }

  pvPort = gpio_hal_port_and_pin_from_id(EXT_SPI_MISO_GPIO, &pin);
  msRef  = board_ms_now();
  while(1)
  {
    if(HAL_GPIO_ReadPin(pvPort, pin) == (GPIO_PinState)level) { res = true; break; }
    if(board_is_timeout(msRef, timeoutMs)) break;
  }
  switchMOSIToGPIOInput(false);

  exit:
  return res;
}


/**
 * Select a slave SPI device, or no slave.
 *
 * @note This function does nothing if the SPI interface is not configured in master mode.
 *
 * @param[in] id The identifier of the slave to select.
 */
void SPI::selectSlave(SSId id)
{
  uint32_t      i, pin;
  const SSInfos *pvInfos;
  GPIO_TypeDef  *pvPort;

  if(!(this->_options & OPTION_MASTER)) { return; }

  for(i = 0; i < this->_ssConfigsCount; i++)
  {
    pvInfos = this->_ptSSConfigs[i].pvInfos;
    pvPort  = gpio_hal_port_and_pin_from_id(this->_ptSSConfigs[i].pvPin->gpio, &pin);
    HAL_GPIO_WritePin(pvPort, pin,
		      (GPIO_PinState)((id & pvInfos->idFlag) ?
			  !pvInfos->activeLow : pvInfos->activeLow));
  }
}


/**
 * Transmit data over SPI, ignore any data received.
 *
 * @param[in] pu8Data   Pointer to the data to send. MUST be NOT NULL.
 * @param[in] size      The number of data to send.
 *                      If SPI data are <= 8 bits then this is the number of data bytes.
 *                      If SPI data are > 8 and <= 16 then this is the number of data words.
 * @param[in] timeoutMs The maximum time allowed, in milliseconds, for the
 *                      write operation to complete.
 *
 * @return true  If all data have been sent in time.
 * @return false otherwise.
 */
bool SPI::write(const uint8_t *pu8Data, uint16_t size, uint32_t timeoutMs)
{
  return HAL_SPI_Transmit(&this->_spi, (uint8_t *)pu8Data, size, timeoutMs) == HAL_OK;
}

/**
 * Receive data over SPI.
 *
 * @param[out] pu8Data   Where the received data are written to. MUST be NOT NULL.
 * @param[in]  nb        The number of data to read.
 *                       If SPI data are <= 8 bits then this is the number of data bytes.
 *                       If SPI data are > 8 and <= 16 then this is the number of data words.
 * @param[in]  timeoutMs The maximum time allowed, in milliseconds, for the
 *                       read operation to complete.
 *
 * @return true  If all data have been received in time.
 * @return false otherwise.
 */
bool SPI::read(uint8_t *pu8Data, uint16_t nb,  uint32_t timeoutMs)
{
  return HAL_SPI_Receive(&this->_spi, pu8Data, nb, timeoutMs) == HAL_OK;
}

/**
 * Transmit and receive data over SPI, these operations are done simultaneously.
 *
 * @attention This only works in full-duplex configuration.
 *
 * @param[in]  pu8TxData   Pointer to the data to send. MUST be NOT NULL.
 * @param[out] pu8RxData   Where the received data are written to. MUST be NOT NULL.
 * @param[in]  size        The number of data to send and to read.
 *                         If SPI data are <= 8 bits then this is the number of data bytes.
 *                         If SPI data are > 8 and <= 16 then this is the number of data words.
 *
 * @return true  If all data have been sent and received in time.
 * @return false otherwise.
 */
bool SPI::writeAndRead(const uint8_t *pu8TxData,
		       uint8_t       *pu8RxData,
		       uint16_t       size,
		       uint32_t       timeoutMs)
{
  return HAL_SPI_TransmitReceive(&this->_spi,
				 (uint8_t *)pu8TxData, pu8RxData, size,
				 timeoutMs) == HAL_OK;
}

