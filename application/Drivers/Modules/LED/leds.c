/*
 * leds.c
 *
 *  Created on: 2 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#include "leds.h"
#include "board.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CONNECSENS_LED_COUNT  4


  typedef struct LEDConfig
  {
    LEDId         id;          ///< The led identifier.
    bool          debug;       ///< Is this a debug LED?
    bool          enabled;     ///< Is the LED enabled?
    GPIOId        gpio;        ///< The GPIO used to control the LED.
    uint8_t       activeLevel; ///< The value to write to the pin to activate the LED.
  }
  LEDConfig;


  static LEDConfig _leds_configs[CONNECSENS_LED_COUNT] =
  {
      { LED_1, true,  false, LED1_DEBUG_GPIO, LED1_DEBUG_ACTIVE_LEVEL },
      { LED_2, true,  false, LED2_DEBUG_GPIO, LED2_DEBUG_ACTIVE_LEVEL },
      { LED_1, false, true,  LED1_USER_GPIO,  LED1_USER_ACTIVE_LEVEL  },
      { LED_2, false, true,  LED2_USER_GPIO,  LED2_USER_ACTIVE_LEVEL  }
  };

  static bool _leds_has_been_initialised = false;

  static void leds_turn_on_using_config( const LEDConfig *pv_config);
  static void leds_turn_off_using_config(const LEDConfig *pv_config);
  static void leds_turn_using_config(    const LEDConfig *pv_config, LEDState state);
  static void leds_toggle_using_config(  const LEDConfig *pv_config);


  /**
   * Initialise the LEDs.
   */
  void leds_init()
  {
    uint8_t          i;
    GPIO_InitTypeDef init;

    if(_leds_has_been_initialised) return;

    init.Alternate = 0;
    init.Mode      = GPIO_MODE_OUTPUT_PP;
    init.Pull      = GPIO_NOPULL;
    init.Speed     = GPIO_SPEED_FREQ_LOW;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      gpio_use_gpio_with_id(_leds_configs[i].gpio);
      HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(_leds_configs[i].gpio, &init.Pin), &init);
    }

    leds_turn_off(LED_ALL);
    _leds_has_been_initialised = true;
  }

  /**
   * De-initialise the LEDs
   */
  void leds_deinit()
  {
    uint8_t i;

    leds_turn_off(LED_ALL);

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      gpio_free_gpio_with_id(_leds_configs[i].gpio);
    }
    _leds_has_been_initialised = false;
  }


  /**
   * Enable/Disable all LEDs
   *
   * When LEDs are disabled then they are turned off
   */
  void leds_enable_all(bool enable)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      _leds_configs[i].enabled = true;
    }
  }

  /**
   * Enable/disable all debug LEDs.
   *
   * The disabled LEDs are turned off.
   */
  void leds_enable_debug(bool enable)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      LEDConfig *pv_lc = &_leds_configs[i];
      if(pv_lc->debug)
      {
        if(!enable) { leds_turn_off(pv_lc->id); }
        pv_lc->enabled = enable;
      }
    }
  }

  /**
   * Enable/disable all user LEDs.
   *
   * The disabled LEDs are turned off.
   */
  void leds_enable_user(bool enable)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      LEDConfig *pv_lc = &_leds_configs[i];
      if(!pv_lc->debug)
      {
        if(!enable) { leds_turn_off(pv_lc->id); }
        pv_lc->enabled = enable;
      }
    }
  }


  /**
   * Turn on a LED using its configuration object.
   *
   * @param[in] pv_config the LED's configuration object. MUST be NOT NULL.
   */
  static void leds_turn_on_using_config(const LEDConfig *pv_config)
  {
    uint32_t pin;

    if(pv_config->enabled)
    {
      HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(pv_config->gpio, &pin),
			pin,
			(GPIO_PinState)pv_config->activeLevel);
    }
  }

  /**
   * Turn off a LED using its configuration object.
   *
   * @param[in] pv_config the LED's configuration object. MUST be NOT NULL.
   */
  static void leds_turn_off_using_config(const LEDConfig *pv_config)
  {
    uint32_t pin;

    HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(pv_config->gpio, &pin),
		      pin,
		      (GPIO_PinState)!pv_config->activeLevel);
  }

  /**
   * Turn on or off a LED using its configuration object.
   *
   * @param[in] pv_config the LED's configuration object. MUST be NOT NULL.
   * @param[in] state     the state to turn the LED to.
   */
  static void leds_turn_using_config(const LEDConfig *pv_config, LEDState state)
  {
    state ? leds_turn_on_using_config(pv_config) : leds_turn_off_using_config(pv_config);
  }

  /**
   * Toggle a LED using its configuration object.
   *
   * @param[in] pv_config the LED's configuration object. MUST be NOT NULL.
   */
  static void leds_toggle_using_config(const LEDConfig *pv_config)
  {
    uint32_t pin;

    if(pv_config->enabled) { HAL_GPIO_TogglePin(gpio_hal_port_and_pin_from_id(pv_config->gpio, &pin), pin); }
    else                   { leds_turn_off_using_config(pv_config);                   }
  }


  /**
   * Turn LED(s) on.
   *
   * @param[in] led the identifier(s) of the LED(s) to turn on.
   */
  void leds_turn_on(LEDId led)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      const LEDConfig *pv_lc = &_leds_configs[i];
      if(pv_lc->id & led) { leds_turn_on_using_config(pv_lc); }
    }
  }

  /**
   * Turn LED(s) off.
   *
   * @param[in] led the identifier(s) of the LED(s) to turn off.
   */
  void leds_turn_off(LEDId led)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      const LEDConfig *pv_lc = &_leds_configs[i];
      if(pv_lc->id & led) { leds_turn_off_using_config(pv_lc); }
    }
  }

  /**
   * Turn LED(s) to a given satet.
   *
   * @param[in] led   the identifier(s) of the LED(s) to turn off.
   * @param[in] state the state to turn the LED(s) to.
   */
  void leds_turn(LEDId led, LEDState state)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      const LEDConfig *pv_lc = &_leds_configs[i];
      if(pv_lc->id & led) { leds_turn_using_config(pv_lc, state); }
    }
  }

  /**
   * Toggle LED(s).
   *
   * @param[in] led the identifier(s) of the LED(s) toggle.
   */
  void leds_toggle(LEDId led)
  {
    uint8_t i;

    for(i = 0; i < CONNECSENS_LED_COUNT; i++)
    {
      const LEDConfig *pv_lc = &_leds_configs[i];
      if(pv_lc->id & led) { leds_toggle_using_config(pv_lc); }
    }
  }

  /**
   * Toggle LED(s) a given number of time with a given period.
   *
   * @param[in] led       the LED(s) to blink.
   * @param[in] period_ms the toggling period, in milliseconds.
   * @param[in] count     the number of toggles.
   */
  void leds_blinky(LEDId led, uint16_t period_ms, uint16_t count)
  {
    uint16_t i;

    for(i = 0; i < count; i++)
    {
      leds_toggle(led);
      board_delay_ms(period_ms);
    }
  }

#ifdef __cplusplus
}
#endif
