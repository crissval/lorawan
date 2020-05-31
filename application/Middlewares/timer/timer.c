/**
 * Implements a timer system.
 *
 * @author Jérôme FUCHET
 * @date   2018
 */
#include <it.h>
#include "timer.h"
#include "rtc.h"
#include "logger.h"


#ifdef __cplusplus
extern "C" {
#endif



#define TIMER_MS_USE_RTC_ALARM      A
#define TIMER_MS_RTC_ALARM_IRQ_ID   PASTER2(RTC_IRQ_ID_ALARM_,   TIMER_MS_USE_RTC_ALARM)
#define TIMER_MS_RTC_ALARM_ID       PASTER2(RTC_ALARM_ID_ALARM_, TIMER_MS_USE_RTC_ALARM)
#define TIMER_SEC_USE_RTC_ALARM     B
#define TIMER_SEC_RTC_ALARM_IRQ_ID  PASTER2(RTC_IRQ_ID_ALARM_,   TIMER_SEC_USE_RTC_ALARM)
#define TIMER_SEC_RTC_ALARM_ID      PASTER2(RTC_ALARM_ID_ALARM_, TIMER_SEC_USE_RTC_ALARM)


  /**
   * Keep track of a group of timers.
   */
  typedef struct TimerGroup
  {
    TimerTimeUnit time_unit;       ///< The time unit used in this group.
    TimerTime   (*pf_time)(void);  ///< Function to use to get time for the current time unit.
    uint32_t      period_min;      ///< Minimum period for this group.
    Timer        *pv_head;         ///< The head of the timer list. Is NULL is the list is empty.

    /**
     * Stop the group's interruption source.
     */
    void (*pf_stop_it_source)(void);

    /**
     * Configure and start the group's interruption source.
     *
     * @param[in] time the time to set to the next interruption.
     *                 Expressed in the group's time unit.
     */
    void (*pf_start_it_source)(TimerTime time);
  }
  TimerGroup;

  static void timer_rtc_date_has_changed(const Datetime *pv_dt_old,
					 const Datetime *pv_dt_new,
					 bool            offset_positive,
					 uint32_t        offset_secs_abs,
					 uint16_t        offset_ms);

  static void timer_schedule(          TimerGroup *pv_group);
  static void timer_schedule_with_time(TimerGroup *pv_group, TimerTime time);

  static void timer_set_has_timed_out( Timer *pv_timer);
  static void timer_exec_callback(     Timer *pv_timer);

  static void timer_ms_stop_rtc_alarm(  void);
  static void timer_ms_set_rtc_alarm(   TimerTime time);
  static void timer_secs_stop_rtc_alarm(void);
  static void timer_secs_set_rtc_alarm( TimerTime time);

  static void timer_ms_irq(       void);
  static void timer_sec_irq(      void);
  static void timer_irq_for_group(TimerGroup *pv_group);

  static TimerTime timer_secs_source(void);
  static TimerTime timer_ms_source(  void);


  /**
   * Stores the timer groups. They are indexed by the time units.
   */
  static TimerGroup _timer_groups[TIMER_UNITS_COUNT] =
  {
      {
	  TIMER_TU_MSECS,
	  timer_ms_source,
	  3,     // period_min
	  NULL,
	  timer_ms_stop_rtc_alarm,
	  timer_ms_set_rtc_alarm
      },
      {
	  TIMER_TU_SECS,
	  timer_secs_source,
	  1,     // period_min
	  NULL,
	  timer_secs_stop_rtc_alarm,
	  timer_secs_set_rtc_alarm
      }
  };

  /**
   * Object used to watch RTC date changes
   */
  static RTCDateWatcher _timer_rtc_date_watcher =
  {
      "timer",
      timer_rtc_date_has_changed,
      NULL
  };

  static Timer *_pv_timer_timed_out_timers_head = NULL;   ///< Points to the head of the list of the timed out timers.
  static Timer *_pv_timer_timed_out_timers_tail = NULL;   ///< Points to the tail of the list of the timed out timers.
  static bool   _timer_has_been_initialised     = false;  ///< Indicate if the timer mode has been initialised.


#define timer_set_is_running(     pv_timer)  (pv_timer)->status |=  TIMER_STATUS_IS_RUNNING
#define timer_clear_is_running(   pv_timer)  (pv_timer)->status &= ~TIMER_STATUS_IS_RUNNING
#define timer_clear_has_timed_out(pv_timer)  (pv_timer)->status &= ~TIMER_STATUS_HAS_TIMED_OUT

#define timer_s_group(pv_timer)  (&_timer_groups[timer_time_unit_from_status((pv_timer)->status)])


  static TimerTime timer_secs_source(void) { return (TimerTime)rtc_get_date_as_secs_since_2000();  }
  static TimerTime timer_ms_source(  void) { return (TimerTime)rtc_get_date_as_ticks_since_2000(); }


  /**
   * Process timers that have timed out since last time this function has been called.
   *
   * Basically, this function calls the callback functions of the timer that have timed out.
   * And it restarts timer that are periodic.
   *
   * @return true if there are immediate processing left to do.
   * @return false otherwise.
   */
  bool timer_process(void)
  {
    Timer *pv_timer;
    while(1)
    {
      ENTER_CRITICAL_SECTION();
      // Get the timed out tail timer
      if(!(pv_timer = _pv_timer_timed_out_timers_tail))
      {
	EXIT_CRITICAL_SECTION();
	break;
      }

      // Remove the timer from the timed out timer list
      if(pv_timer->pv_previous)
      {
	pv_timer->pv_previous->pv_next  = NULL;
	_pv_timer_timed_out_timers_tail = pv_timer->pv_previous;
      }
      else { _pv_timer_timed_out_timers_head = _pv_timer_timed_out_timers_tail = NULL; }
      pv_timer->pv_previous = pv_timer->pv_next = NULL;
      EXIT_CRITICAL_SECTION();

      // Execute callback
      timer_exec_callback(pv_timer);

      // If timer is periodic then restart it.
      if(timer_is_periodic(pv_timer)) { timer_start(pv_timer); }
    }

    return false;
  }

  /**
   * Execute a timer's callback function, if one is set.
   */
  static void timer_exec_callback(Timer *pv_timer)
  {
    switch(pv_timer->cb.type)
    {
      case TIMER_CB_TYPE_NO_ARGS: pv_timer->cb.no_args.pf_cb();                          break;
      case TIMER_CB_TYPE_PV_ARG:  pv_timer->cb.pv_arg.pf_cb(pv_timer->cb.pv_arg.pv_arg); break;
      case TIMER_CB_TYPE_NONE:  // Do nothing
      default:
	// Default case should not happen.
	break;
    }
  }


  /**
   * Initialises the timer software module.
   */
  void _timer_init(void)
  {
    if(_timer_has_been_initialised) { goto exit; }

    // Make sure that the RTC is initialised.
    rtc_init();

    // Register to RTC to detect time changes
    rtc_register_date_watcher(&_timer_rtc_date_watcher);

    // Set RTC IRQ handlers
    rtc_set_irq_handler(TIMER_MS_RTC_ALARM_IRQ_ID,  timer_ms_irq,  true);
    rtc_set_irq_handler(TIMER_SEC_RTC_ALARM_IRQ_ID, timer_sec_irq, true);

    _timer_has_been_initialised = true;

    exit:
    return;
  }


  /**
   * Initialise a timer object with no callback function.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   * @param[in] period   the timer's period. The time unit depends on the timmer's options.
   * @param[in] options  the timer's options. There MUST be at least the time unit used by the timer.
   */
  void timer_init(Timer *pv_timer, TimerTime period, TimerOptions options)
  {
    if(!_timer_has_been_initialised) { _timer_init(); }

    pv_timer->status      = options;
    pv_timer->tref        = 0;
    pv_timer->pv_previous = NULL;
    pv_timer->pv_next     = NULL;
    pv_timer->cb.type     = TIMER_CB_TYPE_NONE;

    switch(timer_time_unit_from_options(options))
    {
      case TIMER_TU_MSECS: pv_timer->period = rtc_ms_to_ticks(period); break;
      default:             pv_timer->period = period;                  break;
    }
    pv_timer->timeout = pv_timer->period;
  }


  /**
   * Remove current callback from a timer object if there is one.
   *
   * @pre the timer must not be running.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   */
  void timer_set_callback_none(Timer *pv_timer)
  {
    if(timer_is_running(pv_timer)) { goto exit; }

    pv_timer->cb.type = TIMER_CB_TYPE_NONE;

    exit:
    return;
  }

  /**
   * Set a callback function that takes no argument.
   *
   * @pre the timer must not be running.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   * @param[in] pf_cb    the callback function. If NULL then remove current callback if there is one.
   */
  void timer_set_callback_no_arg(Timer *pv_timer, void (pf_cb)(void))
  {
    if(timer_is_running(pv_timer)) { goto exit; }

    if(pf_cb)
    {
      pv_timer->cb.type          = TIMER_CB_TYPE_NO_ARGS;
      pv_timer->cb.no_args.pf_cb = pf_cb;
    }
    else { pv_timer->cb.type = TIMER_CB_TYPE_NONE; }

    exit:
    return;
  }

  /**
   * Set a callback function that takes a single void * typed argument.
   *
   * @pre the timer must not be running.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   * @param[in] pf_cb    the callback function. If NULL then remove current callback if there is one.
   * @param[in] pv_arg   the argument to pass to the callback function. Can be NULL.
   */
  void timer_set_callback_pv_arg(Timer *pv_timer, void (pf_cb)(void *pv_arg), void *pv_arg)
  {
    if(timer_is_running(pv_timer)) { goto exit; }

    if(pf_cb)
    {
      pv_timer->cb.type          = TIMER_CB_TYPE_PV_ARG;
      pv_timer->cb.pv_arg.pf_cb  = pf_cb;
      pv_timer->cb.pv_arg.pv_arg = pv_arg;
    }
    else { pv_timer->cb.type = TIMER_CB_TYPE_NONE; }

    exit:
    return;
  }


  /**
   * Sets the timer's period.
   *
   * @pre the timer MUST be stopped.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   * @param[in] period   the period to use.
   * @param[in] unit     the period's time unit.
   */
  void timer_set_period(Timer *pv_timer, TimerTime period, TimerTimeUnit unit)
  {
    pv_timer->status &= ~TIMER_STATUS_TIME_UNIT_MASK;
    pv_timer->status |= unit;

    switch(unit)
    {
      case TIMER_TU_MSECS: pv_timer->period = rtc_ms_to_ticks(period); break;
      default:             pv_timer->period = period;                  break;
    }
    pv_timer->timeout = pv_timer->period;
  }


  /**
   * Stop a timer.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   */
  void timer_stop(Timer *pv_timer)
  {
    TimerGroup *pv_group;

    ENTER_CRITICAL_SECTION();

    // Check if the timer already is stopped.
    if(!timer_is_running(pv_timer)) { goto exit; }

    // Remove timer from it's group list
    if(pv_timer->pv_previous)
    {
      // The timer is not the front runner.
      pv_timer->pv_previous->pv_next = pv_timer->pv_next;
    }
    else
    {
      // The timer is the front running one
      pv_group          = timer_s_group(pv_timer);
      pv_group->pv_head = pv_timer->pv_next;

      // Schedule next timer.
      timer_schedule(pv_group);
    }

    // The timer is no longer running
    pv_timer->pv_previous = pv_timer->pv_next = NULL;
    timer_clear_is_running(pv_timer);

    exit:
    EXIT_CRITICAL_SECTION();
    return;
  }

  /**
   * Start a timer.
   *
   * @pre the timer MUST have been initialised first.
   */
  void timer_start(Timer *pv_timer)
  {
    Timer      *pv_t, *pv_prev;
    TimerGroup *pv_group;
    uint32_t    time, timeout, time_left;

    ENTER_CRITICAL_SECTION();

    // Check if the timer already is running.
    if(timer_is_running(pv_timer)) { goto exit; }
    timer_clear_has_timed_out(pv_timer);

    // Get the timer's group
    pv_group = timer_s_group(pv_timer);

    // Set timer reference time
    pv_timer->tref = time = pv_group->pf_time();

    // For all time units 0 is an absolute reference.
    // For the case of seconds for example 0 means 2001-01-01T00:00:00, that is midnight.
    // So for absolute period we can simply use a modulo operation.

    // Correct timeout value for absolute periods
    timeout = pv_timer->period;
    if(timer_is_absolute(pv_timer)) { timeout -= time % timeout; }
    pv_timer->timeout = timeout;

    // Insert timer in the group's timer list.
    // Look for insertion point.
    for(pv_prev = NULL, pv_t = pv_group->pv_head; pv_t; pv_prev = pv_t, pv_t = pv_t->pv_next)
    {
      // Compute time left
      time_left = time - pv_t->tref;
      time_left = time_left <= pv_t->timeout ? pv_t->timeout - time_left : 0;

      // Compare
      if(time_left > timeout) break;
    }
    if(!pv_prev) { goto set_timer_as_front_runner; }

    // Insert
    pv_timer->pv_previous = pv_prev;
    pv_timer->pv_next     = pv_prev ->pv_next;
    pv_prev ->pv_next     = pv_timer;
    if(pv_timer->pv_next) { pv_timer->pv_next->pv_previous = pv_timer; }
    timer_set_is_running(   pv_timer);
    goto exit;

    set_timer_as_front_runner:
    pv_timer->pv_previous =  NULL;
    pv_timer->pv_next     =  pv_group->pv_head;
    pv_group->pv_head     =  pv_timer;
    timer_set_is_running(    pv_timer);
    timer_schedule_with_time(pv_group, time);

    exit:
    EXIT_CRITICAL_SECTION();
    return;
  }

  /**
   * Starts a timer using a given period.
   *
   * @pre the timer MUST be new or MUST has been stopped before calling this function.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   * @param[in] period   the period to use.
   * @param[in] unit     the period's time unit.
   */
  void timer_start_with_period(Timer *pv_timer, TimerTime period, TimerTimeUnit unit)
  {
    timer_set_period(pv_timer, period, unit);
    timer_start(     pv_timer);
  }


  /**
   * Restarts a timer. Stop it and restart it it's current parameters.
   *
   * @param[in] pv_timer the time. MUST be NOT NULL.
   */
  void timer_restart(Timer *pv_timer)
  {
    timer_stop( pv_timer);
    timer_start(pv_timer);
  }


  /**
   * Set a timer as timed out.
   *
   * The timer is removed from it's group and placed in the list of the timed out timers.
   * It's status is changed from running to timed out.
   * But it's callback function, if one is set, is not called.
   * Calling the callback is done in the timer_process() function that you have to call.
   *
   * @param[in] pv_timer the timer. MUST be NOT NULL.
   */
  static void timer_set_has_timed_out(Timer *pv_timer)
  {
    // Change status
    pv_timer->status &= ~TIMER_STATUS_IS_RUNNING;
    pv_timer->status |=  TIMER_STATUS_HAS_TIMED_OUT;

    // If the timer has a callback then we will need to call it later in the timer_process() function.
    // Or if it is a periodic one.
    if(pv_timer->cb.type != TIMER_CB_TYPE_NONE || timer_is_periodic(pv_timer))
    {
      // Move it to the timed out list for the timer_process() function.
      pv_timer->pv_previous = NULL;
      pv_timer->pv_next     = _pv_timer_timed_out_timers_head;
      if(_pv_timer_timed_out_timers_head) { _pv_timer_timed_out_timers_head->pv_previous = pv_timer; }
      else                                { _pv_timer_timed_out_timers_tail              = pv_timer; }
      _pv_timer_timed_out_timers_head = pv_timer;
    }
    else { pv_timer->pv_previous = pv_timer->pv_next = NULL; }
  }


  /**
   * Schedule next interruption for the next timer to time out in a given timer group.
   *
   * @param[in] pv_group the timer group. MUST be NOT NULL.
   */
  static void timer_schedule(TimerGroup *pv_group)
  {
    timer_schedule_with_time(pv_group, pv_group->pf_time());
  }

  /**
   * Schedule next interruption for the next timer to time out in a given timer group.
   *
   * @param[in] pv_group the timer group. MUST be NOT NULL.
   * @param[in] time     the current time, in the time unit used by the group.
   */
  static void timer_schedule_with_time(TimerGroup *pv_group, TimerTime time)
  {
    TimerTime time_left;
    Timer    *pv_timer;

    // Is there a timer to start?
    if(!(pv_timer = pv_group->pv_head))
    {
      // No
      pv_group->pf_stop_it_source();
      goto exit;
    }

    // Compute time left to interruption
    time_left = time - pv_timer->tref;
    time_left = time_left <= pv_timer->timeout ?
	pv_timer->timeout - time_left :
	pv_group->period_min;
    if(time_left < pv_group->period_min) { time_left = pv_group->period_min; }

    // Program RTC alarm for front running timer.
    pv_group->pf_start_it_source(time_left);

    exit:
    return;
  }

  static void timer_ms_stop_rtc_alarm(void)
  {
    rtc_stop_alarm(TIMER_MS_RTC_ALARM_ID);
  }
  static void timer_ms_set_rtc_alarm(TimerTime time)
  {
    rtc_set_alarm_relative_ticks(TIMER_MS_RTC_ALARM_ID, time);
  }
  static void timer_secs_stop_rtc_alarm(void)
  {
    rtc_stop_alarm(TIMER_SEC_RTC_ALARM_ID);
  }
  static void timer_secs_set_rtc_alarm(TimerTime time)
  {
    rtc_set_alarm_relative_secs(TIMER_SEC_RTC_ALARM_ID, (uint32_t)time);
  }


  /**
   * Function called when the RTC time has been changed.
   *
   * @param[in] pd_dt_old        the old time. MUST be NOT NULL.
   * @param[in] pv_dt_new        the new time. MUST be NOT NULL.
   * @param[in] offset_positive  indicate if the offset is positive or not
   *                             (new date is greater than old one or not).
   * @param[in] offset_secs_abs  the offset's absolute value number of seconds.
   * @param[in] offset_ms_abs    the offset's absolute value number of milliseconds.
   */
  static void timer_rtc_date_has_changed(const Datetime *pv_dt_old,
					 const Datetime *pv_dt_new,
  					 bool            offset_positive,
  					 uint32_t        offset_secs_abs,
					 uint16_t        offset_ms_abs)
  {
    TimerTime   offset_ticks_abs;
    Timer      *pv_timer;
    TimerGroup *pv_group;
    UNUSED(pv_dt_old);
    UNUSED(pv_dt_new);

    // Convert the offset to ticks for the milliseconds timers
    offset_ticks_abs = (TimerTime)(rtc_secs_to_ticks(offset_secs_abs) + rtc_ms_to_ticks(offset_ms_abs));

    ENTER_CRITICAL_SECTION();

    // Update time references in all groups related to RTC.
    // Milliseconds timers
    pv_group = &_timer_groups[TIMER_TU_MSECS];
    pv_group->pf_stop_it_source();
    for(pv_timer = pv_group->pv_head; pv_timer; pv_timer = pv_timer->pv_next)
    {
      if(offset_positive) { pv_timer->tref += offset_ticks_abs; }
      else                { pv_timer->tref -= offset_ticks_abs; }
    }
    timer_schedule(pv_group);  // So that a new alarm value is set.

    // Seconds timers
    pv_group = &_timer_groups[TIMER_TU_SECS];
    pv_group->pf_stop_it_source();
    for(pv_timer = pv_group->pv_head; pv_timer; pv_timer = pv_timer->pv_next)
    {
      if(offset_positive) { pv_timer->tref += offset_secs_abs; }
      else                { pv_timer->tref -= offset_secs_abs; }
    }
    timer_schedule(pv_group);  // So that a new alarm value is set.

    EXIT_CRITICAL_SECTION();
  }


  /**
   * Called when the milliseconds time source generates an interruption.
   */
  static void timer_ms_irq(void)
  {
    timer_irq_for_group(&_timer_groups[TIMER_TU_MSECS]);
  }

  /**
   * Called when the seconds time source generates an interruption.
   */
  static void timer_sec_irq(void)
  {
    timer_irq_for_group(&_timer_groups[TIMER_TU_SECS]);
  }

  /**
   * Call to process an interruption on a group.
   *
   * The interruption means that the front runner timer has timed out.
   * As well as all the next timers that have the save time out time; if there are any.
   *
   * @param[in] pv_group the timer group to process. MUST be NOT NULL.
   */
  static void timer_irq_for_group(TimerGroup *pv_group)
  {
    Timer     *pv_timer, *pv_next;
    TimerTime time;

    if(!(pv_timer = pv_group->pv_head)) { goto exit; }  // This should not happen.

    // Get time
    time = pv_group->pf_time();

    // Go through all the timers at the top of the list that have expired
    // The front timer at the top of the list always is time out; but there might be others.
    for( ; pv_timer && (time - pv_timer->tref >= pv_timer->timeout); pv_timer = pv_next)
    {
      pv_next = pv_timer->pv_next;  // Save pv_timer->pv_next before it is overwritten.
      timer_set_has_timed_out(pv_timer);
    }
    if(pv_timer) { pv_timer->pv_previous = NULL; }
    pv_group->pv_head = pv_timer;

    // Schedule next timeout
    timer_schedule_with_time(pv_group, time);

#ifdef TIMER_PROCESS_IN_IRQ
    timer_process();
#else
    if(pv_timer->status & TIMER_PROCESS_IN_IRQ) { timer_process(); }
#endif

    exit:
    return;
  }



#ifdef __cplusplus
}
#endif
