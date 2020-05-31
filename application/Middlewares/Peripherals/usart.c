/*
 * Implements USART communication.
 *
 * @author: Jérôme FUCHET
 * @date:   2018
 */

#include <string.h>
#include "usart.h"
#include "logger.h"
#include "powerandclocks.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef USART_IRQ_PRIORITY
#define USART_IRQ_PRIORITY  10
#else
#if USART_IRQ_PRIORITY < 0 || USART_IRQ_PRIORITY < 15
#error "USART_IRQ_PRIORITY MUST be in range [0..15]."
#endif
#endif


  CREATE_LOGGER(usart);
#undef  logger
#define logger  usart


  /**
   * USART's signal identifiers.
   */
  typedef enum USARTSignal
  {
    USART_SIGNAL_NONE,
    USART_SIGNAL_TX,
    USART_SIGNAL_RX
  }
  USARTSignal;


  /**
   * The type used to store information about a USART user.
   */
  typedef struct USARTUser
  {
    const char *ps_name;
  }
  USARTUser;


  /**
   * Defines the interruption options.
   */
  typedef enum USARTItOption
  {
    USART_IT_NONE               = 0,        ///< Do not use interruptions.
    USART_IT_RX                 = 1u << 0,  ///< Use interruption for Rx.
    USART_IT_RX_USE_DMA         = 1u << 1,  ///< Use DMA for Rx.
    USART_IT_RX_FROM            = 1u << 2,  ///< Rx use 'from' mode.
    USART_IT_RX_TO              = 1u << 3,  ///< Rx use 'to' mode.
    USART_IT_RX_KEEP_BOUNDARIES = 1u << 4,  ///< Rx keep boundaries in 'from' and 'to' modes.
    USART_IT_RX_CONTINUOUS      = 1u << 5   ///< Continuous reading, and not single-shot.
  }
  USARTItOption;
  typedef uint8_t  USARTItOptions;  ///< Type used to store a ORed combination of USARTItOption values.


  /**
   * Defines the IRQ handler identifier.
   */
  typedef enum USARTIrqHandlerId
  {
    USART_IRQ_HANDLER_USART1,
    USART_IRQ_HANDLER_USART2,
    USART_IRQ_HANDLER_USART3,
    USART_IRQ_HANDLER_UART4,
    USART_IRQ_HANDLER_UART5
  }
  USARTIrqHandlerId;
#define USART_IRQ_HANDLER_ID_COUNT  5


  /**
   * Stores information for an IRQ handler.
   */
  typedef struct USARTIrqHandlerInfos
  {
    USART *pv_usart;  ///< The USART object the interruption is for.
  }
  USARTIrqHandlerInfos;


  /**
   * Defines the interruption Rx states.
   */
  typedef enum USARTItRxState
  {
    USART_IT_RX_STATE_IDLE,              ///< Idle
    USART_IT_RX_STATE_WAITING_FOR_FROM,  ///< Waiting for the 'from' data byte.
    USART_IT_RX_STATE_WAITING_FOR_TO,    ///< Waiting for the 'to' data byte.
    USART_IT_RX_STATE_RECEIVING          ///< Receiving data.
  }
  USARTItRxState;


  /**
   * The type used to store USART interruption informations.
   */
  typedef struct USARTIt
  {
    USARTItOptions options;    ///< The interruption options

    USARTIrqHandlerId    irq_handler_id;     ///< The IRQ handler identifier.
    DMA_Channel_TypeDef *pv_rx_dma_channel;  ///< The DMA channel that can be used for Rx.

    USARTItRxState    rx_state;        ///< The Rx state.
    uint8_t           rx_from;         ///< The 'from' byte to start copy data at.
    uint8_t           rx_to;           ///< the 'to' byte to stop copy data at.
    uint8_t          *pu8_rx_buffer;   ///< The buffer Rx buffer.
    uint32_t          rx_buffer_size;  ///< The Rx buffer's size, in bytes.
    uint32_t          rx_data_count;   ///< The number of data bytes received.
    USARTItRxCallback pf_rx_cb;        ///< The Rx callback.
    void             *pv_rx_cb_args;   ///< The Tx callback.
  }
  USARTIt;

#define USART_DMA_CHANNEL(num, cha_num)  PASTER4(DMA, num, _Channel, cha_num)


  /**
   * Define a USART object.
   */
  struct USART
  {
    const USARTId id;       ///< The USART's identifier.
    const char   *ps_name;  ///< The USART's name.

    PwrClkClockId _clock_source;  ///< The clock used by the USART.
    bool          _is_opened;     ///< Indicate if the UART is opened or not.
    uint32_t      _baudrate;      ///< The USART's current baudrate.
    USARTParams   _params;        ///< The USART's parameters.

    UART_HandleTypeDef  _uart;       ///< The UART peripheral
    GPIOId              _tx_gpio;    ///< The TX GPIO.
    uint32_t            _tx_pin_af;  ///< TX GPIO pin alternate function.
    GPIOId              _rx_gpio;    ///< The RX GPIO.
    uint32_t            _rx_pin_af;  ///< RX GPIO pin alternate function.

    USARTIt     it;         ///< Interruptions data.
  };


  /**
   * Keep track of which USART is used by whom.
   *
   * The index of the table is the USART identifier.
   * If the USARTUser's ps_name is NULL then the USART is free.
   */
  static USARTUser _usarts_in_use_by[USART_ID_COUNT];  // Table is all set to NULL by startup initialisation.

  static USARTIrqHandlerInfos _usarts_irq_handler_infos[USART_IRQ_HANDLER_ID_COUNT];

  /**
   * Gives the NVIC IRQ number using the USART's IRQ handler id.
   */
  static const IRQn_Type _usart_irqn[USART_IRQ_HANDLER_ID_COUNT] =
  {
      USART1_IRQn,
      USART2_IRQn,
      USART3_IRQn,
      UART4_IRQn,
      UART5_IRQn,
  };

  static USART _usarts[USART_ID_COUNT] =
  {
      {
	  USART_ID_EXT, "EXT_USART", PWRCLK_CLOCK_PCLK1, false, 0, USART_PARAM_NONE,
	  {
#if EXT_USART_ID <= 3
	  PASTER2(USART, EXT_USART_ID),
#else
	  PASTER2(UART, EXT_USART_ID),
#endif
	  },
	  EXT_USART_TX_GPIO, EXT_USART_TX_AF,
	  EXT_USART_RX_GPIO, EXT_USART_RX_AF,
	  {
	      USART_IT_NONE,
	      (USARTIrqHandlerId)(EXT_USART_ID - 1),
	      USART_DMA_CHANNEL(EXT_USART_RX_DMA_NUM, EXT_USART_RX_DMA_CHANNEL_NUM),
	      USART_IT_RX_STATE_IDLE, 0, 0, NULL, 0, 0, NULL, NULL
	  }
      },
      {
	  USART_ID_SIGFOX, "SigFox_USART", PWRCLK_CLOCK_PCLK1, false, 0, USART_PARAM_NONE,
	  {
#if SIGFOX_USART_ID <= 3
	  PASTER2(USART, SIGFOX_USART_ID),
#else
	  PASTER2(UART, SIGFOX_USART_ID),
#endif
	  },
	  SIGFOX_USART_TX_GPIO, SIGFOX_USART_TX_AF,
	  SIGFOX_USART_RX_GPIO, SIGFOX_USART_RX_AF,
	  {
	      USART_IT_NONE,
	      (USARTIrqHandlerId)(SIGFOX_USART_ID - 1),
	      USART_DMA_CHANNEL(SIGFOX_USART_RX_DMA_NUM, SIGFOX_USART_RX_DMA_CHANNEL_NUM),
	      USART_IT_RX_STATE_IDLE, 0, 0, NULL, 0, 0, NULL, NULL
	  }
      },
#ifdef A2135_USART_ID
      {
	  USART_ID_GPS_A2135, "A2135_GPS_USART", PWRCLK_CLOCK_PCLK1,
	  false, 0, USART_PARAM_NONE,
	  {
#if A2135_USART_ID <= 3
	  PASTER2(USART, A2135_USART_ID),
#else
	  PASTER2(UART, A2135_USART_ID),
#endif
	  },
#ifdef A2135_USART_TX_GPIO
	  A2135_USART_TX_GPIO, A2135_USART_TX_AF,
#else
	  GPIO_NONE,           0,
#endif
	  A2135_USART_RX_GPIO, A2135_USART_RX_AF,
	  {
	      USART_IT_NONE,
	      (USARTIrqHandlerId)(A2135_USART_ID - 1),
	      USART_DMA_CHANNEL(A2135_USART_RX_DMA_NUM, A2135_USART_RX_DMA_CHANNEL_NUM),
	      USART_IT_RX_STATE_IDLE, 0, 0, NULL, 0, 0, NULL, NULL
	  }
      }
#endif // A2135_USART_ID
  };

  static void _usart_pwrclk_listerner_cb(PwrClkPowerMode current_mode,
					 PwrClkPowerMode previous_mode);
  /// Power mode change listener.
  static PwrClkPowerModeChangeListener _usart_pwrclk_listener =
  {
      "USART",
      _usart_pwrclk_listerner_cb,
      NULL
  };

  static bool _usart_pwrclk_listener_has_been_registered = false;


  static void usart_init_ios(    USART *pv_usart);
  static void usart_deinit_ios(  USART *pv_usart);
  static void usart_enable_clock(USART *pv_usart, bool enable);

  static void usart_set_it_rx_base(USART            *pv_usart,
				   USARTItOptions    options,
				   uint8_t          *pu8_buffer,
				   uint32_t          size,
				   USARTItRxCallback pf_cb,
				   void             *pv_cb_args);

  static void usart_process_irq_with_id(     USARTIrqHandlerId irq_id);
  static void usart_process_rx_irq_for_usart(USART            *pv_usart);
  static void usart_process_rx_irq_add_data( USART            *pv_usart, uint8_t b);


  /**
   * Return the USART's name.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   *
   * @return the name. Is NOT NULL and NOT EMPTY.
   */
  const char *usart_name(USART *pv_usart) { return pv_usart->ps_name; }

  /**
   * Indicate if the USART is opened or not.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   *
   * @return true  if the USART is opened.
   * @return false otherwise.
   */
  bool usart_is_opened(USART *pv_usart) { return pv_usart->_is_opened; }


  /**
   * Return a USART object using it's identifier.
   *
   * @param[in] id the UART's identifier.
   *
   * @return the UART object.
   * @return NULL in case of error.
   */
  USART *usart_get_by_id(USARTId id)
  {
    return (id < 0 || id >= USART_ID_COUNT) ? NULL : &_usarts[id];
  }

  /**
   * Return a USART object using it's name.
   *
   * @note the comparison is not case sensitive.
   *
   * @param[in] ps_name the UART's name. Can be NULL.
   *
   * @return the UART object.
   * @return NULL in case of error.
   */
  USART *usart_get_by_name(const char *ps_name)
  {
    uint8_t i;

    if(ps_name && *ps_name)
    {
      for(i = 0; i < USART_ID_COUNT; i++)
      {
	if(strcasecmp(_usarts[i].ps_name, ps_name) == 0) { return &_usarts[i]; }
      }
    }

    return NULL;
  }

  /**
   * Set an USART's clock source.
   *
   * @param[in] pv_usart The USART whose clock to set up.
   * @param[in] clock_id The identifier of the clock to use.
   *                     Only #PWRCLK_CLOCK_SYSCLK, #PWRCLK_CLOCK_PCLK1,
   *                     #PWRCLK_CLOCK_PCLK2, #PWRCLK_CLOCK_HSI16 and #PWRCLK_CLOCK_LSE
   *                     are valid values. #PWRCLK_CLOCK_PCLK1 and #PWRCLK_CLOCK_PCLK1
   *                     are interchangeable, they are processed the same way.
   */
  static void usart_set_clock_source(USART *pv_usart, PwrClkClockId clock_id)
  {
    RCC_OscInitTypeDef osc_init;
    uint32_t           reg_value, reg_pos;

    // Get the register value to write
    switch(clock_id)
    {
      case PWRCLK_CLOCK_PCLK1:
      case PWRCLK_CLOCK_PCLK2:
	reg_value = 0;
	break;
      case PWRCLK_CLOCK_SYSCLK: reg_value = 1; break;
      case PWRCLK_CLOCK_HSI16:
	reg_value = 2;
	// Make sure that the HSI clock is enabled.
	osc_init.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
	osc_init.HSIState            = RCC_HSI_ON;
	osc_init.HSICalibrationValue = 0;
	if(HAL_RCC_OscConfig(&osc_init) != HAL_OK)
	{
	  log_error(logger, "Failed to turn ON HSI clock for USART '%s'.", pv_usart->ps_name);
	}
	break;
      case PWRCLK_CLOCK_LSE: reg_value = 3; break;
      default:
	log_fatal(logger, "Cannot use clock source %d for USART.");
	goto exit;
    }

    // Then get the register place to write the value to.
    switch((uint32_t)pv_usart->_uart.Instance)
    {
#ifdef USART1
      case (uint32_t)USART1: reg_pos = 0; break;
#endif
#ifdef USART2
      case (uint32_t)USART2: reg_pos = 2; break;
#endif
#ifdef USART3
      case (uint32_t)USART3: reg_pos = 4; break;
#endif
#ifdef UART4
      case (uint32_t)UART4:  reg_pos = 6; break;
#endif
#ifdef UART5
      case (uint32_t)UART5:  reg_pos = 8; break;
#endif
      default:
	log_fatal(logger, "Unknown USART '%08X'.", pv_usart->_uart.Instance);
	goto exit;
    }

    // Write to register
    RCC->CCIPR &= ~(3u      << reg_pos);
    RCC->CCIPR |= reg_value << reg_pos;

    pv_usart->_clock_source = clock_id;

    exit:
    return;
  }

  /**
   * Enable/disable the clock for an USART.
   *
   * @param[in] pv_usart the USART. MUST be NOT NULL.
   * @param[in] enable   enable (true) or disable (false) the USART.
   */
  static void usart_enable_clock(USART *pv_usart, bool enable)
  {
    switch((uint32_t)pv_usart->_uart.Instance)
    {
#ifdef USART1
      case (uint32_t)USART1:
	if(enable) { __HAL_RCC_USART1_CLK_ENABLE();  }
	else       { __HAL_RCC_USART1_CLK_DISABLE(); }
	break;
#endif
#ifdef USART2
      case (uint32_t)USART2:
	if(enable) { __HAL_RCC_USART2_CLK_ENABLE();  }
	else       { __HAL_RCC_USART2_CLK_DISABLE(); }
	break;
#endif
#ifdef USART3
      case (uint32_t)USART3:
	if(enable) { __HAL_RCC_USART3_CLK_ENABLE();  }
	else       { __HAL_RCC_USART3_CLK_DISABLE(); }
	break;
#endif
#ifdef UART4
      case (uint32_t)UART4:
	if(enable) { __HAL_RCC_UART4_CLK_ENABLE();  }
	else       { __HAL_RCC_UART4_CLK_DISABLE(); }
	break;
#endif
#ifdef UART5
      case (uint32_t)UART5:
	if(enable) { __HAL_RCC_UART5_CLK_ENABLE();  }
	else       { __HAL_RCC_UART5_CLK_DISABLE(); }
	break;
#endif
    }
  }


  /**
   * Configure the USART GPIOs.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   */
  static void usart_init_ios(USART *pv_usart)
  {
    GPIO_InitTypeDef init;

    init.Mode  = GPIO_MODE_AF_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    // TX
    if(pv_usart->_params & USART_PARAM_MODE_TX)
    {
      init.Alternate = pv_usart->_tx_pin_af;
      gpio_use_gpio_with_id(pv_usart->_tx_gpio);
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(pv_usart->_tx_gpio, &init.Pin), &init);
    }
    // RX
    if(pv_usart->_params & USART_PARAM_MODE_RX)
    {
      init.Alternate = pv_usart->_rx_pin_af;
      gpio_use_gpio_with_id(pv_usart->_rx_gpio);
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(pv_usart->_rx_gpio, &init.Pin), &init);
    }
  }

  /**
   * Set the GPIOs used by the the USART in analog mode.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   */
  static void usart_deinit_ios(USART *pv_usart)
  {
    if(pv_usart->_params & USART_PARAM_MODE_TX) { gpio_free_gpio_with_id(pv_usart->_tx_gpio); }
    if(pv_usart->_params & USART_PARAM_MODE_RX) { gpio_free_gpio_with_id(pv_usart->_rx_gpio); }
  }


  /**
   * Open the serial port.
   *
   * @param[in] pv_usart     the USART object to open. MUST be NOT NULL.
   * @param[in] ps_user_name the USART user name. MUST be NOT NULL and NOT empty.
   * @param[in] baudrate     the baudrate to use.
   * @param[in] params       the USART configuration parameters.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool usart_open(USART *pv_usart, const char *ps_user_name, uint32_t baudrate, USARTParams params)
  {
    USART   *pv_u;
    uint32_t word_len, stop_bits, parity, mode;
    uint8_t  i;
    bool     res = false;

    // Check the user name.
    if(!ps_user_name || !*ps_user_name) { goto exit; }

    if(pv_usart->_is_opened)
    {
      res = (pv_usart->_baudrate == baudrate && pv_usart->_params == params);
      goto exit;
    }

    // Check that the hardware USART is not already in use.
    // Indeed 'logical' USARTs can share a same hardware USART.
    for(i = 0; i < USART_ID_COUNT; i++)
    {
      pv_u = &_usarts[i];
      if(pv_u == pv_usart || !_usarts_in_use_by[i].ps_name) { continue; }

      if(pv_usart->_uart.Instance == pv_u->_uart.Instance)
      {
	log_error(logger,
		  "Cannot open USART '%s', hardware USART already is used by '%s'.",
		  pv_usart->ps_name, _usarts_in_use_by[i].ps_name);
	goto exit;
      }
    }

    // Indicate that the USART is in use.
    _usarts_in_use_by[pv_usart->id].ps_name = ps_user_name;

    // Set values
    pv_usart->_baudrate = baudrate;
    pv_usart->_params   = params;

    // Set UART clock source
    usart_set_clock_source(pv_usart,
			   (params & USART_PARAM_RX_WHEN_ASLEEP) ?
			       PWRCLK_CLOCK_HSI16:
			       PWRCLK_CLOCK_PCLK1);

    // Enable USART clock
    usart_enable_clock(pv_usart, true);

    // Configure GPIOs

    usart_init_ios(pv_usart);

    // Convert UART parameters values to peripheral initialisation values
    switch(params & USART_PARAM_WORD_LEN_MASK)
    {
      case USART_PARAM_WORD_LEN_7BITS: word_len = UART_WORDLENGTH_7B; break;
      case USART_PARAM_WORD_LEN_8BITS: word_len = UART_WORDLENGTH_8B; break;
      case USART_PARAM_WORD_LEN_9BITS: word_len = UART_WORDLENGTH_9B; break;
      default: goto exit;
    }
    switch(params & USART_PARAM_STOP_BITS_MASK)
    {
      case USART_PARAM_STOP_BITS_0_5: stop_bits = UART_STOPBITS_0_5; break;
      case USART_PARAM_STOP_BITS_1:   stop_bits = UART_STOPBITS_1;   break;
      case USART_PARAM_STOP_BITS_1_5: stop_bits = UART_STOPBITS_1_5; break;
      case USART_PARAM_STOP_BITS_2:   stop_bits = UART_STOPBITS_2;   break;
      default: goto exit;
    }
    switch(params & USART_PARAM_PARITY_MASK)
    {
      case USART_PARAM_PARITY_NONE: parity = UART_PARITY_NONE; break;
      case USART_PARAM_PARITY_EVEN: parity = UART_PARITY_EVEN; break;
      case USART_PARAM_PARITY_ODD:  parity = UART_PARITY_ODD;  break;
      default: goto exit;
    }
    switch(params & USART_PARAM_MODE_MASK)
    {
      case USART_PARAM_MODE_RX:    mode = UART_MODE_RX;    break;
      case USART_PARAM_MODE_TX:    mode = UART_MODE_TX;    break;
      case USART_PARAM_MODE_RX_TX: mode = UART_MODE_TX_RX; break;
      default: goto exit;
    }

    // Configure peripheral
    pv_usart->_uart.Init.BaudRate               = baudrate;
    pv_usart->_uart.Init.WordLength             = word_len;
    pv_usart->_uart.Init.StopBits               = stop_bits;
    pv_usart->_uart.Init.Parity                 = parity;
    pv_usart->_uart.Init.Mode                   = mode;
    pv_usart->_uart.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    pv_usart->_uart.Init.OverSampling           = UART_OVERSAMPLING_16;
    pv_usart->_uart.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    pv_usart->_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    res = (HAL_UART_Init(&pv_usart->_uart) == HAL_OK);

    // If the USART has to be active when in stop mode and the baudrate is high enough,
    // Then the clock source must continuously be active when in stop mode.
    // It consumes more energy, but we do not have the choice.
    // The threshold baudrate can be precisely computed (application note AN4991).
    // Here we'll use a fixed value, so it's not uptimum, but it should be sufficient.
    if((params & USART_PARAM_RX_WHEN_ASLEEP) && baudrate >= 20000)
    {
      // Set CR3_UCESM bit, that is not defined in the STM32 header file for some reason.
      pv_usart->_uart.Instance->CR3 |=  (1u << 23);
    }
    else
    {
      // Clear CR3_UCESM bit.
      pv_usart->_uart.Instance->CR3 &= ~(1u << 23);
    }

    pv_usart->_is_opened = res;

    // Register to power mode changes if not already done.
    if(!_usart_pwrclk_listener_has_been_registered && (params & USART_PARAM_RX_WHEN_ASLEEP))
    {
      pwrclk_register_power_mode_change_listener(&_usart_pwrclk_listener);
      _usart_pwrclk_listener_has_been_registered = true;
    }
    exit:
    return res;
  }

  /**
   * Close a serial port.
   *
   * @param[in] pv_usart the USART object to close. MUST be NOT NULL.
   */
  void usart_close(USART *pv_usart)
  {
    if(pv_usart->_is_opened)
    {
      // Stop continuous read.
      usart_stop_it_read(pv_usart);

      // De-init UART
      HAL_UART_DeInit(&pv_usart->_uart);

      // Disable clock
      usart_enable_clock(pv_usart, false);

      // Free GPIOs
      usart_deinit_ios(pv_usart);

      // Indicate that the USART is free
      pv_usart->_is_opened                    = false;
      _usarts_in_use_by[pv_usart->id].ps_name = NULL;
    }
  }


  /**
   * Put a USART to sleep.
   *
   * Free it's GPIOs and gate the peripheral's clock.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   */
  void usart_sleep(USART *pv_usart)
  {
    if(pv_usart->_is_opened)
    {
      usart_enable_clock(pv_usart, false);
      usart_deinit_ios(  pv_usart);
    }
  }

  /**
   * Wake an USART up from sleep.
   *
   * If it was open before going to sleep then this reconfigure the GPIOs and
   * enable back the peripheral's clock.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   */
  void usart_wakeup(USART *pv_usart)
  {
    if(pv_usart->_is_opened)
    {
      // Set UART clock source
      usart_set_clock_source(pv_usart, pv_usart->_clock_source);
      usart_init_ios(        pv_usart);
      usart_enable_clock(    pv_usart, true);
    }
  }

  /**
   * Function called when the working power mode has changed.
   *
   * @param[in] current_mode  The power mode we are into.
   * @param[in] previous_mode The power mode we were before the change.
   */
  void _usart_pwrclk_listerner_cb(PwrClkPowerMode current_mode,
				  PwrClkPowerMode previous_mode)
  {
    uint8_t i;
    USART  *pv_usart;
    bool    enable;
    bool    do_something = false;

    if( current_mode >= PWRCLK_POWER_MODE_STOP0 &&
	current_mode <= PWRCLK_POWER_MODE_STOP1 &&
	previous_mode < PWRCLK_POWER_MODE_STOP0)
    {
      enable       = true;
      do_something = true;
    }
    else if(previous_mode >= PWRCLK_POWER_MODE_STOP0 &&
	previous_mode     <= PWRCLK_POWER_MODE_STOP1 &&
	current_mode       < PWRCLK_POWER_MODE_STOP0)
    {
      enable       = false;
      do_something = true;
    }
    if(!do_something) { goto exit; }

    for(i = 0; i < USART_ID_COUNT; i++)
    {
      pv_usart = &_usarts[i];
      if(pv_usart->_is_opened && (pv_usart->_params & USART_PARAM_RX_WHEN_ASLEEP))
      {
	if(enable) { HAL_UARTEx_EnableStopMode( &pv_usart->_uart); }
	else       { HAL_UARTEx_DisableStopMode(&pv_usart->_uart); }
      }
    }

    exit:
    return;
  }


#define usart_disable_read_its(pv_usart)  \
  (pv_usart)->_uart.Instance->CR1 &=      \
  ~(USART_CR1_EOBIE | USART_CR1_RTOIE | USART_CR1_CMIE | USART_CR1_RXNEIE | USART_CR1_IDLEIE); \
  (pv_usart)->_uart.Instance->CR2 &= ~(USART_CR2_LBDIE); \
  (pv_usart)->_uart.Instance->CR3 &= ~(USART_CR3_WUFIE | USART_CR3_CTSIE)

#define usart_clear_read_its(pv_usart)  \
  (pv_usart)->_uart.Instance->ICR |=  ( \
      USART_ICR_CMCF  | USART_ICR_WUCF   | USART_ICR_RTOCF | USART_ICR_CTSCF | \
      USART_ICR_LBDCF | USART_ICR_IDLECF | USART_ICR_ORECF | USART_ICR_NCF   | \
      USART_ICR_FECF  | USART_ICR_PECF   | USART_ICR_EOBCF)

  /**
   * Set the base information  for receiving data using interruptions.
   *
   * @param[in]  pv_usart   the USART object to read from. MUST be NOT NULL.
   * @param[in]  options    the interruption options to use.
   * @param[out] pu8_buffer the buffer where to write the data. MUST be NOT NULL.
   * @param[in]  size       the buffer's size.
   * @param[in]  pf_cb      the function called when the data have been received. Can be NULL.
   * @param[in]  pv_cb_args the argument to pass to the callback function. Can be NULL.
   */
  static void usart_set_it_rx_base(USART            *pv_usart,
				   USARTItOptions    options,
				   uint8_t          *pu8_buffer,
				   uint32_t          size,
				   USARTItRxCallback pf_cb,
				   void             *pv_cb_args)
  {
    IRQn_Type irqn;

    // Disable and clear interruptions
    usart_disable_read_its(pv_usart);
    usart_clear_read_its(  pv_usart);

    // Enable interruptions
    pv_usart->_uart.Instance->CR1 |= (USART_CR1_PEIE);
    pv_usart->_uart.Instance->CR3 |= (USART_CR3_EIE);

    // Compute Rx UART buffer mask
    UART_MASK_COMPUTATION(&pv_usart->_uart);

    // Set up irq handler context
    _usarts_irq_handler_infos[pv_usart->it.irq_handler_id].pv_usart = pv_usart;

    // Set up Read interruption context.
    pv_usart->it.options        = options;
    pv_usart->it.pu8_rx_buffer  = pu8_buffer;
    pv_usart->it.rx_buffer_size = size;
    pv_usart->it.rx_data_count  = 0;
    pv_usart->it.pf_rx_cb       = pf_cb;
    pv_usart->it.pv_rx_cb_args  = pv_cb_args;
    pv_usart->it.rx_state       = USART_IT_RX_STATE_IDLE;

    // Enable IRQ in NVIC
    irqn = _usart_irqn[pv_usart->it.irq_handler_id];
    HAL_NVIC_SetPriority(irqn, USART_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(  irqn);
  }


  /**
   * Stop current and continuous read for an USART.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   */
  void usart_stop_it_read(USART *pv_usart)
  {
    // Make sure the USART is set for interruptions
    if(_usarts_irq_handler_infos[pv_usart->it.irq_handler_id].pv_usart != pv_usart) { goto exit; }

    // Disable IRQ in NVIC
    HAL_NVIC_DisableIRQ(_usart_irqn[pv_usart->it.irq_handler_id]);

    // Disable and clear interruptions
    usart_disable_read_its(pv_usart);
    usart_clear_read_its(  pv_usart);

    // Clear IRQ handler context
    _usarts_irq_handler_infos[pv_usart->it.irq_handler_id].pv_usart = NULL;

    // Make sure that we cannot wakeup from stop mode
    HAL_UARTEx_DisableStopMode(&pv_usart->_uart);

    exit:
    return;
  }


  /**
   * Read data from a serial port.
   *
   * @param[in]  pv_usart   the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer the buffer where to write the data. MUST be NOT NULL.
   * @param[in]  size       the buffer's size.
   * @param[in]  timeout_ms the maximum time, in milliseconds, allowed to get the data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool usart_read(USART *pv_usart, uint8_t *pu8_buffer, uint16_t size, uint32_t timeout_ms)
  {
    return HAL_UART_Receive(&pv_usart->_uart, pu8_buffer, size, timeout_ms) == HAL_OK;
  }

  /**
   * Read data from a serial port. Non blocking; use interruptions.
   *
   * @param[in]  pv_usart   the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer the buffer where to write the data. MUST be NOT NULL.
   * @param[in]  size       the buffer's size.
   * @param[in]  continuous continuous reading (true), or single-shot (false)?
   * @param[in]  pf_cb      the function called when the data have been received. Can be NULL.
   * @param[in]  pv_cb_args the argument to pass to the callback function. Can be NULL.
   */
  void usart_read_it(USART            *pv_usart,
		     uint8_t          *pu8_buffer,
		     uint32_t          size,
		     bool              continuous,
		     USARTItRxCallback pf_cb,
		     void             *pv_cb_args)
  {
    if(!size) { goto exit; }

    usart_set_it_rx_base(pv_usart, USART_IT_RX, pu8_buffer, size, pf_cb, pv_cb_args);
    if(continuous) { pv_usart->it.options |= USART_IT_RX_CONTINUOUS; }

    // Start interruptions
    usart_process_rx_irq_for_usart(pv_usart);

    exit:
    return;
  }


  /**
   * Look for a start byte then copy a given number of bytes from the UART to a buffer.
   *
   * @param[in]  pv_usart       the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer     the buffer where the data are written to. MUST be NOT NULL.
   *                            The buffer MUST be big enough to receive all the bytes read from the UART,
   *                            plus the start byte if we are asked to copy it.
   * @param[in]  len            the number of bytes to read and copy from the UART to the buffer.
   *                            Does not include the start byte, only the following bytes.
   * @param[in]  from           the start byte to look for. The copy to the buffer will start as soon as
   *                            this start byte has been read from the UART.
   * @param[in]  copy_from_byte copy the from (start) byte to the buffer or not?
   * @param[in]  timeout_ms     the maximum time, in milliseconds, allowed to get all the data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool usart_read_from(USART   *pv_usart,
		       uint8_t *pu8_buffer,
		       uint16_t len,
		       uint8_t  from,
		       bool     copy_from_byte,
		       uint32_t timeout_ms)
  {
    uint8_t  b;
    uint32_t ref_ms, diff_ms;
    bool     res = false;

    // Look for the start byte
    ref_ms = board_ms_now();
    do
    {
      if(HAL_UART_Receive(&pv_usart->_uart, &b, 1, timeout_ms) != HAL_OK) { goto exit; }
      timeout_ms -= (diff_ms = board_ms_diff(ref_ms, board_ms_now())) > timeout_ms ? timeout_ms : diff_ms;
    }
    while(b != from);
    if(copy_from_byte) { *pu8_buffer++ = b; }

    // We have received the start byte; get the number of bytes requested.
    res = HAL_UART_Receive(&pv_usart->_uart, pu8_buffer, len, timeout_ms);

    exit:
    return res;
  }


  /**
   * Look for a start byte then copy a given number of bytes from the UART to a buffer.
   * Non blocking; use interruptions.
   *
   * @param[in]  pv_usart       the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer     the buffer where the data are written to. MUST be NOT NULL.
   *                            The buffer MUST be big enough to receive all the bytes read from the UART,
   *                            plus the start byte if we are asked to copy it.
   * @param[in]  len            the number of bytes to read and copy from the UART to the buffer.
   *                            Does not include the start byte, only the following bytes.
   * @param[in]  from           the start byte to look for. The copy to the buffer will start as soon as
   *                            this start byte has been read from the UART.
   * @param[in]  copy_from_byte copy the from (start) byte to the buffer or not?
   * @param[in]  continuous     continuous reading (true), or single-shot (false)?
   * @param[in]  pf_cb          the function called when the data have been received. Can be NULL.
   * @param[in]  pv_cb_args     the argument to pass to the callback function. Can be NULL.
   */
  void usart_read_from_it(USART            *pv_usart,
			  uint8_t          *pu8_buffer,
			  uint32_t          len,
			  uint8_t           from,
			  bool              copy_from_byte,
			  bool              continuous,
			  USARTItRxCallback pf_cb,
			  void             *pv_cb_args)
  {
    if(!len) { goto exit; }

    usart_set_it_rx_base(pv_usart,   USART_IT_RX | USART_IT_RX_FROM,
			 pu8_buffer, len,
			 pf_cb,      pv_cb_args);
    pv_usart->it.rx_from = from;
    if(copy_from_byte) { pv_usart->it.options |= USART_IT_RX_KEEP_BOUNDARIES; }
    if(continuous)     { pv_usart->it.options |= USART_IT_RX_CONTINUOUS;      }

    // Start interruptions
    usart_process_rx_irq_for_usart(pv_usart);

    exit:
    return;
  }


  /**
   * Read data from the UART and copy them to a buffer until we read a given stop byte.
   *
   * @param[in]  pv_usart     the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer   the buffer where the data are written to. MUST be NOT NULL.
   * @param[in]  size         the buffer's size.
   * @param[in]  to           the stop byte to look for.
   * @param[in]  copy_to_byte copy the to (stop) byte to the buffer or not?
   * @param[in]  timeout_ms   the maximum time, in milliseconds, allowed to get all the data.
   *
   * @return the number of bytes written to the buffer.
   * @return 0 in case of error or timeout.
   */
  uint16_t usart_read_to(USART   *pv_usart,
			 uint8_t *pu8_buffer,
			 uint16_t size,
			 uint8_t  to,
			 bool     copy_to_byte,
			 uint32_t timeout_ms)
  {
    uint8_t  b;
    uint32_t ref_ms, diff_ms;
    uint8_t *pu8     = pu8_buffer;
    uint8_t *pu8_end = pu8_buffer + size;

    // Read and look for the stop byte
    ref_ms = board_ms_now();
    while(1)
    {
      if(HAL_UART_Receive(&pv_usart->_uart, &b, 1, timeout_ms) != HAL_OK) { goto error_exit; }
      timeout_ms -= (diff_ms = board_ms_diff(ref_ms, board_ms_now())) > timeout_ms ? timeout_ms : diff_ms;

      if(b == to)
      {
	if(copy_to_byte)
	{
	  if(pu8 >= pu8_end) { goto error_exit; }
	  *pu8++  = b;
	}
	break;
      }
      else
      {
	if(pu8 >= pu8_end) { goto error_exit; }
	*pu8++  = b;
      }
    }

    return pu8 - pu8_buffer;

  error_exit:
    return 0;
  }


  /**
   * Read data from the UART and copy them to a buffer until we read a given stop byte.
   * Non blocking; use interruptions.
   *
   * @param[in]  pv_usart     the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer   the buffer where the data are written to. MUST be NOT NULL.
   * @param[in]  size         the buffer's size.
   * @param[in]  to           the stop byte to look for.
   * @param[in]  copy_to_byte copy the to (stop) byte to the buffer or not?
   * @param[in]  continuous   continuous reading (true), or single-shot (false)?
   * @param[in]  pf_cb        the function called when the data have been received. Can be NULL.
   * @param[in]  pv_cb_args   the argument to pass to the callback function. Can be NULL.
   */
  void usart_read_to_it(USART            *pv_usart,
			uint8_t          *pu8_buffer,
			uint32_t          size,
			uint8_t           to,
			bool              copy_to_byte,
			bool              continuous,
			USARTItRxCallback pf_cb,
			void             *pv_cb_args)
  {
    UART_WakeUpTypeDef wakeup_init;

    if(!size) { goto exit; }

    usart_set_it_rx_base(pv_usart,   USART_IT_RX | USART_IT_RX_TO,
			 pu8_buffer, size,
			 pf_cb,      pv_cb_args);
    pv_usart->it.rx_to = to;
    if(copy_to_byte) { pv_usart->it.options |= USART_IT_RX_KEEP_BOUNDARIES; }
    if(continuous)   { pv_usart->it.options |= USART_IT_RX_CONTINUOUS;      }

    // Configure wakeup from STOP; wake up if received something.
    wakeup_init.WakeUpEvent = UART_WAKEUP_ON_READDATA_NONEMPTY;
    HAL_UARTEx_StopModeWakeUpSourceConfig(&pv_usart->_uart, wakeup_init);

    // Start interruptions
    usart_process_rx_irq_for_usart(pv_usart);

    exit:
    return;
  }


  /**
   * Read from the UART and copy the data bytes that are between a start (from) and a stop (to) bytes.
   *
   * @param[in]  pv_usart        the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer      the buffer where the data are written to. MUST be NOT NULL.
   * @param[in]  size            the buffer's size.
   * @param[in]  from            the start byte to look for.
   * @param[in]  to              the end byte to look for.
   * @param[in]  copy_boundaries copy the start and stop bytes to the buffer?
   * @param[in]  timeout_ms      the maximum time, in milliseconds, allowed to get all the data.
   *
   * @return the number of bytes written to the buffer.
   * @return 0 in case of error or timeout.
   */
  uint16_t usart_read_from_to(USART   *pv_usart,
			      uint8_t *pu8_buffer,
			      uint16_t size,
			      uint8_t  from,
			      uint8_t  to,
			      bool     copy_boundaries,
			      uint32_t timeout_ms)
  {
    uint8_t  b;
    uint32_t ref_ms, diff_ms;
    uint8_t *pu8     = pu8_buffer;
    uint8_t *pu8_end = pu8_buffer + size;

    // Look for the start byte
    ref_ms = board_ms_now();
    do
    {
      if(HAL_UART_Receive(&pv_usart->_uart, &b, 1, timeout_ms) != HAL_OK) { goto error_exit; }
      timeout_ms -= (diff_ms = board_ms_diff(ref_ms, board_ms_now())) > timeout_ms ? timeout_ms : diff_ms;
    }
    while(b != from);
    if(copy_boundaries) { *pu8++ = b;      }
    if(pu8 >= pu8_end)  { goto error_exit; }

    // Look for the stop byte
    while(1)
    {
      if(HAL_UART_Receive(&pv_usart->_uart, &b, 1, timeout_ms) != HAL_OK) { goto error_exit; }
      timeout_ms -= (diff_ms = board_ms_diff(ref_ms, board_ms_now())) > timeout_ms ? timeout_ms : diff_ms;

      if(b == to)
      {
        if(copy_boundaries)
        {
  	if(pu8 >= pu8_end) { goto error_exit; }
  	*pu8++  = b;
        }
        break;
      }
      else
      {
        if(pu8 >= pu8_end) { goto error_exit; }
        *pu8++  = b;
      }
    }

    return pu8 - pu8_buffer;

    error_exit:
    return 0;
  }


  /**
   * Read from the UART and copy the data bytes that are between a start (from) and a stop (to) bytes.
   *
   * @param[in]  pv_usart        the USART object to read from. MUST be NOT NULL.
   * @param[out] pu8_buffer      the buffer where the data are written to. MUST be NOT NULL.
   * @param[in]  size            the buffer's size.
   * @param[in]  from            the start byte to look for.
   * @param[in]  to              the end byte to look for.
   * @param[in]  copy_boundaries copy the start and stop bytes to the buffer?
   * @param[in]  continuous      continuous reading (true), or single-shot (false)?
   * @param[in]  pf_cb           the function called when the data have been received. Can be NULL.
   * @param[in]  pv_cb_args      the argument to pass to the callback function. Can be NULL.
   */
  void usart_read_from_to_it(USART            *pv_usart,
			     uint8_t          *pu8_buffer,
			     uint16_t          size,
			     uint8_t           from,
			     uint8_t           to,
			     bool              copy_boundaries,
			     bool              continuous,
			     USARTItRxCallback pf_cb,
			     void             *pv_cb_args)
  {
    if(!size) { goto exit; }

    usart_set_it_rx_base(pv_usart,   USART_IT_RX | USART_IT_RX_FROM | USART_IT_RX_TO,
			 pu8_buffer, size,
			 pf_cb,      pv_cb_args);
    pv_usart->it.rx_from = from;
    pv_usart->it.rx_to   = to;
    if(copy_boundaries) { pv_usart->it.options |= USART_IT_RX_KEEP_BOUNDARIES; }
    if(continuous)      { pv_usart->it.options |= USART_IT_RX_CONTINUOUS;      }

    // Start interruptions
    usart_process_rx_irq_for_usart(pv_usart);

    exit:
    return;
  }


  /**
   * Write data to a serial port.
   *
   * @param[in]  pv_usart   the USART object to write to. MUST be NOT NULL.
   * @param[out] pu8_buffer the data to write. MUST be NOT NULL.
   * @param[in]  size       the amount of data bytes to write.
   * @param[in]  timeout_ms the maximum time, in milliseconds, allowed to write the data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool usart_write(USART *pv_usart, const uint8_t *pu8_data, uint16_t size, uint32_t timeout_ms)
  {
    return HAL_UART_Transmit(&pv_usart->_uart, (uint8_t *)pu8_data, size, timeout_ms) == HAL_OK;
  }

  /**
   * Write a string to the UART.
   *
   *@param[in]  pv_usart   the USART object to write to. MUST be NOT NULL.
   * @param[in] pu8_data   the data to write. MUST be NOT NULL. MUST be null terminated.
   * @param[in] timeout_ms the maximum time, in milliseconds, allowed to send all the data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool usart_write_string(USART *pv_usart, const char *ps, uint32_t timeout_ms)
  {
    return HAL_UART_Transmit(&pv_usart->_uart, (uint8_t *)ps, strlen(ps), timeout_ms) == HAL_OK;
  }


  /**
   * Process an interruption.
   *
   * @param[in] irq_id the identifier of the interruption request.
   */
  static void usart_process_irq_with_id(USARTIrqHandlerId irq_id)
  {
    USARTIrqHandlerInfos *pv_infos;
    USART                *pv_usart;

    // Get the IRQ infos from the identifier.
    pv_infos = &_usarts_irq_handler_infos[irq_id];

    // Check that an USART is registered for this IRQ; if not, there is problem.
    if(!(pv_usart = pv_infos->pv_usart)) { goto exit; }

    // Enable HSI if it is the USART's clock source.
    // This is to handle properly wakeup-from sleep when CR3->UCESM is 0.
    if(pv_usart->_clock_source == PWRCLK_CLOCK_HSI16) { __HAL_RCC_HSI_ENABLE(); }

    // Process Rx interruptions
    if(pv_usart->it.options & USART_IT_RX) { usart_process_rx_irq_for_usart(pv_usart); }

    exit:
    // Clear every interruption flag.
    usart_clear_read_its(pv_usart);
    return;
  }

  /**
   * Process Rx interruptions.
   *
   * @param[in] pv_uart the USART object. MUST be NOT NULL.
   */
  static void usart_process_rx_irq_for_usart(USART *pv_usart)
  {
    uint8_t  data;
    uint32_t isr_flags = pv_usart->_uart.Instance->ISR;
    bool     has_data     = false;
    bool     has_all_data = false;

    // Some error flags have been commented out because they are related to data noise.
    // Data communication can be OK, even if there is noise.
    if(isr_flags & (USART_ISR_PE | USART_ISR_ORE /*| USART_ISR_NE | USART_ISR_FE*/))
    {
      // Error
      // Go to idle
      pv_usart->it.rx_state = USART_IT_RX_STATE_IDLE;
    }
    if(isr_flags & USART_ISR_RXNE)
    {
      // Received data byte. BEWARE: does not work if data is 9 bits and no parity.
      data     = pv_usart->_uart.Instance->RDR;
      data    &= (uint8_t)pv_usart->_uart.Mask; // To remove parity bit.
      has_data = true;
    }

    switch(pv_usart->it.rx_state)
    {
      case USART_IT_RX_STATE_IDLE:
	// Disable and clear all Rx interruptions
	pv_usart->_uart.Instance->CR1 &= ~(USART_CR1_CMIE | USART_CR1_RXNEIE);
	usart_clear_read_its(pv_usart);

	// Reset data counter
	pv_usart->it.rx_data_count = 0;

	// Configure
	if(pv_usart->it.options & USART_IT_RX_FROM)
	{
	  // Configure interruption on character match on the 'from' byte.
	  __HAL_UART_DISABLE(  &pv_usart->_uart);
	  pv_usart->_uart.Instance->CR2 &= ~USART_CR2_ADD;
	  pv_usart->_uart.Instance->CR2 |= ((uint32_t)pv_usart->it.rx_from) << UART_CR2_ADDRESS_LSB_POS;
	  pv_usart->_uart.Instance->CR3 |=  USART_CR3_OVRDIS;   // Disable overrun error.
	  __HAL_UART_ENABLE(   &pv_usart->_uart);
	  __HAL_UART_ENABLE_IT(&pv_usart->_uart, UART_IT_CM);
	  pv_usart->it.rx_state = USART_IT_RX_STATE_WAITING_FOR_FROM;
	}
	else if(pv_usart->it.options & USART_IT_RX_TO)
	{
	  // Switch to USART_IT_RX_STATE_WAITING_FOR_TO state
	  __HAL_UART_DISABLE(  &pv_usart->_uart);
	  pv_usart->_uart.Instance->CR2 &= ~USART_CR2_ADD;
	  pv_usart->_uart.Instance->CR2 |= ((uint32_t)pv_usart->it.rx_to)   << UART_CR2_ADDRESS_LSB_POS;
	  __HAL_UART_ENABLE(   &pv_usart->_uart);
	  __HAL_UART_ENABLE_IT(&pv_usart->_uart, UART_IT_CM);
	  __HAL_UART_ENABLE_IT(&pv_usart->_uart, UART_IT_RXNE);
	  pv_usart->it.rx_state = USART_IT_RX_STATE_WAITING_FOR_TO;
	}
	else
	{
	  // Switch to USART_IT_RX_STATE_RECEIVING state.
	  __HAL_UART_ENABLE_IT(&pv_usart->_uart, UART_IT_RXNE);
	  pv_usart->it.rx_state = USART_IT_RX_STATE_RECEIVING;
	}
	break;


      case USART_IT_RX_STATE_WAITING_FOR_FROM:
	if(isr_flags & USART_ISR_CMF)
	{
	  // Copy 'from' byte?
	  if(pv_usart->it.options & USART_IT_RX_KEEP_BOUNDARIES)
	  {
	    usart_process_rx_irq_add_data(pv_usart, data);
	  }

	  // Disable UART
	  __HAL_UART_DISABLE(&pv_usart->_uart);
	  pv_usart->_uart.Instance->CR3 &= ~USART_CR3_OVRDIS;   // Enable overrun error.

	  // Switch state
	  if(pv_usart->it.options & USART_IT_RX_TO)
	  {
	    // Switch to USART_IT_RX_STATE_WAITING_FOR_TO state

	    pv_usart->_uart.Instance->CR2 &= ~USART_CR2_ADD;
	    pv_usart->_uart.Instance->CR2 |= ((uint32_t)pv_usart->it.rx_to) << UART_CR2_ADDRESS_LSB_POS;
	    __HAL_UART_ENABLE_IT(&pv_usart->_uart, UART_IT_RXNE);
	    pv_usart->it.rx_state = USART_IT_RX_STATE_WAITING_FOR_TO;
	  }
	  else
	  {
	    // Switch to USART_IT_RX_STATE_RECEIVING state
	    __HAL_UART_DISABLE_IT(&pv_usart->_uart, UART_IT_CM);
	    __HAL_UART_ENABLE_IT( &pv_usart->_uart, UART_IT_RXNE);
	    pv_usart->it.rx_state = USART_IT_RX_STATE_RECEIVING;
	  }

	  // Enable UART
	  __HAL_UART_ENABLE(&pv_usart->_uart);
	}
	break;


      case USART_IT_RX_STATE_WAITING_FOR_TO:
	if(isr_flags & USART_ISR_CMF)
	{
	  // We received the 'to' byte.
	  if(pv_usart->it.options & USART_IT_RX_KEEP_BOUNDARIES)
	  {
	    usart_process_rx_irq_add_data(pv_usart, data);
	  }

	  // We have all the requested data.
	  has_all_data = true;
	}
	else if(has_data)
	{
	  usart_process_rx_irq_add_data(pv_usart, data);
	}
      	break;


      case USART_IT_RX_STATE_RECEIVING:
	if(has_data)
	{
	  usart_process_rx_irq_add_data(pv_usart, data);

	  // See if we have all the requested data.
	  if(pv_usart->it.rx_data_count == pv_usart->it.rx_buffer_size) { has_all_data = true; }
	}
	break;

      default:  // This should not happen
	break;
    }

    // Do we have all the requested data?
    if(has_all_data)
    {
      // Call callback
      if(pv_usart->it.pf_rx_cb)
      {
	pv_usart->it.pf_rx_cb(pv_usart,
			      pv_usart->it.pu8_rx_buffer,
			      pv_usart->it.rx_data_count,
			      pv_usart->it.pv_rx_cb_args);
      }

      // Switch state to idle
      pv_usart->it.rx_state = USART_IT_RX_STATE_IDLE;

      // Continuous mode
      if(pv_usart->it.options & USART_IT_RX_CONTINUOUS)
      {
	// Prepare for next round.
	usart_process_rx_irq_for_usart(pv_usart);
      }
      else { usart_stop_it_read(pv_usart); }
    }
  }


  /**
   * Add current data byte to the Rx buffer.
   *
   * @param[in] pv_usart the USART object. MUST be NOT NULL.
   * @param[in] b        the data byte to add.
   */
  static void usart_process_rx_irq_add_data(USART *pv_usart, uint8_t b)
  {
    if(pv_usart->it.rx_data_count < pv_usart->it.rx_buffer_size)
    {
      // Copy data.
      pv_usart->it.pu8_rx_buffer[pv_usart->it.rx_data_count++] = b;
    }
    else
    {
      // Error, go back to idle.
      pv_usart->it.rx_state = USART_IT_RX_STATE_IDLE;
      usart_process_rx_irq_for_usart(pv_usart);
    }
  }


  void USART1_IRQHandler(void) { usart_process_irq_with_id(USART_IRQ_HANDLER_USART1); }
  void USART2_IRQHandler(void) { usart_process_irq_with_id(USART_IRQ_HANDLER_USART2); }
  void USART3_IRQHandler(void) { usart_process_irq_with_id(USART_IRQ_HANDLER_USART3); }
  void UART4_IRQHandler( void) { usart_process_irq_with_id(USART_IRQ_HANDLER_UART4);  }
  void UART5_IRQHandler( void) { usart_process_irq_with_id(USART_IRQ_HANDLER_UART5);  }


#ifdef __cplusplus
}
#endif
