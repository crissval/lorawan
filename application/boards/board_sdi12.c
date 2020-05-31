/**
 * @file  board.c
 * @brief Main board SDI-12 implementation file.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#include <gpio.h>
#include "board.h"
#include "powerandclocks.h"
#include "sdi12gen_manager.h"
//#include "sdi12manager.h"
#ifdef SDI12_BREAK_USE_SYSTICK
#include "stm32l4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define SDI12_UART_BAUDRATE       1200u
#define SDI12_UART_BREAK_NB_BITS  10u   ///< The break duration, in number of UART bits.


  static void board_sdi12_phy_init(                 SDI12PHY *p_phy);
  static void board_sdi12_phy_set_tx_to_break_level(SDI12PHY *p_phy,
						    void (*cb)(void *pv_args), void   *pv_cbargs);
  static void board_sdi12_phy_send_data(            SDI12PHY *p_phy,
						    uint8_t *pu8_data,         uint8_t len,
				                    void (*cb)(void *pv_args), void   *pv_cbargs);
  static bool board_sdi12_phy_receive_byte_by_byte( SDI12PHY *p_phy,
						    bool    (*cb)(uint8_t b,      void   *pv_args),
						    void     *pv_cbargs,
						    uint32_t  timeout_ms);
  static void board_sdi12_phy_stop_receiving(       SDI12PHY *p_phy);


  /**
   * The structure used handle the SID-12 UART.
   */
  typedef struct BoardSDI12UART
  {
    SDI12PHY           phy;     ///< The SDI-12 phy base
    USART_TypeDef     *p_regs;  ///< The UART's registers.
#ifdef SDI12_BREAK_USE_TIMER
    SDI12Timer     timer;   ///< The timer used by the UART
#endif
  }
  BoardSDI12UART;

  static bool _board_sdi12_has_been_initialised = false;

  /**
   * The SDI-12 interface.
   */
  static SDI12Interface sdi12;


  /// The board SDI-12 physical interface.
  static BoardSDI12UART sdi12_phy =
  {
      {
	  board_sdi12_phy_init,
	  NULL,
	  board_sdi12_phy_set_tx_to_break_level,
	  board_sdi12_phy_send_data,
	  board_sdi12_phy_receive_byte_by_byte,
	  board_sdi12_phy_stop_receiving
      }
  };


#define board_sdi12_uart_disable(   p_phy)  (p_phy)->p_regs->CR1 &= ~USART_CR1_UE
#define board_sdi12_uart_enable(    p_phy)  (p_phy)->p_regs->CR1 |=  USART_CR1_UE
#define board_sdi12_uart_is_enabled(p_phy) ((p_phy)->p_regs->CR1  &  USART_CR1_UE)



  /**
   * Initialise the SDI-12 features.
   *
   * @param[in] p_phy the physical interface to initalise. MUST be NOT NULL.
   */
  static void board_sdi12_phy_init(SDI12PHY *p_phy)
  {
    GPIO_TypeDef   *pv_rx_port, *pv_tx_port, *pv_dir_port;
    uint32_t        rx_pin,      tx_pin,      dir_pin;
    BoardSDI12UART *p_uart = (BoardSDI12UART *)p_phy;

    gpio_use_gpios_with_ids(SDI12_UART_RX_GPIO, SDI12_UART_TX_GPIO, SDI12_UART_TX_DIR_GPIO);
    pv_rx_port  = gpio_hal_port_and_pin_from_id(SDI12_UART_RX_GPIO, NULL);
    rx_pin      = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_RX_GPIO);
    pv_tx_port  = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_GPIO, NULL);
    tx_pin      = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_GPIO);
    pv_dir_port = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_DIR_GPIO, NULL);
    dir_pin     = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_DIR_GPIO);

    // Configure the GPIO mode for the UART TX, TX_DIR and RX.
    // Begin by configuring the pin as GPIO, not alternate functions.
    // By default switch PHY interface in RX mode.
    gpio_set_input_full( pv_rx_port, rx_pin, SDI12_UART_RX_GPIO_PULL);
    gpio_set_output_full(pv_tx_port, tx_pin,
			 GPIO_PUSH_PULL,
			 GPIO_PULL_NONE,
			 SDI12_UART_TX_GPIO_SPEED,
			 HIGH);
    gpio_set_output_full(pv_dir_port, dir_pin,
			 GPIO_PUSH_PULL,
			 GPIO_PULL_NONE,
			 SDI12_UART_TX_DIR_GPIO_SPEED,
			 SDI12_UART_TX_DIR_LEVEL_FOR_RX);
#if SDI12_UART >= 1 && SDI12_UART <= 3
    gpio_set_alternate_func(pv_rx_port, rx_pin, 7u);
    gpio_set_alternate_func(pv_tx_port, tx_pin, 7u);
#else
    gpio_set_alternate_func(pv_rx_port, rx_pin, 8u);
    gpio_set_alternate_func(pv_tx_port, tx_pin, 8u);
#endif
    gpio_set_alternate(pv_rx_port, rx_pin);
    gpio_set_alternate(pv_tx_port, tx_pin);

#ifdef SDI12_BREAK_USE_TIMER
    // Init the timer
    sdi12_timer_init(&p_uart->timer);
#endif

    // Set the UART clock
#if   SDI12_UART == 1
    RCC->CCIPR      &= ~RCC_CCIPR_USART3SEL_Msk;   // Set PCLK as the UART's clock.
    RCC->APB2ENR    |=  RCC_APB2ENR_USART1EN;      // Enable the UART clock.
    RCC->APB2SMENR  &= ~RCC_APB2SMENR_USART1SMEN;  // Clock gating during sleep and stop modes.
#elif SDI12_UART == 2
    RCC->CCIPR      &= ~RCC_CCIPR_USART2SEL_Msk;   // Set PCLK as the UART's clock.
    RCC->APB1ENR1   |=  RCC_APB1ENR1_USART2EN;     // Enable the UART clock.
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_USART2SMEN; // Clock gating during sleep and stop modes.
#elif SDI12_UART == 3
    RCC->CCIPR      &= ~RCC_CCIPR_USART3SEL_Msk;   // Set PCLK as the UART's clock.
    RCC->APB1ENR1   |=  RCC_APB1ENR1_USART3EN;     // Enable the UART clock.
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_USART3SMEN; // Clock gating during sleep and stop modes.
#elif SDI12_UART == 4
    RCC->CCIPR      &= ~RCC_CCIPR_UART4SEL_Msk;    // Set PCLK as the UART's clock.
    RCC->APB1ENR1   |=  RCC_APB1ENR1_UART4EN;      // Enable the UART clock.
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_UART4SMEN;  // Clock gating during sleep and stop modes.
#elif SDI12_UART == 5
    RCC->CCIPR      &= ~RCC_CCIPR_UART5SEL_Msk;    // Set PCLK as the UART's clock.
    RCC->APB1ENR1   |=  RCC_APB1ENR1_UART5EN;      // Enable the UART clock.
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_UART5SMEN;  // Clock gating during sleep and stop modes.
#endif

    // Initialise UART
#if SDI12_UART < 4
    p_uart->p_regs       =  PASTER2(USART, SDI12_UART);  // Set the registers to use.
#else
    p_uart->p_regs       =  PASTER2(UART,  SDI12_UART);  // Set the registers to use.
#endif
    board_sdi12_uart_disable(p_uart);         // Disable UART.
    p_uart->p_regs->CR1  =  USART_CR1_PCE;    // 7 bits + even parity (so 8 bits for STM32's UART), 16x oversampling.
    p_uart->p_regs->CR2  =  USART_CR2_TXINV;  // LSB first, 1 stop bit, use inverse logic (1=L, 0=H) for TX.
    p_uart->p_regs->CR3  =  0x00000000;
#if SDI12_UART == 1
    p_uart->p_regs->BRR  = (uint16_t)(pwrclk_clock_frequency_hz(PWRCLK_CLOCK_PCLK2) / SDI12_UART_BAUDRATE);  // This formula because 16x oversampling;
#else
    p_uart->p_regs->BRR  = (uint16_t)(pwrclk_clock_frequency_hz(PWRCLK_CLOCK_PCLK1) / SDI12_UART_BAUDRATE);  // This formula because 16x oversampling
#endif
  }

  /**
   * Send a break.
   *
   * @param[in] p_phy     the physical interface. MUST be NOT NULL.
   * @param[in] cb        the function to call at the end of the break.  Can be NULL.
   * @param[in] pv_cbargs the argument to pass to the callback function. Can be NULL.
   */
  static void board_sdi12_phy_set_tx_to_break_level(SDI12PHY *p_phy,
						    void (*cb)(void *pv_args), void *pv_cbargs)
  {
#if defined SDI12_BREAK_USE_TIMER || defined SDI12_BREAK_USE_SYSTICK
    GPIO_TypeDef   *pv_tx_port;
    uint32_t        tx_pin;
#endif
    GPIO_TypeDef   *pv_dir_port;
    uint32_t        dir_pin;
    BoardSDI12UART *p_uart = (BoardSDI12UART *)p_phy;

#if defined SDI12_BREAK_USE_TIMER || defined SDI12_BREAK_USE_SYSTICK
    pv_tx_port  = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_GPIO, NULL);
    tx_pin      = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_GPIO);
#endif
    pv_dir_port = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_DIR_GPIO, NULL);
    dir_pin     = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_DIR_GPIO);

#ifdef SDI12_BREAK_USE_TIMER
    // Configure the interface for TX mode. Use TX as a GPIO, not controlled by the UART anymore.
    gpio_set_output(pv_tx_port,  tx_pin, HIGH);
    gpio_set_level( pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_TX);

    // Disable the UART
    board_sdi12_uart_disable(p_uart);

    // Start the timer
    sdi12_timer_start(&p_uart->timer, SDI12_BREAK_DURATION_MS, cb, pv_cbargs);
#elif defined SDI12_BREAK_USE_SYSTICK
    // Configure the interface for TX mode. Use TX as a GPIO, not controlled by the UART anymore.
    gpio_set_output(pv_tx_port,  tx_pin, HIGH);
    gpio_set_level( pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_TX);

    // Disable the UART
    board_sdi12_uart_disable(p_uart);

    // Wait for break duration
    HAL_Delay(SDI12_BREAK_DURATION_MS);

    // Go back to idle for a little while to be sure that the start bit is detected
    gpio_set_output(pv_tx_port, tx_pin, LOW);
    HAL_Delay(1);

    // Call the callback
    if(cb) { cb(pv_cbargs); }
#else  // SDI12_BREAK_USE_TIMER
    // We will use the UART's break functionnality.
    // The break duration must be of at least 12ms but at 1200 bauds the break
    // generated by the UART is only 8ms long.
    // So we will reduce the baudrate while we send the break to get the break duration right.
    uint32_t brr_bak = p_uart->p_regs->BRR;
    uint32_t brr     = (SDI12_UART_BREAK_NB_BITS * 1000) / SDI12_BREAK_DURATION_MS;

    // Disable the UART
    board_sdi12_uart_disable(p_uart);

    // Update the baud rate register value
    p_uart->p_regs->BRR = (uint16_t)((brr_bak * ((uint32_t)SDI12_UART_BAUDRATE)) / brr);

    // Configure interface driver for TX
    gpio_set_level(pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_TX);

    // Enable the UART
    board_sdi12_uart_enable(p_uart);
    p_uart->p_regs->CR1 |= USART_CR1_TE;  // Enable transmission

    // Send the break
    p_uart->p_regs->RQR |= USART_RQR_SBKRQ;

    // Wait for the end of the break
    while(p_uart->p_regs->ISR & USART_ISR_SBKF) /* Do nothing */;

    // The interrupt flag is reset during the stop bit of the break transmission.
    // It doesn't say at the end of the stop bit.
    // so maybe we should wait some extra time to be sure that all the
    // stop bits have been transmitted before changing back the baudrate.

    // Revert to original baudrate
    p_uart->p_regs->BRR = brr_bak;

    // Well, I've had to add this delay for the SMT100 sensors.
    // It seems that for those sensors the post-break '1' was too short, it did not respond to commands.
    board_delay_ms(1);

    // Call the callback
    if(cb) { cb(pv_cbargs); }
#endif // SDI12_BREAK_USE_TIMER
  }

  /**
   * Send data through the UART.
   *
   * @post After calling this function, the interface is still configured for transmission
   *       (TX_DIR pin is at TX level).
   *
   * @param[in] p_phy     the physical interface. MUST be NOT NULL.
   * @param[in] pu8_data  the data to send. MUST be NOT NULL.
   * @param[in] len       the number of data bytes to send.
   * @param[in] cb        the function to call when all the data have been sent. Can be NULL.
   * @param[in] pv_cbargs the arguments to pass to the callback function. Can be NULL.
   */
  static void board_sdi12_phy_send_data(SDI12PHY *p_phy,
					uint8_t *pu8_data,         uint8_t len,
					void (*cb)(void *pv_args), void   *pv_cbargs)
  {
#if defined SDI12_BREAK_USE_TIMER || defined SDI12_BREAK_USE_SYSTICK
    GPIO_TypeDef   *pv_tx_port;
    uint32_t        tx_pin;
#endif
    GPIO_TypeDef   *pv_dir_port;
    uint32_t        dir_pin;
    BoardSDI12UART *p_uart = (BoardSDI12UART *)p_phy;

    // Configure the interface for TX mode
    pv_dir_port = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_DIR_GPIO, NULL);
    dir_pin     = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_DIR_GPIO);
    gpio_set_level(pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_TX);

#if defined SDI12_BREAK_USE_TIMER || defined SDI12_BREAK_USE_SYSTICK
    // Give back control of the TX GPIO to the UART
    pv_tx_port  = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_GPIO, NULL);
    tx_pin      = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_GPIO);
    gpio_set_alternate(pv_tx_port, tx_pin);
#endif

    // Enable the UART
    board_sdi12_uart_enable(p_uart);
    p_uart->p_regs->CR1 |= USART_CR1_TE;

#ifdef SDI12_USE_DMA
    // TODO: implement me. use DMA to send data.
#else
    for(; len; len--, pu8_data++)
    {
      // Wait for the transmission buffer to be empty
      while(!(p_uart->p_regs->ISR & USART_ISR_TXE)) /* Do nothing */;

      // Set next byte to transmit in transmission buffer
      p_uart->p_regs->TDR = *pu8_data;
    }

    // Wait for end of transmission
    while(!(p_uart->p_regs->ISR & USART_ISR_TC)) /* Do nothing */;

    // Transmission is done; it's time to call the callback function
    if(cb) { cb(pv_cbargs); }
#endif
  }


  /**
   * Receive data from the UART. And submit each byte received to a callback function.
   *
   * @post the UART configuration is kept in receiving mode.
   *
   * @param[in] p_phy      the physical interface. MUST be NOT NULL.
   * @param[in] cb         the callback function. MUST be NOT NULL.
   *                       the function returns true when we have to keep receiving and
   *                       false when we have to stop.
   * @param[in] pv_cbargs  the custom arguments to pass to the callback function. Can be NULL.
   * @param[in] timeout_ms the maximum time, in seconds, to get all the data we need.
   *                       A value of 0 does not disable the timeout.
   *
   * @return true  on success.
   * @return false on timeout.
   */
  static bool board_sdi12_phy_receive_byte_by_byte(SDI12PHY *p_phy,
						   bool    (*cb)(uint8_t b, void *pv_args),
						   void     *pv_cbargs,
						   uint32_t  timeout_ms)
  {
    GPIO_TypeDef   *pv_dir_port;
    uint32_t        dir_pin;
    uint32_t        reception_flag;
    uint8_t         b;
    BoardSDI12UART *p_uart   = (BoardSDI12UART *)p_phy;
    uint32_t        start_ts = board_ms_now();
    bool            ok       = true;

    // Configure the interface for reception
    pv_dir_port = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_DIR_GPIO, NULL);
    dir_pin     = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_DIR_GPIO);
    gpio_set_level(pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_RX);

    // Make sure that the UART is enabled
    board_sdi12_uart_enable(p_uart);
    p_uart->p_regs->CR1 |= USART_CR1_RE; // Enable reception.

    // Wait for a data byte to arrive and process it.
    while(board_sdi12_uart_is_enabled(p_uart))
    {
      // Wait for a data byte
      while(board_sdi12_uart_is_enabled(p_uart)                       &&
	  (ok              = !board_is_timeout(start_ts, timeout_ms)) &&
	  !(reception_flag = (p_uart->p_regs->ISR & USART_ISR_RXNE))); // Do nothing

      if(reception_flag)
      {
	// We received a byte; give it to the callback function
	// The mask operation is needed to eliminate the parity bit from the data buffer.
	b = ((uint8_t)p_uart->p_regs->RDR) & (uint8_t)0x7F;
	if(!cb(b, pv_cbargs)) { break; }  // Stop receiving
      }
      else { break; } // The UART has been disabled, or timeout; stop receiving.
    }

    return ok;
  }


  /**
   * Stop receiving data through the UART.
   * In fact, we disable the UART.
   *
   * @post the UART interface is configured in reception mode (TX_DIR).
   *
   * @param[in] p_phy the physical interface. MUST be NOT NULL.
   */
  static void board_sdi12_phy_stop_receiving(SDI12PHY *p_phy)
  {
    GPIO_TypeDef   *pv_dir_port;
    uint32_t        dir_pin;
    BoardSDI12UART *p_uart = (BoardSDI12UART *)p_phy;

    // Just disable the UART; it will stop it from receiving.
    board_sdi12_uart_disable(p_uart);
    p_uart->p_regs->CR1 &= ~USART_CR1_RE; // Disable reception.

    // Keep the interface in reception mode
    pv_dir_port = gpio_hal_port_and_pin_from_id(SDI12_UART_TX_DIR_GPIO, NULL);
    dir_pin     = GPIO_PIN_ID_FROM_GPIO_ID(SDI12_UART_TX_DIR_GPIO);
    gpio_set_level(pv_dir_port, dir_pin, SDI12_UART_TX_DIR_LEVEL_FOR_RX);
  }

  /**
   * Initialises the SDI-12 interface(s)
   */
  void board_sdi12_init(void)
  {
    if(!_board_sdi12_has_been_initialised)
    {
      sdi12_gen_mgr_init();
      sdi12_gen_mgr_register_interface(SDI12_INTERFACE_NAME, &sdi12, (SDI12PHY *)&sdi12_phy);
      _board_sdi12_has_been_initialised = true;
    }
  }


#ifdef __cplusplus
}
#endif
