/**
 * Function to pilot a buzzer
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef PERIPHERALS_BUZZER_H_
#define PERIPHERALS_BUZZER_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

  extern void buzzer_set_enable(bool enable);
#define buzzer_enable()   buzzer_set_enable(true)
#define buzzer_sidable()  buzzer_set_enable(false)

  extern bool buzzer_configure_on_off(const char *ps_pin_name);

  extern void buzzer_set_turn_on(bool on);
  extern void buzzer_toggle();
#define buzzer_turn_on()   buzzer_set_turn_on(true)
#define buzzer_turn_off()  buzzer_set_turn_on(false)

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERALS_BUZZER_H_ */
