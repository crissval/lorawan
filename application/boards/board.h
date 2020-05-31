/**
 * @file  board.h
 * @brief Main board header file.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __BOARD_H__
#define __BOARD_H__

// The board uses a STM32L476
//#define STM32L476xx

#include "stm32l4xx.h"
#include "defs.h"
#include "config.h"
#include "gpio.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the flags identifying for the controllable power supplies
   */
  typedef enum BoardPowerFlag
  {
    BOARD_POWER_NONE             = 0,
    BOARD_POWER_INTERNAL_SENSORS = 1u << 1,             ///< Power supply for internal sensors.
    BOARD_POWER_EXT_ADJ          = 1u << 2,             ///< Adjustable power supply for external sensors.
    BOARD_POWER_EXT_INT          = 1u << 3,             ///< Power to the interruption lines for the external sensors.
    BOARD_POWER_SDI12            = BOARD_POWER_EXT_INT  ///< Power for the SDI-12 interface.
  }
  BoardPowerFlag;
  typedef uint8_t BoardPower;  ///< Defines a controllable power configuration. A ORed combination of BoardPowerFlag values.

  /**
   * Defines the reset cause flags.
   */
  typedef enum BoardResetFlag
  {
    BOARD_RESET_FLAG_NONE                 = 0,
    BOARD_RESET_FLAG_BOR                  = 1 << 0,  ///< BOR reset
    BOARD_RESET_FLAG_PIN                  = 1 << 1,  ///< NRST pin
    BOARD_RESET_FLAG_SOFTWARE             = 1 << 2,  ///< Software request
    BOARD_RESET_FLAG_SOFTWARE_ERROR       = 1 << 3,  ///< Software request because of error.
    BOARD_RESET_FLAG_WATCHDOG             = 1 << 4,  ///< Watchdog
    BOARD_RESET_FLAG_INDEPENDENT_WATCHDOG = 1 << 5,  ///< Independent watchdog
    BOARD_RESET_FLAG_LOW_PWR_ERROR        = 1 << 6,  ///< Low power mode usage error
    BOARD_RESET_FLAG_OPTION_BYTES_LOADING = 1 << 7,  ///< Option bytes loading
    BOARD_RESET_FLAG_FIREWALL             = 1 << 8   ///< Firewall
  }
  BoardResetFlag;
  typedef uint32_t BoardResetFlags;  ///< Type used to store a ORed combination of #BoardResetFlag values.

  /**
   * Defines the type of software resets.
   */
  typedef enum BoardSoftwareResetType
  {
    BOARD_SOFTWARE_RESET_OK,      ///< Normal software reset request.
    BOARD_SOFTWARE_RESET_ERROR,   ///< Software reset request because of an error.
  }
  BoardSoftwareResetType;


#define board_delay_ms(ms)            HAL_Delay(ms)
#define board_ms_now()                HAL_GetTick()
#define board_ms_diff(   ref_ms, ms)  ((ms) - (ref_ms))
#define board_is_timeout(ref_ms, timeout_ms)  \
  (board_ms_diff(ref_ms, board_ms_now()) >= timeout_ms)


  extern void board_init(void);
  extern void board_sdi12_init(void);
  extern void board_watchdog_reset(void);

  extern void board_set_power(BoardPower power);
  extern void board_add_power(BoardPower power);

  extern void            board_reset(BoardSoftwareResetType type);
  extern BoardResetFlags board_reset_source_flags(bool clear);
  extern void            board_reset_source_clear(void);


  /*==================== Error handling =======================*/
#define fatal_error_handler(ps_file, line, ps_func_name, ps_msg, ...) // Do nothing
#include "error.h"


  /*=================== Clocks configuration ====================*/
//#define HSI_FREQ_HZ    16000000u
//#define LSI_FREQ_HZ    32000u
#define LSE_FREQ_HZ    32768u
//#define SYSCLK_FREQ_HZ 80000000u
//#define HCLK_FREQ_HZ   80000000u
//#define PCLK1_FREQ_HZ  20000000u  // Use fixed PCLK1 frequency
//#define PCLK2_FREQ_HZ  20000000u  // Use fixed PCLK2 frequency

#define LSE_DRIVE_LEVEL  RCC_LSEDRIVE_HIGH


  //==================== Power ===================================
#define VDDA_VOLTAGE_MV  3300


  //==================== JTAG ====================================
#define JTAG_SWDIO_GPIO   GPIO_PA13
#define JTAG_SWDCLK_GPIO  GPIO_PA14


  //==================== Power control ===========================
#define ENABLE_PWR_INTERNAL_GPIO      GPIO_PG15
#define STATE_PWR_INTERNAL_GPIO       GPIO_PB11
#define ENABLE_PWR_EXTERNAL_GPIO      GPIO_PG0
#define STATE_PWR_EXTERNAL_GPIO       GPIO_PC3
#define ENABLE_PWR_EXTINTERRUPT_GPIO  GPIO_PF3
#define STATE_PWR_EXTINTERRUPT_GPIO   GPIO_PG8


  //==================== SD Card =================================
#define SDCARD_DMA_RX_NUM           2
#define SDCARD_DMA_TX_NUM           2
#define SDCARD_DMA_RX_CHANNEL_NUM   4
#define SDCARD_DMA_TX_CHANNEL_NUM   4
#define SDCARD_D0_GPIO              GPIO_PC8
#define SDCARD_D1_GPIO              GPIO_PC9
#define SDCARD_D2_GPIO              GPIO_PC10
#define SDCARD_D3_GPIO              GPIO_PC11
#define SDCARD_CLK_GPIO             GPIO_PC12
#define SDCARD_CMD_GPIO             GPIO_PD2
#define SDCARD_GPIOS_ALTERNATE      GPIO_AF12_SDMMC1
#define SDCARD_HAS_EXTERNAL_PULLUPS
//#define SDCARD_DETECT_GPIO
//#define SDCARD_DETECT_PULL
//#define SDCARD_DETECT_ACTIVE_LEVEL
//#define SDCARD_USE_DETECT


  //==================== Wakeup lines ============================
#define WAKEUP_EXT_SENSOR_GPIO 			GPIO_PA0
#define WAKEUP_EXT_SENSOR_EDGE_SENSITIVITY      GPIO_RISING_EDGE
#define WAKEUP_EXT_SENSOR_PULL                  GPIO_PULL_NONE
#define WAKEUP_EXT_SENSOR_WKUP_PIN              PWR_WAKEUP_PIN1_HIGH
#define	WAKEUP_INT_SENSOR_GPIO			GPIO_PC13
#define WAKEUP_INT_SENSOR_EDGE_SENSITIVITY      GPIO_RISING_EDGE
#define WAKEUP_INT_SENSOR_PULL                  GPIO_PULL_NONE
#define WAKEUP_INT_SENSOR_WKUP_PIN              PWR_WAKEUP_PIN2_HIGH
#define WAKEUP_USB_GPIO 			GPIO_PE6
#if defined CONNECSENS_PROTO1 || defined CONNECSENS_PROTO2
#define WAKEUP_USB_EDGE_SENSITIVITY             GPIO_FALLING_EDGE
#define WAKEUP_USB_WKUP_PIN                     PWR_WAKEUP_PIN3_LOW
#else
#define WAKEUP_USB_EDGE_SENSITIVITY             GPIO_RISING_EDGE
#define WAKEUP_USB_WKUP_PIN                     PWR_WAKEUP_PIN3_HIGH
#endif
#define WAKEUP_USB_PULL                         GPIO_PULL_NONE


  //=================== Internal interruption lines ==============
  // BEWARE: The schematics of the main board is wrong;
  // the pinout of the connector for the sensor board is wrong
  // You have to use the pinout of the connector on the sensors board's schematics.
#define LIS3DH_INTERRUPT_GPIO    GPIO_PB8
#define LPS25_INTERRUPT_GPIO	   GPIO_PB9
#define OPT3001_INTERRUPT_GPIO   GPIO_PE1
#define SHT35_INTERRUPT_GPIO	   GPIO_PE0
  // Pin used to clear the interruption memories
#define SENSOR_INT_CLEAR_INTERRUPT_GPIO  GPIO_PE2
#define SENSOR_INT_CLEAR_INTERRUPT_CLEAR_LEVEL   LEVEL_LOW


  //=================== External interruption lines ==============
#define SPI_INTERRUPT_GPIO    GPIO_PD12
#define I2C_INTERRUPT_GPIO    GPIO_PF2
#define USART_INTERRUPT_GPIO  GPIO_PD11   // Beware there is a schematics problem:
#define SDI12_INTERRUPT_GPIO  GPIO_PD4    // UART and SDI12 interrupts pin are exchanged.
#define OPTO1_INTERRUPT_GPIO  GPIO_PF12
#define OPTO1_INPUT_GPIO      GPIO_PF5    // On the schematics OPTO_1 and OPTO_2 are inverted
#define OPTO2_INTERRUPT_GPIO  GPIO_PF13
#define OPTO2_INPUT_GPIO      GPIO_PF4    // On the schematics OPTO_1 and OPTO_2 are inverted
#define INT1_INTERRUPT_GPIO   GPIO_PF14
#define INT2_INTERRUPT_GPIO   GPIO_PF15
  // Pin used to clear the interruption memories
#define SENSOR_EXT_CLEAR_INTERRUPT_GPIO         GPIO_PD14
#define SENSOR_EXT_CLEAR_INTERRUPT_CLEAR_LEVEL  LEVEL_LOW


  //=================== Internal LEDs ===========================
#define LED1_DEBUG_GPIO		GPIO_PE14
#define LED1_DEBUG_ACTIVE_LEVEL	LEVEL_HIGH
#define LED2_DEBUG_GPIO 		GPIO_PE13
#define LED2_DEBUG_ACTIVE_LEVEL	LEVEL_HIGH
#define LED1_USER_GPIO		GPIO_PE3
#define LED1_USER_ACTIVE_LEVEL	LEVEL_HIGH
#define LED2_USER_GPIO 		GPIO_PE4
#define LED2_USER_ACTIVE_LEVEL	LEVEL_HIGH


  //==================== I2C configuration =======================

#define I2C_INTERNAL_ID               1
#define I2C_INTERNAL_TIMING           0x20420F13  // 100 kHz
#define I2C_INTERNAL_FILTER_ANALOG    I2C_ANALOGFILTER_DISABLE
#define I2C_INTERNAL_FILTER_DIGITAL   0
#define I2C_INTERNAL_SDA_GPIO         GPIO_PG13
#define I2C_INTERNAL_SDA_AF           GPIO_AF4_I2C1
#define I2C_INTERNAL_SCL_GPIO         GPIO_PG14
#define I2C_INTERNAL_SCL_AF           GPIO_AF4_I2C1

#define I2C_EXTERNAL_ID               2
#define I2C_EXTERNAL_TIMING           0x20426064  // 20 kHz
#define I2C_EXTERNAL_FILTER_ANALOG    I2C_ANALOGFILTER_DISABLE
#define I2C_EXTERNAL_FILTER_DIGITAL   0
#define I2C_EXTERNAL_SDA_GPIO         GPIO_PF0
#define I2C_EXTERNAL_SDA_AF           GPIO_AF4_I2C2
#define I2C_EXTERNAL_SCL_GPIO         GPIO_PF1
#define I2C_EXTERNAL_SCL_AF           GPIO_AF4_I2C2


  //==================== ADC configuration =======================
#define ADC_ANA1_GPIO              GPIO_PC0
#define ADC_ANA1_ADC_ID            1
#define ADC_ANA1_ADC_CHANNEL_ID    1
#define ADC_ANA2_GPIO              GPIO_PC1
#define ADC_ANA2_ADC_ID            1
#define ADC_ANA2_ADC_CHANNEL_ID    2
#define ADC_BATV_GPIO              GPIO_PC2
#define ADC_BATV_ADC_ID            1
#define ADC_BATV_ADC_CHANNEL_ID    3
#define ADC_REFINT_ADC_ID          1
#define ADC_REFINT_ADC_CHANNEL_ID  0


  //==================== GPIO pins on extension port =============
#define EXT_GPIO1_GPIO  GPIO_PG2
#define EXT_GPIO2_GPIO  GPIO_PG3
#define EXT_GPIO3_GPIO  GPIO_PG4
#define EXT_GPIO4_GPIO  GPIO_PG5
#define EXT_GPIO5_GPIO  GPIO_PG6


  //==================== SPI on extension port ===================
#define EXT_SPI_ID              2
#define EXT_SPI_CS_GPIO         GPIO_PB12
#define EXT_SPI_MISO_GPIO       GPIO_PB14
#define EXT_SPI_MISO_AF         GPIO_AF5_SPI2
#define EXT_SPI_MOSI_GPIO       GPIO_PB15
#define EXT_SPI_MOSI_AF         GPIO_AF5_SPI2
#define EXT_SPI_CLK_GPIO        GPIO_PB13
#define EXT_SPI_CLK_AF          GPIO_AF5_SPI2
#define EXT_SPI_DMA_RX_ID       1
#define EXT_SPI_DMA_RX_CHANNEL  4
#define EXT_SPI_DMA_RX_REQUEST  1
#define EXT_SPI_DMA_TX_ID       1
#define EXT_SPI_DMA_TX_CHANNEL  5
#define EXT_SPI_DMA_TX_REQUEST  1


  //==================== USART on extension port =================
#define EXT_USART_ID                  2
#define EXT_USART_TX_GPIO             GPIO_PD5
#define EXT_USART_TX_AF               GPIO_AF7_USART2
#define EXT_USART_TX_DMA_NUM          1
#define EXT_USART_TX_DMA_CHANNEL_NUM  7
#define EXT_USART_RX_GPIO             GPIO_PD6
#define EXT_USART_RX_AF               GPIO_AF7_USART2
#define EXT_USART_RX_DMA_NUM          1
#define EXT_USART_RX_DMA_CHANNEL_NUM  6

  //==================== USART for SigFox =================
#define SIGFOX_USART_ID                  1
#define SIGFOX_USART_TX_GPIO             GPIO_PB6
#define SIGFOX_USART_TX_AF               GPIO_AF7_USART1
#define SIGFOX_USART_TX_DMA_NUM          1
#define SIGFOX_USART_TX_DMA_CHANNEL_NUM  4
#define SIGFOX_USART_RX_GPIO             GPIO_PB7
#define SIGFOX_USART_RX_AF               GPIO_AF7_USART1
#define SIGFOX_USART_RX_DMA_NUM          1
#define SIGFOX_USART_RX_DMA_CHANNEL_NUM  5


  /*==================== SDI-12 configuration ====================*/
extern void board_sdi12_init(void);

#define SDI12_UART_TX_GPIO              GPIO_PD8
#define SDI12_UART_TX_GPIO_SPEED        GPIO_SPEED_LOW
#define SDI12_UART_RX_GPIO              GPIO_PD9
#define SDI12_UART_RX_GPIO_PULL         GPIO_PULL_NONE
#define SDI12_UART_TX_DIR_GPIO          GPIO_PD10
#define SDI12_UART_TX_DIR_GPIO_SPEED    GPIO_SPEED_LOW
#ifdef CONNECSENS_PROTO1
#define SDI12_UART_TX_DIR_LEVEL_FOR_TX  LOW
#define SDI12_UART_TX_DIR_LEVEL_FOR_RX  HIGH
#else
#define SDI12_UART_TX_DIR_LEVEL_FOR_TX  HIGH
#define SDI12_UART_TX_DIR_LEVEL_FOR_RX  LOW
#endif

#define USE_UART3
#define UART3_ALIAS sdi12_uart
#define SDI12_UART  3  ///< The id of the UART used by the SDI-12 interface.
//#define SDI12_BREAK_USE_TIMER
//#define SDI12_BREAK_USE_SYSTICK


/*==================== LoRa configuration ====================*/
#define LORA_SPI_ID            1
#define LORA_RESET_GPIO        GPIO_PB2
#define LORA_MOSI_GPIO         GPIO_PA7
#define LORA_MOSI_AF           GPIO_AF5_SPI1
#define LORA_MISO_GPIO         GPIO_PA6
#define LORA_MISO_AF           GPIO_AF5_SPI1
#define LORA_SCK_GPIO          GPIO_PA5
#define LORA_SCK_AF            GPIO_AF5_SPI1
#define LORA_NSS_GPIO          GPIO_PA4
#define LORA_DIO0_GPIO         GPIO_PB1
#define LORA_DIO1_GPIO         GPIO_PB0
#define LORA_DIO2_GPIO         GPIO_PC5
#define LORA_DIO3_GPIO         GPIO_PC4
#define LORA_PWR_GPIO          GPIO_PF11
#define LORA_PWR_LEVEL_FOR_ON  HIGH


//=================== GPS configuration ======================
#define GPS_A2135_USART_ID    1
#define GPS_A2135_USART_AF    GPIO_AF7_USART1
#define GPS_A2135_RX_GPIO     GPIO_PG10
#define GPS_A2135_TX_GPIO     GPIO_PG9
#define GPS_A2135_PPS_GPIO    GPIO_PG11
#define GPS_A2135_PWR_GPIO    GPIO_PD7
#define GPS_A2135_RST_GPIO    GPIO_PG12
#define GPS_A2135_ONOFF_GPIO  GPIO_PD3
#define GPS_A2135_WKUP_GPIO   GPIO_PE15
#define GPS_TIMEOUT_BLOCKING_FIX_SEC  600  // 10 minutes for blocking fix.
#define GPS_TIMEOUT_FIX_SEC           500  // 5 minutes for periodic fix.


#ifdef __cplusplus
}
#endif
#endif /* __BOARD_H__ */
