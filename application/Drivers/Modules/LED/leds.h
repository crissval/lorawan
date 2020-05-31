/*
 * leds.h
 *
 *  Created on: 2 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef MODULES_LED_LEDS_H_
#define MODULES_LED_LEDS_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * LEDS identifiers
   */
  typedef enum LEDId
  {
    LED_1    = 0x01,
    LED_2    = 0x02,
    LED_ALL  = LED_1 | LED_2,
    LED_NONE = 0x00
  }
  LEDId;

  /**
   * The illuminance state of a LED.
   */
  typedef enum LEDState
  {
    LED_OFF,
    LED_ON
  }
  LEDState;
#define leds_bool_to_state(b) ((LEDState)(b))


void leds_init();
void leds_deinit();

void leds_enable_all(  bool enable);
void leds_enable_debug(bool enable);
void leds_enable_user( bool enable);

void leds_turn_on( LEDId led);
void leds_turn_off(LEDId led);
void leds_turn(    LEDId led, LEDState state);
void leds_toggle(  LEDId led);
void leds_blinky(  LEDId led, uint16_t period_ms, uint16_t count);

#ifdef __cplusplus
}
#endif
#endif /* MODULES_LED_LEDS_H_ */
