/**
 * Integration file added so that the LoRaWAN communication layer can use
 * our timer system.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */

#ifndef NETWORK_LORAWAN_LORA_INTEGRATION_TIMESERVER_H_
#define NETWORK_LORAWAN_LORA_INTEGRATION_TIMESERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "timer.h"
#include "rtc.h"

  typedef uint32_t TimerTime_t;
  typedef Timer    TimerEvent_t;


#define TimerInit(pv_timer, pf_cb)  \
  timer_init(pv_timer, 0, TIMER_MSECS | TIMER_SINGLE_SHOT | TIMER_RELATIVE | TIMER_PROCESS_IN_IRQ); \
  timer_set_callback_no_arg(pv_timer, pf_cb)

#define TimerStart(pv_timer)  timer_start( pv_timer)
#define TimerStop( pv_timer)  timer_stop(  pv_timer)
#define TimerReset(pv_timer)  timer_restart(pv_timer)

#define TimerSetValue(pv_timer, value) \
		timer_stop(pv_timer); \
		timer_set_period(pv_timer, value, TIMER_TU_MSECS)

#define TimerGetCurrentTime()  rtc_ticks_to_ms(rtc_get_date_as_ticks_since_2000())
  extern TimerTime_t TimerGetElapsedTime(TimerTime_t saved_time);


#ifdef __cplusplus
}
#endif
#endif /* NETWORK_LORAWAN_LORA_INTEGRATION_TIMESERVER_H_ */
