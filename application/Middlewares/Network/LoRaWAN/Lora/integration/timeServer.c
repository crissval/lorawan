/**
 * Integration file added so that the LoRaWAN communication layer can use
 * our timer system.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */

#include "timeServer.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*!
   * Return the time elapsed since a time reference, in milliseconds.
   *
   * @param[in] time_ref time reference, in milliseconds.
   *
   * @return the time ellapsed.
   */
  TimerTime_t TimerGetElapsedTime(TimerTime_t time_ref)
  {
    uint32_t nowInTicks  = rtc_get_date_as_ticks_since_2000();
    uint32_t pastInTicks = rtc_ms_to_ticks(time_ref);

    // Intentional wrap around. Works Ok if tick duration below 1ms
    return rtc_ticks_to_ms(nowInTicks - pastInTicks);
  }


#ifdef __cplusplus
}
#endif

