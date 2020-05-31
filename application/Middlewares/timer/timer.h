/**
 * Implements a timer system.
 *
 * @author Jérôme FUCHET
 * @date   2018
 */
#ifndef TIMER_TIMER_H_
#define TIMER_TIMER_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the time units that can be used by the timer.
   */
  typedef enum TimerTimeUnit
  {
    TIMER_TU_MSECS,  ///< Milliseconds. Beware: time values are actually RTC ticks, not milliseconds.
    TIMER_TU_SECS    ///< Seconds.
  }
  TimerTimeUnit;
#define TIMER_UNITS_COUNT  (TIMER_TU_SECS + 1)


  /**
   * Defines the timer status flags.
   */
  typedef enum TimerStatusFlag
  {
    TIMER_STATUS_PERIODIC       = 1u << 4,  ///< Indicate that the timer is periodic and not one shot.
    TIMER_STATUS_IS_RUNNING     = 1u << 5,  ///< Indicate if the the timer is running or not.
    TIMER_STATUS_HAS_TIMED_OUT  = 1u << 6,  ///< Indicate if the timer has elapsed or not.
    TIMER_STATUS_IS_ABSOLUTE    = 1u << 7,  ///< Indicate if the timer is absolute (reference is midnight), or relative.
    TIMER_STATUS_PROCESS_IN_IRQ = 1u << 8   ///< Do timer processing in IRQ and not in timer_process().
  }
  TimerStatusFlag;
  typedef uint16_t TimerStatus;  ///< The type used to store the timer status.
#define TIMER_STATUS_TIME_UNIT_MASK  0x03
#define timer_time_unit_from_status(status)  ((TimerTimeUnit)((status) & TIMER_STATUS_TIME_UNIT_MASK))

  /**
   * Defines the timer option flags.
   */
  typedef enum TimerOptionFlag
  {
    TIMER_MSECS          = TIMER_TU_MSECS,              ///< Milliseconds.
    TIMER_SECS           = TIMER_TU_SECS,               ///< Seconds.
    TIMER_SINGLE_SHOT    = 0,                           ///< The timer is single shot.
    TIMER_PERIODIC       = TIMER_STATUS_PERIODIC,       ///< The timer is periodic.
    TIMER_ABSOLUTE       = TIMER_STATUS_IS_ABSOLUTE,    ///< The timer is absolute (reference is midnight).
    TIMER_RELATIVE       = 0,                           ///< Timer is relative.
    TIMER_PROCESS_IN_IRQ = TIMER_STATUS_PROCESS_IN_IRQ  ///< Do timer processing in IRQ and not in timer_process()..
  }
  TimerOptionFlag;
  typedef uint16_t TimerOptions;
#define TIMER_OPTIONS_TIME_UNIT_MASK  TIMER_STATUS_TIME_UNIT_MASK
#define timer_time_unit_from_options(options)  ((TimerTimeUnit)((options) & TIMER_OPTIONS_TIME_UNIT_MASK))


  /**
   * Defines the different time callback options that can be used.
   */
  typedef enum TimerCallbackType
  {
    TIMER_CB_TYPE_NONE,     ///< No callback type set.
    TIMER_CB_TYPE_NO_ARGS,  ///< A callback function that has no argument.
    TIMER_CB_TYPE_PV_ARG    ///< The callback function has a single argument of type void *.
  }
  TimerCallbackType;

  /**
   * Type used to store time callback informations.
   */
  typedef struct TimerCallback
  {
    TimerCallbackType type;   ///< The type of callback.

    union
    {
      struct
      {
	void (*pf_cb)(void);  ///< The callback function.
      } no_args;              ///< The informations for the NO_ARGS callback type.

      struct
      {
	void (*pf_cb)(void *pv_arg); ///< The callback function.
	void  *pv_arg;               ///< The argument to give the callback function.
      } pv_arg;                      ///< The informations for the PV_ARG callback type.
    };
  }
  TimerCallback;

  typedef uint32_t TimerTime;


  /**
   * Defines the timer class.
   */
  struct Timer;
  typedef struct Timer Timer;
  struct Timer
  {
    TimerStatus   status;       ///< The status.
    TimerTime     period;       ///< The timer's period. Time Unit depends on status value.
    TimerTime     timeout;      ///< The timeout value. May be different from period for absolute timers.
    TimerTime     tref;         ///< The reference time to compute elapsed time.
    TimerCallback cb;           ///< The callback informations.

    Timer       *pv_previous;  ///< Previous timer in the timer list. Can be NULL.
    Timer       *pv_next;      ///< Next timer in the timer list. Can be NULL.
  };

#define timer_is_periodic(   pv_timer)  ((pv_timer)->status & TIMER_STATUS_PERIODIC)
#define timer_is_single_shot(pv_timer)  (!timer_is_periodic(pv_timer))
#define timer_is_running(    pv_timer)  ((pv_timer)->status & TIMER_STATUS_IS_RUNNING)
#define timer_has_timed_out( pv_timer)  ((pv_timer)->status & TIMER_STATUS_HAS_TIMED_OUT)
#define timer_is_absolute(   pv_timer)  ((pv_timer)->status & TIMER_STATUS_IS_ABSOLUTE)
#define timer_is_relative(   pv_timer)  (!timer_is_absolute(pv_timer))


  extern bool timer_process(void);

  extern void timer_init(Timer       *pv_timer,
			 TimerTime    period,
			 TimerOptions options);

  extern void timer_set_callback_none(  Timer *pv_timer);
  extern void timer_set_callback_no_arg(Timer *pv_timer, void (pf_cb)(void));
  extern void timer_set_callback_pv_arg(Timer *pv_timer, void (pf_cb)(void *pv_arg), void *pv_arg);

  extern void timer_set_period(Timer *pv_timer, TimerTime period, TimerTimeUnit unit);
#define       timer_period(pv_timer)  ((pv_timer)->period)

  extern void timer_stop(             Timer       *pv_timer);
  extern void timer_start(            Timer       *pv_timer);
  extern void timer_start_with_period(Timer       *pv_timer, TimerTime period, TimerTimeUnit unit);
  extern void timer_restart(          Timer *pv_timer);




#ifdef __cplusplus
}
#endif
#endif /* TIMER_TIMER_H_ */
