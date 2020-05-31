/**
 * @brief Header file to use the STM32 GPIOs.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <stdarg.h>
#include "gpio.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Store the context for an interruption handler.
   */
  typedef struct GPIOIrqHandlerContext
  {
    GPIOIrqHandler    pf_handler;     ///< The handler function, in case do not use argument.
    GPIOIrqHandlerArg pf_handler_arg; ///< The handler function, in case use argument.
    void             *pv_arg;         ///< The argument to call the handler with.
  }
  GPIOIrqHandlerContext;

  /**
   * Store all the interruption handler contexts.
   */
  static GPIOIrqHandlerContext _gpio_irq_handlers[16];  // All set to 0 at initialisation.

  /**
   * Makes the association between a GPIO pin identifier and the associated IRQn.
   */
  static int8_t _gpio_pin_id_to_irqn[16] =
  {
      EXTI0_IRQn,     EXTI1_IRQn,     EXTI2_IRQn,     EXTI3_IRQn,     EXTI4_IRQn,
      EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,
      EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn
  };


  /**
   * Store the association between a port identifier and a HAL Port object.
   */
  static GPIO_TypeDef* _gpio_hal_ports[GPIO_PORT_COUNT] =
  {
      NULL,    // GPIO_PORT_ID_NONE
#ifdef GPIOA
      GPIOA,
#else
      NULL,
#endif
#ifdef GPIOB
      GPIOB,
#else
      NULL,
#endif
#ifdef GPIOC
      GPIOC,
#else
      NULL,
#endif
#ifdef GPIOD
      GPIOD,
#else
      NULL,
#endif
#ifdef GPIOE
      GPIOE,
#else
      NULL,
#endif
#ifdef GPIOF
      GPIOF,
#else
      NULL,
#endif
#ifdef GPIOG
      GPIOG,
#else
      NULL,
#endif
#ifdef GPIOH
      GPIOH,
#else
      NULL,
#endif
#ifdef GPIOI
      GPIOI,
#else
      NULL,
#endif
  };

  /**
   * Keep track of which GPIO is in use.
   */
  static uint16_t _gpio_in_use[GPIO_PORT_COUNT];
#define gpio_is_in_use(gpio_id)   \
  (_gpio_in_use[GPIO_PORT_ID_FROM_GPIO_ID(gpio_id)] &  (1u << GPIO_PIN_ID_FROM_GPIO_ID(gpio_id)))
#define gpio_is_free(gpio_id) (!gpio_is_in_use(pgio_id))
#define gpio_set_in_use(gpio_id)  \
  _gpio_in_use[GPIO_PORT_ID_FROM_GPIO_ID(gpio_id)] |=  (1u << GPIO_PIN_ID_FROM_GPIO_ID(gpio_id))
#define gpio_set_free(gpio_id)    \
  _gpio_in_use[GPIO_PORT_ID_FROM_GPIO_ID(gpio_id)] &= ~(1u << GPIO_PIN_ID_FROM_GPIO_ID(gpio_id))
#define gpio_pin_in_use_in_port(       port_id)  _gpio_in_use[port_id]
#define gpio_all_pins_are_free_in_port(port_id)  (!gpio_pin_in_use_in_port(port_id))



  /**
   * Return the HAL GPIO port and pin using a GPIO identifier.
   *
   * @param[in]  gpio_id  the GPIO's identifier.
   * @param[out] pu32_pin where the HAL pin is written to.
   *                      Can be NULL if we are not interested in this information.
   *
   * @return the HAL GPIO port.
   * @return NULL if no port has been found.
   */
  GPIO_TypeDef *gpio_hal_port_and_pin_from_id(GPIOId gpio_id, uint32_t *pu32_pin)
  {
    GPIO_TypeDef *pv_port = NULL;
    GPIOPortId    port_id = GPIO_PORT_ID_FROM_GPIO_ID(gpio_id);

    if(port_id < GPIO_PORT_COUNT)
    {
      pv_port = _gpio_hal_ports[port_id];
      if(pu32_pin) { *pu32_pin = GPIO_PIN_ID_TO_HAL_PIN(GPIO_PIN_ID_FROM_GPIO_ID(gpio_id)); }
    }

    return pv_port;
  }


  /**
   * Enable a GPIO port clock using a GPIO identifier.
   *
   * @param[in] gpio_id the GPIO's identifier.
   */
  void gpio_enable_clock_using_gpio_id(GPIOId gpio_id)
  {
    RCC->AHB2ENR |=   ((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(gpio_id) - GPIO_PORT_ID_A);
  }

  /**
   * Disable a GPIO port clock using a GPIO identifier.
   *
   * @param[in] gpio_id the GPIO's identifier.
   */
  void gpio_disable_clock_using_gpio_id(GPIOId gpio_id)
  {
    RCC->AHB2ENR &= ~(((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(gpio_id) - GPIO_PORT_ID_A));
  }

  /**
   * Enable a GPIO port clock when in sleep or stop mode using a GPIO identifier.
   *
   * @param[in] gpio_id the GPIO's identifier.
   */
  void gpio_enable_sclock_using_gpio_id(GPIOId gpio_id)
  {
    RCC->AHB2SMENR |=   ((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(gpio_id) - GPIO_PORT_ID_A);
  }

  /**
   * Disable a GPIO port clock when in sleep or stop mode using a GPIO identifier.
   *
   * @param[in] gpio_id the GPIO's identifier.
   */
  void gpio_disable_sclock_using_gpio_id(GPIOId gpio_id)
  {
    RCC->AHB2SMENR &= ~(((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(gpio_id) - GPIO_PORT_ID_A));
  }

  /**
   * Enable clocks for multiple GPIOS using their identifiers.
   *
   * This function uses a variable number of arguments.
   * The last argument MUST always be GPIO_PORT_ID_NONE.
   *
   * @param[in] ref a reference. Is only used to get the next arguments.
   *                We do not use it's value, so it can be anything.
   */
  void _gpio_enable_clocks_using_gpio_ids(GPIOId ref,...)
  {
    int     id;
    va_list args;

    va_start(args, ref);
    while((id = va_arg(args, int)) != GPIO_NONE)
    {
      RCC->AHB2ENR |= ((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(id) - GPIO_PORT_ID_A);
    }
    va_end(args);
  }

  /**
   * Disable clocks for multiple GPIOS using their identifiers.
   *
   * This function uses a variable number of arguments.
   * The last argument MUST always be GPIO_PORT_ID_NONE.
   *
   * @param[in] ref a reference. Is only used to get the next arguments.
   *                We do not use it's value, so it can be anything.
   */
  void _gpio_disable_clocks_using_gpio_ids(GPIOId ref,...)
  {
    int     id;
    va_list args;

    va_start(args, ref);
    while((id = va_arg(args, int)) != GPIO_NONE)
    {
      RCC->AHB2ENR &= ~(((uint32_t)1u) << (GPIO_PORT_ID_FROM_GPIO_ID(id) - GPIO_PORT_ID_A));
    }
    va_end(args);
  }

  /**
   * Declare that we want to use a GPIO, and enable the port's clock if needed.
   *
   * @param[in] id the GPIO's identifier.
   */
  void gpio_use_gpio_with_id(GPIOId id)
  {
    if(id != GPIO_NONE)
    {
      if(gpio_all_pins_are_free_in_port(GPIO_PORT_ID_FROM_GPIO_ID(id)))
      {
	gpio_enable_clock_using_gpio_id(id);
      }
      gpio_set_in_use(id);
    }
  }

  /**
   * Declare that we no longer need a GPIO.
   * Disable the port's clock if all it's pins are free.
   *
   * @param[in] id the GPIO's identifier.
   */
  void gpio_free_gpio_with_id(GPIOId id)
  {
    uint32_t pin;

    if(gpio_is_in_use(id))
    {
      gpio_set_free(id);
      HAL_GPIO_DeInit(gpio_hal_port_and_pin_from_id(id,  &pin), pin);

      if(gpio_all_pins_are_free_in_port(GPIO_PORT_ID_FROM_GPIO_ID(id)))
      {
	gpio_disable_clock_using_gpio_id(id);
      }
    }
  }

  /**
   * Declare that we want to use several GPIOS using their identifiers.
   *
   * This function uses a variable number of arguments.
   * The last argument MUST always be GPIO_PORT_ID_NONE.
   *
   * @param[in] ref a reference. Is only used to get the next arguments.
   *                We do not use it's value, so it can be anything.
   */
  void _gpio_use_gpios_with_ids(GPIOId ref, ...)
  {
    int     id;
    va_list args;

    va_start(args, ref);
    while((id = va_arg(args, int)) != GPIO_NONE)
    {
      gpio_use_gpio_with_id((GPIOId)id);
    }
    va_end(args);
  }

  /**
   * Declare that we no longer use several GPIOS using their identifiers.
   * Disable clock of ports whose all pins are free.
   *
   * This function uses a variable number of arguments.
   * The last argument MUST always be GPIO_PORT_ID_NONE.
   *
   * @param[in] ref a reference. Is only used to get the next arguments.
   *                We do not use it's value, so it can be anything.
   */
  void _gpio_free_gpios_with_ids(GPIOId ref, ...)
  {
    int     id;
    va_list args;

    va_start(args, ref);
    while((id = va_arg(args, int)) != GPIO_NONE)
    {
      gpio_free_gpio_with_id((GPIOId)id);
    }
    va_end(args);
  }


  /**
   * Set a GPIO pin low.
   *
   * @param[in] id the GPIO's identifier. Can be GPIO_NONE.
   */
  void gpio_set_low_using_id(GPIOId id)
  {
    if(id > 0 && id < GPIO_PORT_COUNT)
    {
      gpio_set_low(_gpio_hal_ports[GPIO_PORT_ID_FROM_GPIO_ID(id)], GPIO_PIN_ID_FROM_GPIO_ID(id));
    }
  }

  /**
   * Set a GPIO pin high.
   *
   * @param[in] id the GPIO's identifier. Can be GPIO_NONE.
   */
  void gpio_set_high_using_id(GPIOId id)
  {
    if(id > 0 && id < GPIO_PORT_COUNT)
    {
      gpio_set_high(_gpio_hal_ports[GPIO_PORT_ID_FROM_GPIO_ID(id)], GPIO_PIN_ID_FROM_GPIO_ID(id));
    }
  }

  /**
   * Set a GPIO pin high or low.
   *
   * @param[in] id    the GPIO's identifier. Can be GPIO_NONE.
   * @param[in] level 0 to set to low, any other value to set to high.
   */
  void gpio_set_level_using_id(GPIOId id, int level)
  {
    GPIOPortId port_id;
    GPIOPinId  pin_id;

    if(id > 0 && id < GPIO_PORT_COUNT)
    {
      port_id = GPIO_PORT_ID_FROM_GPIO_ID(id);
      pin_id  = GPIO_PIN_ID_FROM_GPIO_ID( id);

      if(level) { gpio_set_high(_gpio_hal_ports[port_id], pin_id); }
      else      { gpio_set_low( _gpio_hal_ports[port_id], pin_id); }
    }
  }


  /**
   * Set/unset an interrupt handler not using an argument for a given GPIO.
   *
   * @param[in] gpio_id    the identifier of the GPIO to set/unset a handler for.
   * @param[in] prio       the interruption priority to use.
   * @param[in] pf_handler the handler function. Set to NULL to remove current handler.
   * @param[in] pv_arg     the argument to pass to the handler function  when called. Can be NULL.
   */
  void gpio_set_irq_handler(GPIOId gpio_id, uint32_t prio, GPIOIrqHandler pf_handler)
  {
    GPIOIrqHandlerContext *pv_ctx;
    GPIOPinId              pin;
    IRQn_Type              irqn;

    if(gpio_id != GPIO_NONE)
    {
      pin                    = GPIO_PIN_ID_FROM_GPIO_ID(gpio_id);
      pv_ctx                 = &_gpio_irq_handlers[pin];
      pv_ctx->pf_handler_arg = NULL;
      pv_ctx->pv_arg         = NULL;
      pv_ctx->pf_handler     = pf_handler;

      if(pf_handler)
      {
	irqn = (IRQn_Type)_gpio_pin_id_to_irqn[pin];
	HAL_NVIC_SetPriority(irqn , prio, 0);
	HAL_NVIC_EnableIRQ(  irqn);
      }
    }
  }

  /**
   * Set/unset an interrupt handler using an argument for a given GPIO.
   *
   * @param[in] gpio_id    the identifier of the GPIO to set/unset a handler for.
   * @param[in] prio       the interruption priority to use.
   * @param[in] pf_handler the handler function. Set to NULL to remove current handler.
   * @param[in] pv_arg     the argument to pass to the handler function  when called. Can be NULL.
   */
  void gpio_set_irq_handler_with_arg(GPIOId            gpio_id,
				     uint32_t          prio,
				     GPIOIrqHandlerArg pf_handler,
				     void             *pv_arg)
  {
    GPIOIrqHandlerContext *pv_ctx;
    GPIOPinId              pin;
    IRQn_Type              irqn;

    if(gpio_id != GPIO_NONE)
    {
      pin                    = GPIO_PIN_ID_FROM_GPIO_ID(gpio_id);
      pv_ctx                 = &_gpio_irq_handlers[pin];
      pv_ctx->pf_handler     = NULL;
      pv_ctx->pf_handler_arg = pf_handler;
      pv_ctx->pv_arg         = pv_arg;

      if(pf_handler)
      {
	irqn = (IRQn_Type)_gpio_pin_id_to_irqn[pin];
	HAL_NVIC_SetPriority(irqn , prio, 0);
	HAL_NVIC_EnableIRQ(  irqn);
      }
    }
  }


  // ================ Interruption handlers ==============

  static void gpio_process_interruption_for_pin(GPIOPinId pin_id)
  {
    GPIOIrqHandlerContext *pv_ctx;

    if(__HAL_GPIO_EXTI_GET_IT( GPIO_PIN_ID_TO_HAL_PIN(pin_id)))
    {
      __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_ID_TO_HAL_PIN(pin_id));

      pv_ctx = &_gpio_irq_handlers[pin_id];
      if(     pv_ctx->pf_handler)     { pv_ctx->pf_handler();                   }
      else if(pv_ctx->pf_handler_arg) { pv_ctx->pf_handler_arg(pv_ctx->pv_arg); }
    }
  }


  void EXTI0_IRQHandler(void) { gpio_process_interruption_for_pin(GPIO_PIN_ID_0); }
  void EXTI1_IRQHandler(void) { gpio_process_interruption_for_pin(GPIO_PIN_ID_1); }
  void EXTI2_IRQHandler(void) { gpio_process_interruption_for_pin(GPIO_PIN_ID_2); }
  void EXTI3_IRQHandler(void) { gpio_process_interruption_for_pin(GPIO_PIN_ID_3); }
  void EXTI4_IRQHandler(void) { gpio_process_interruption_for_pin(GPIO_PIN_ID_4); }


  void EXTI9_5_IRQHandler(void)
  {
    gpio_process_interruption_for_pin(GPIO_PIN_ID_5);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_6);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_7);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_8);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_9);
  }

  void EXTI15_10_IRQHandler(void)
  {
    gpio_process_interruption_for_pin(GPIO_PIN_ID_10);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_11);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_12);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_13);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_14);
    gpio_process_interruption_for_pin(GPIO_PIN_ID_15);
  }


#ifdef __cplusplus
}
#endif
