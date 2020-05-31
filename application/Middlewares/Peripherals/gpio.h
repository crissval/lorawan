/**
 * @brief Header file to use the STM32 GPIOs.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __GPIO_H__
#define __GPIO_H__

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GPIOHALRegs  GPIO_TypeDef
#define GPIOHALPin   uint16_t


  /**
   * Define the port identifiers
   */
  typedef enum GPIOPortId
  {
    GPIO_PORT_ID_NONE,
    GPIO_PORT_ID_A,
    GPIO_PORT_ID_B,
    GPIO_PORT_ID_C,
    GPIO_PORT_ID_D,
    GPIO_PORT_ID_E,
    GPIO_PORT_ID_F,
    GPIO_PORT_ID_G,
    GPIO_PORT_ID_H,
    GPIO_PORT_ID_I
  }
  GPIOPortId;
#define GPIO_PORT_COUNT   (GPIO_PORT_ID_I + 1)
#define GPIO_PORT_ID_POS  4

  /**
   * Define GPIO pin identifiers
   */
  typedef enum GPIOPinId
  {
    GPIO_PIN_ID_0,
    GPIO_PIN_ID_1,
    GPIO_PIN_ID_2,
    GPIO_PIN_ID_3,
    GPIO_PIN_ID_4,
    GPIO_PIN_ID_5,
    GPIO_PIN_ID_6,
    GPIO_PIN_ID_7,
    GPIO_PIN_ID_8,
    GPIO_PIN_ID_9,
    GPIO_PIN_ID_10,
    GPIO_PIN_ID_11,
    GPIO_PIN_ID_12,
    GPIO_PIN_ID_13,
    GPIO_PIN_ID_14,
    GPIO_PIN_ID_15
  }
  GPIOPinId;
#define GPIO_PIN_COUNT_PER_PORT         16
#define GPIO_PIN_ID_TO_HAL_PIN(pin_id)  (((uint16_t)1u) << (pin_id))

#define GPIO_FROM_IDS(            port_id, pin_id)  ((uint32_t)   ((port_id) << GPIO_PORT_ID_POS) | (pin_id))
#define GPIO_PORT_ID_FROM_GPIO_ID(gpio_id)          ((GPIOPortId)(((gpio_id) >> GPIO_PORT_ID_POS) & 0x0F))
#define GPIO_PIN_ID_FROM_GPIO_ID( gpio_id)          ((GPIOPinId)  ((gpio_id)                      & 0x0F))

  /**
   * Define the GPIO identifiers
   */
  typedef enum GPIOId
  {
    GPIO_NONE = GPIO_FROM_IDS(GPIO_PORT_ID_NONE, GPIO_PIN_ID_0),

    GPIO_PA0  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_0),
    GPIO_PA1  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_1),
    GPIO_PA2  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_2),
    GPIO_PA3  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_3),
    GPIO_PA4  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_4),
    GPIO_PA5  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_5),
    GPIO_PA6  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_6),
    GPIO_PA7  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_7),
    GPIO_PA8  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_8),
    GPIO_PA9  = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_9),
    GPIO_PA10 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_10),
    GPIO_PA11 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_11),
    GPIO_PA12 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_12),
    GPIO_PA13 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_13),
    GPIO_PA14 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_14),
    GPIO_PA15 = GPIO_FROM_IDS(GPIO_PORT_ID_A, GPIO_PIN_ID_15),

    GPIO_PB0  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_0),
    GPIO_PB1  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_1),
    GPIO_PB2  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_2),
    GPIO_PB3  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_3),
    GPIO_PB4  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_4),
    GPIO_PB5  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_5),
    GPIO_PB6  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_6),
    GPIO_PB7  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_7),
    GPIO_PB8  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_8),
    GPIO_PB9  = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_9),
    GPIO_PB10 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_10),
    GPIO_PB11 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_11),
    GPIO_PB12 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_12),
    GPIO_PB13 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_13),
    GPIO_PB14 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_14),
    GPIO_PB15 = GPIO_FROM_IDS(GPIO_PORT_ID_B, GPIO_PIN_ID_15),

    GPIO_PC0  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_0),
    GPIO_PC1  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_1),
    GPIO_PC2  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_2),
    GPIO_PC3  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_3),
    GPIO_PC4  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_4),
    GPIO_PC5  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_5),
    GPIO_PC6  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_6),
    GPIO_PC7  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_7),
    GPIO_PC8  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_8),
    GPIO_PC9  = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_9),
    GPIO_PC10 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_10),
    GPIO_PC11 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_11),
    GPIO_PC12 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_12),
    GPIO_PC13 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_13),
    GPIO_PC14 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_14),
    GPIO_PC15 = GPIO_FROM_IDS(GPIO_PORT_ID_C, GPIO_PIN_ID_15),

    GPIO_PD0  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_0),
    GPIO_PD1  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_1),
    GPIO_PD2  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_2),
    GPIO_PD3  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_3),
    GPIO_PD4  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_4),
    GPIO_PD5  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_5),
    GPIO_PD6  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_6),
    GPIO_PD7  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_7),
    GPIO_PD8  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_8),
    GPIO_PD9  = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_9),
    GPIO_PD10 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_10),
    GPIO_PD11 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_11),
    GPIO_PD12 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_12),
    GPIO_PD13 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_13),
    GPIO_PD14 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_14),
    GPIO_PD15 = GPIO_FROM_IDS(GPIO_PORT_ID_D, GPIO_PIN_ID_15),

    GPIO_PE0  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_0),
    GPIO_PE1  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_1),
    GPIO_PE2  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_2),
    GPIO_PE3  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_3),
    GPIO_PE4  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_4),
    GPIO_PE5  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_5),
    GPIO_PE6  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_6),
    GPIO_PE7  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_7),
    GPIO_PE8  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_8),
    GPIO_PE9  = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_9),
    GPIO_PE10 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_10),
    GPIO_PE11 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_11),
    GPIO_PE12 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_12),
    GPIO_PE13 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_13),
    GPIO_PE14 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_14),
    GPIO_PE15 = GPIO_FROM_IDS(GPIO_PORT_ID_E, GPIO_PIN_ID_15),

    GPIO_PF0  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_0),
    GPIO_PF1  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_1),
    GPIO_PF2  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_2),
    GPIO_PF3  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_3),
    GPIO_PF4  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_4),
    GPIO_PF5  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_5),
    GPIO_PF6  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_6),
    GPIO_PF7  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_7),
    GPIO_PF8  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_8),
    GPIO_PF9  = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_9),
    GPIO_PF10 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_10),
    GPIO_PF11 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_11),
    GPIO_PF12 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_12),
    GPIO_PF13 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_13),
    GPIO_PF14 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_14),
    GPIO_PF15 = GPIO_FROM_IDS(GPIO_PORT_ID_F, GPIO_PIN_ID_15),

    GPIO_PG0  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_0),
    GPIO_PG1  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_1),
    GPIO_PG2  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_2),
    GPIO_PG3  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_3),
    GPIO_PG4  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_4),
    GPIO_PG5  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_5),
    GPIO_PG6  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_6),
    GPIO_PG7  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_7),
    GPIO_PG8  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_8),
    GPIO_PG9  = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_9),
    GPIO_PG10 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_10),
    GPIO_PG11 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_11),
    GPIO_PG12 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_12),
    GPIO_PG13 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_13),
    GPIO_PG14 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_14),
    GPIO_PG15 = GPIO_FROM_IDS(GPIO_PORT_ID_G, GPIO_PIN_ID_15),

    GPIO_PH0  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_0),
    GPIO_PH1  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_1),
    GPIO_PH2  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_2),
    GPIO_PH3  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_3),
    GPIO_PH4  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_4),
    GPIO_PH5  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_5),
    GPIO_PH6  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_6),
    GPIO_PH7  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_7),
    GPIO_PH8  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_8),
    GPIO_PH9  = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_9),
    GPIO_PH10 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_10),
    GPIO_PH11 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_11),
    GPIO_PH12 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_12),
    GPIO_PH13 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_13),
    GPIO_PH14 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_14),
    GPIO_PH15 = GPIO_FROM_IDS(GPIO_PORT_ID_H, GPIO_PIN_ID_15),

    GPIO_PI0  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_0),
    GPIO_PI1  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_1),
    GPIO_PI2  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_2),
    GPIO_PI3  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_3),
    GPIO_PI4  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_4),
    GPIO_PI5  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_5),
    GPIO_PI6  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_6),
    GPIO_PI7  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_7),
    GPIO_PI8  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_8),
    GPIO_PI9  = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_9),
    GPIO_PI10 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_10),
    GPIO_PI11 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_11),
    GPIO_PI12 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_12),
    GPIO_PI13 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_13),
    GPIO_PI14 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_14),
    GPIO_PI15 = GPIO_FROM_IDS(GPIO_PORT_ID_I, GPIO_PIN_ID_15),
  }
  GPIOId;

typedef enum GPIOPullType
{
  GPIO_PULL_NONE = 0u,
  GPIO_PULL_UP   = 1u,
  GPIO_PULL_DOWN = 2u
}
GPIOPullType;

typedef enum GPIOOutputMode
{
  GPIO_PUSH_PULL  = 0u,
  GPIO_OPEN_DRAIN = 1u
}
GPIOOutputMode;

#undef GPIO_SPEED_LOW
#undef GPIO_SPEED_MEDIUM
#undef GPIO_SPEED_HIGH
typedef enum GPIOOutputSpeed
{
  GPIO_SPEED_LOW       = 0u,
  GPIO_SPEED_MEDIUM    = 1u,
  GPIO_SPEED_HIGH      = 2u,
  GPIO_SPEED_VERY_HIGH = 3u
}
GPIOOutputSpeed;

/**
 * Defines the edge sensitivity values
 */
typedef enum GPIOEdgeSensitivity
{
  GPIO_NO_EDGE_SENSITIVITY     = 0,
  GPIO_RISING_EDGE             = ((uint32_t)0x00100000),
  GPIO_FALLING_EDGE            = ((uint32_t)0x00200000),
  GPIO_RISING_AND_FALLING_EDGE = GPIO_RISING_EDGE | GPIO_FALLING_EDGE
}
GPIO_EdgeSensitivity;
#define is_sensitive_to_rising_edge( sens) ((sens) & GPIO_RISING_EDGE)
#define is_sensitive_to_falling_edge(sens) ((sens) & GPIO_FALLING_EDGE)

#define GPIO_MODE_IT   ((uint32_t)0x10010000)   ///< base mode for a GPIO configured as an interruption source.
#define GPIO_MODE_EVT  ((uint32_t)0x10020000)   ///< base mode for a GPIO configured as an event source.


/**
 * Defines the type of the function used to handle interruption requests, without argument.
 */
typedef void (*GPIOIrqHandler)(void);

/**
 * Defines the type of the function used to handle interruption requests, with an argument.
 *
 * @param[in] pv_arg the argument passed to the callback function. Can be NULL.
 */
typedef void (*GPIOIrqHandlerArg)(void *pv_arg);


extern GPIO_TypeDef *gpio_hal_port_and_pin_from_id(GPIOId gpio_id, uint32_t *pu32_pin);

extern void gpio_set_irq_handler(         GPIOId            gpio_id,
					  uint32_t          prio,
					  GPIOIrqHandler    pf_handler);
extern void gpio_set_irq_handler_with_arg(GPIOId            gpio_id,
					  uint32_t          prio,
					  GPIOIrqHandlerArg pf_handler,
					  void             *pv_arg);
#define     gpio_remove_irq_handler(gpio_id)  gpio_set_irq_handler(gpio_id, 0, NULL)

extern void gpio_enable_clock_using_gpio_id(   GPIOId gpio_id);
extern void gpio_disable_clock_using_gpio_id(  GPIOId gpio_id);
extern void gpio_enable_sclock_using_gpio_id(  GPIOId gpio_id);
extern void gpio_disable_sclock_using_gpio_id( GPIOId gpio_id);
#define     gpio_enable_clocks_using_gpio_ids( ...)  \
  _gpio_enable_clocks_using_gpio_ids(GPIO_NONE, __VA_ARGS__, GPIO_NONE)
#define     gpio_disable_clocks_using_gpio_ids(...)  \
  _gpio_disable_clocks_using_gpio_ids(GPIO_NONE, __VA_ARGS__, GPIO_NONE)
extern void _gpio_enable_clocks_using_gpio_ids( GPIOId ref, ...);
extern void _gpio_disable_clocks_using_gpio_ids(GPIOId ref, ...);

extern void gpio_use_gpio_with_id( GPIOId id);
extern void gpio_free_gpio_with_id(GPIOId id);
#define     gpio_use_gpios_with_ids( ...)  \
  _gpio_use_gpios_with_ids( GPIO_NONE, __VA_ARGS__, GPIO_NONE)
#define     gpio_free_gpios_with_ids(...)  \
  _gpio_free_gpios_with_ids(GPIO_NONE, __VA_ARGS__, GPIO_NONE)
extern void _gpio_use_gpios_with_ids( GPIOId ref, ...);
extern void _gpio_free_gpios_with_ids(GPIOId ref, ...);


#define gpio_enable_clock( port) (RCC->AHB2ENR |=  RCC_AHB2ENR_##port##EN)
#define gpio_disable_clock(port) (RCC->AHB2ENR &= ~RCC_AHB2ENR_##port##EN)

#define gpio_set_low(  port, pin)  (port)->BRR  = (uint32_t)(1u << (pin))
#define gpio_set_high( port, pin)  (port)->BSRR = (uint32_t)(1u << (pin))
#define gpio_toggle(   port, pin)  (port)->ODR ^= (uint32_t)(1u << (pin))
#define gpio_set_level(port, pin, level) do \
    { \
      if((level)) \
      {\
	gpio_set_high(port, pin); \
      } \
      else \
      { \
	gpio_set_low(port, pin); \
      }\
    }\
    while(0)

extern void gpio_set_low_using_id(  GPIOId id);
extern void gpio_set_high_using_id( GPIOId id);
extern void gpio_set_level_using_id(GPIOId id, int level);

#define gpio_set_input( port, pin)  (port)->MODER &= ~(uint32_t)(0x03u << ((pin) * 2))
#define gpio_set_output(port, pin, level)  \
    gpio_set_level(port, pin, level); \
    (port)->MODER &= ~(uint32_t)(0x03u << ((pin) * 2)); \
    (port)->MODER |=  (uint32_t)(0x01u << ((pin) * 2))
#define gpio_set_alternate(port, pin)  \
    gpio_set_input(port, pin); \
    (port)->MODER |= (uint32_t)(0x02u << ((pin) * 2))
#define gpio_set_analog( port, pin)  (port)->MODER |= (uint32_t)(0x03u << ((pin) * 2))

#define gpio_set_pull_type(port, pin, pull) \
    (port)->PUPDR &= ~(uint32_t)( 0x03u << ((pin) * 2)); \
    (port)->PUPDR |=  (uint32_t)((pull) << ((pin) * 2))
#define gpio_set_mode(port, pin, mode) \
    (port)->OTYPER   &= ~(uint32_t)( 0x01u  << (pin)); \
    (port)->OTYPER   |=  (uint32_t)((mode)  << (pin))
#define gpio_set_speed(port, pin, speed) \
    (port)->OSPEEDR &= ~(uint32_t)( 0x03u  << (pin)); \
    (port)->OSPEEDR |=  (uint32_t)((speed) << (pin))

#define gpio_set_input_full(port, pin, pull) \
    gpio_set_input(    port, pin); \
    gpio_set_pull_type(port, pin, pull)
#define gpio_set_output_full(port, pin, mode, pull, speed, level) \
    gpio_set_pull_type(port, pin, pull);  \
    gpio_set_mode(     port, pin, mode);  \
    gpio_set_speed(    port, pin, speed); \
    gpio_set_output(   port, pin, level)
#define gpio_set_alternate_func(port, pin, alt) \
  (port)->AFR[(pin) / 8] &= ~(uint32_t)( 0x0F << (((pin) % 8) * 4)); \
  (port)->AFR[(pin) / 8] |=  (uint32_t)((alt) << (((pin) % 8) * 4))
#define gpio_set_alternate_full(port, pin, alt, mode, pull, speed) \
    gpio_set_alternate(     port, pin); \
    gpio_set_pull_type(     port, pin, pull); \
    gpio_set_mode(          port, pin, mode); \
    gpio_set_speed(         port, pin, speed); \
    gpio_set_alternate_func(port, pin, alt)

#define gpio_read(port, pin) ((port)->IDR & (uint32_t)(1u << (pin)))


#ifdef __cplusplus
}
#endif
#endif /* __GPIO_H__ */
