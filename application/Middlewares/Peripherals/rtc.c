/**
 * RTC interface.
 *
 * @note the RTC is used to get 'ticks', the HAL functions using Systicks
 * are overwritten here by functions using the RTC.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "rtc.h"
#include "board.h"
#include "sdcard.h"
#include "logger.h"
#include "nodeinfo.h"


#ifdef __cplusplus
extern "C" {
#endif


  CREATE_LOGGER(rtc);
#undef  logger
#define logger  rtc


//#define USE_SHADOW_REGS
#define RTC_YEARS_OFFSET  2000

#define RTC_MINIMUM_TIMEOUT_TICKS  3

#define N_PREDIV_S  10                              ///< Subsecond number of bits
#define PREDIV_S    ((1 << N_PREDIV_S)        - 1)  ///< Synchonuous prediv
#define PREDIV_A    ((1 << (15 - N_PREDIV_S)) - 1)  ///< Asynchonuous prediv

  //Sub-second mask definition
#if  (N_PREDIV_S == 10)
#define ALARMSUBSECONDMASK RTC_ALARMSUBSECONDMASK_SS14_10
#else
#error "Please define ALARMSUBSECONDMASK"
#endif


#define RTC_CAL_OFFSET_PPM_MAX     488
#define RTC_CAL_OFFSET_PPM_MIN   (-487)
#define RTC_CAL_OFFSET_PPM_FILE  RTC_PRIVATE_DIR_NAME "/cal_offset_ppm"


  /*
   * NOTE: deactivate write protection because it messes up LoRaWAN communication when the date
   * is changed for example.
   * The long term-solution would be to handle the write protection in the timeserver code.
   * Unless we are fine with unprotected RTC register, which should be fine.
   */
#define DEACTIVATE_RTC_DOMAIN_WRITE_PROTECTION

  /**
   * An interruption context.
   */
  typedef struct RTCIrqCtx
  {
    bool          enabled;    ///< Is the interruption enabled?
    RTCIrqHandler pf_handler; ///< The interruption handler.
  }
  RTCIrqCtx;


  static RTC_HandleTypeDef _rtc;
  static bool              _rtc_first_run_since_power_up;
  static RTCIrqCtx         _rtc_irq_handlers[RTC_IRQ_ID_COUNT];
  static bool              _rtc_has_been_initialised = false;
  static RTCDateWatcher   *_pv_rtc_date_whatchers    = NULL;

  static const uint8_t _rtc_days_in_month[]           = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  static const uint8_t _rtc_days_in_month_leap_year[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  static void     rtc_read_date_and_time(RTC_DateTypeDef *pv_date, RTC_TimeTypeDef *pv_time);

  static uint32_t rtc_read_backup_register( RTCBackupRegister reg);
  static void     rtc_write_backup_register(RTCBackupRegister reg, uint32_t v);

  static bool rtc_set_calibration_value(uint32_t calr);

  static void rtc_set_alarm_relative(RTCAlarmId alarm, uint32_t secs, RTCTicks sub_secs);


/*
 * NOTE: The macro used to protect the backup domain is de-activated
 */
#define enable_write_in_backup_domain()   PWR->CR1 |=  PWR_CR1_DBP
#ifdef DEACTIVATE_RTC_DOMAIN_WRITE_PROTECTION
#define disable_write_in_backup_domain()  // Do nothing
#else
#define disable_write_in_backup_domain()  PWR->CR1 &= ~PWR_CR1_DBP
#endif



void rtc_init(void)
{
  uint32_t v;

  if(_rtc_has_been_initialised) { return; }

  __HAL_RCC_RTC_ENABLE();

  // Initialise RTC
  _rtc.Instance = RTC;
#ifndef USE_SHADOW_REGS
  HAL_RTCEx_EnableBypassShadow(&_rtc);
#endif

  // Access to backup register MUST be performed after the RTC (backup domain) clock source has been set.

  // Check if this is the first run since the power up.
  v = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);
  if((_rtc_first_run_since_power_up = ((v & RTC_SYSTEM_STATUS_NOT_FIRST_RUN_SINCE_POWER_UP) == 0)))
  {
    // Set the flag so that the next time around we do not detect a first run.
    rtc_write_backup_register(RTC_BKUPREG_SYS_STATUS, v | RTC_SYSTEM_STATUS_NOT_FIRST_RUN_SINCE_POWER_UP);

    _rtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    _rtc.Init.AsynchPrediv   = PREDIV_A;
    _rtc.Init.SynchPrediv    = PREDIV_S;
    _rtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    _rtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    _rtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    _rtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    HAL_RTC_Init(&_rtc);
  }

  // Enable interruptions
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

  rtc_stop_alarm(RTC_ALARM_ID_ALARM_A);
  rtc_stop_alarm(RTC_ALARM_ID_ALARM_B);

  // Clear EXTI line's flag for RTC alarm.
  __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();

  _rtc_has_been_initialised = true;
}


/**
 * Register a date watcher.
 *
 * @param[in] pv_watcher the date watcher to register. MUST be NOT NULL.
 */
void rtc_register_date_watcher(RTCDateWatcher *pv_watcher)
{
  pv_watcher->pv_next    = _pv_rtc_date_whatchers;
  _pv_rtc_date_whatchers = pv_watcher;
}


/**
 * Calibrate the RTC clock using an offset from base, in ppm.
 *
 * @param[in] ppm  the offset to use.
 * @param[in] save save the offset in a persistent way so that it can be restored at power up?
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool rtc_set_calibration_offset_ppm(int16_t ppm, bool save)
{
  File     file;
  uint32_t cal  = 0;
  uint32_t plus = RTC_SMOOTHCALIB_PLUSPULSES_RESET;
  bool     res  = true;

  if(ppm < RTC_CAL_OFFSET_PPM_MIN || ppm > RTC_CAL_OFFSET_PPM_MAX) { res = false; goto exit; }

  // Compute registers' values.
  if(     ppm > 0)
  {
    plus = RTC_SMOOTHCALIB_PLUSPULSES_SET;
    cal  = 512 - (((uint32_t)  ppm)  << 10) / ((uint32_t)(0.954 * 1024.0));
  }
  else if(ppm < 0)
  {
    cal  =       (((uint32_t)(-ppm)) << 10) / ((uint32_t)(0.954 * 1024.0));
  }
  // Else, ppm is 0, do nothing

  // Set the registers' values
  cal |= RTC_SMOOTHCALIB_PERIOD_32SEC | plus;
  if(_rtc.Instance->CALR != cal)
  {
    res = rtc_set_calibration_value(cal);
  }
  // Else do nothing

  // Save value if we are asked to
  if(save)
  {
    if(sdcard_fopen(&file, RTC_CAL_OFFSET_PPM_FILE, FILE_TRUNCATE | FILE_WRITE))
    {
      res &= sdcard_fwrite(&file, (uint8_t *)_rtc.Instance->CALR, 4);
      sdcard_fclose(&file);
    }
    else { res = false; }
  }

  exit:
  return res;
}


/**
 * Set a calibration value in the RTC smooth calibration register.
 *
 * @param[in] calr the register's value.
 *
 * @return true  on success.
 * @return false otherwise.
 */
static bool rtc_set_calibration_value(uint32_t calr)
{
  uint32_t tref;
  bool     res = true;

  // Check value
  if((calr & (RTC_SMOOTHCALIB_PERIOD_16SEC | RTC_SMOOTHCALIB_PERIOD_8SEC)) ==
      (RTC_SMOOTHCALIB_PERIOD_16SEC | RTC_SMOOTHCALIB_PERIOD_8SEC))
  {
    // Error those bits cannot be set at the same time
    res = false;
    goto exit;
  }

  // Disable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_DISABLE(&_rtc);

  // Check if a calibration is pending
  if(_rtc.Instance->ISR & RTC_ISR_RECALPF)
  {
    tref = board_ms_now();
    while(_rtc.Instance->ISR & RTC_ISR_RECALPF)
    {
      if(board_is_timeout(tref, RTC_TIMEOUT_VALUE))
      {
	res = false;
	goto exit;
      }
    }
  }

  // Set register
  _rtc.Instance->CALR = calr;

  exit:
  // Enable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_ENABLE(&_rtc);

  return res;
}

/**
 * Load and set RTC calibration from persistent storage.
 *
 * @return true  on success.
 * @return true  if there are no calibration data.
 * @return false on error.
 */
bool rtc_load_and_set_calibration()
{
  File     file;
  uint32_t value;
  int16_t  ppm = 0;
  bool     res = true;

  // Read the file's content
  if(sdcard_fopen(&file, RTC_CAL_OFFSET_PPM_FILE, FILE_OPEN))
  {
    res  = sdcard_fread(&file, (uint8_t *)&value, 4);
    sdcard_fclose(&file);
    res &= rtc_set_calibration_value(value);
  }
  else
  {
    switch(nodeinfo_main_board_rtc_cap())
    {
#ifdef RTC_CAL_PPM_VALUE_CAP_12PF
      case NODEINFO_RTC_CAP_12PF: ppm = RTC_CAL_PPM_VALUE_CAP_12PF; break;
#endif
#ifdef RTC_CAL_PPM_VALUE_CAP_20PF
      case NODEINFO_RTC_CAP_20PF: ppm = RTC_CAL_PPM_VALUE_CAP_20PF; break;
#endif
      case NODEINFO_RTC_CAP_UNKNOWN:
      default:
	break;
    }
    if(ppm) { rtc_set_calibration_offset_ppm(ppm, false); }
  }

  return res;
}

/**
 * Return the current RTC calibration offset, in ppm.
 *
 * @return the current offset.
 */
int32_t rtc_calibration_offset_ppm()
{
  int32_t ppm;

  if(_rtc.Instance->CALR & RTC_SMOOTHCALIB_PLUSPULSES_SET)
  {
    ppm = ((512 - (_rtc.Instance->CALR & RTC_CALR_CALM)) * (int32_t)(0.954 * 1024)) >> 10;
  }
  else
  {
    ppm = ((_rtc.Instance->CALR & RTC_CALR_CALM) * (int32_t)(0.954 * 1024)) >> 10;
  }

  return ppm;
}


/**
 * Indicate if this is the first run since the power up.
 *
 * @return true  if it is.
 * @return false otherwise.
 */
bool rtc_is_first_run_since_power_up(void) { return _rtc_first_run_since_power_up; }

/**
 * Once this function has been called then we no longer consider this is the first run since power up.
 */
void rtc_set_is_not_first_run(void) { _rtc_first_run_since_power_up = false; }


/**
 * Read date and time from RTC.
 *
 * @param[out] pv_date where to write the date information.
 *                     Can be NULL if we are not interested in this information.
 * @param[out] pv_time where the time is written to.
 *                     Can be NULL if we are not interested in this information.
 */
static void rtc_read_date_and_time(RTC_DateTypeDef *pv_date, RTC_TimeTypeDef *pv_time)
{

#ifndef USE_SHADOW_REGS
  RTC_TimeTypeDef _time;
  uint32_t first_read;

  if(!pv_time) { pv_time = &_time; }

  HAL_RTC_GetTime(  &_rtc, pv_time, RTC_FORMAT_BIN);
  do
  {
    first_read = pv_time->SubSeconds;
    if(pv_date) { HAL_RTC_GetDate(&_rtc, pv_date, RTC_FORMAT_BIN); }
    HAL_RTC_GetTime(              &_rtc, pv_time, RTC_FORMAT_BIN);
  }
  while(first_read != pv_time->SubSeconds);
#else
  if(pv_time) { HAL_RTC_GetTime(&_rtc, &rtc_time, RTC_FORMAT_BIN); }
  if(pv_date) { HAL_RTC_GetDate(&_rtc, &rtc_date, RTC_FORMAT_BIN); }
#endif
}

void rtc_get_date(Datetime *pv_time)
{
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;

  rtc_read_date_and_time(&rtc_date, &rtc_time);

  pv_time->day         = rtc_date.Date;
  pv_time->month       = rtc_date.Month;
  pv_time->year        = rtc_date.Year + RTC_YEARS_OFFSET;
  pv_time->hours       = rtc_time.Hours;
  pv_time->minutes     = rtc_time.Minutes;
  pv_time->seconds     = rtc_time.Seconds;
  pv_time->sub_seconds = rtc_ticks_to_ms(PREDIV_S - rtc_time.SubSeconds);
}

/**
 * Update the RTC time and get the time offset to current time.
 *
 * The offset if +/-(offset_secs + offset_ms).
 *
 * @param[in]  pv_time            the new time to set. MUST be NOT NULL.
 * @param[out] pb_offset_positive set to true if the time offset is positive
 *                                (new time grater that old one), set to false otherwise.
 *                                Can be set to NULL if you are not interested in this information.
 * @param[out] pu32_offset_secs   where the absolute value of the time offset, in seconds,
 *                                is written to.
 *                                Can be set to NULL if you are not interested in this information.
 * @param[out] pu16_offset_ms     where the absolute value of the milliseconds offset is written to.
 *                                Can be set to NULL if you are not interested in this information.
 */
bool rtc_set_date_get_offset(const Datetime *pv_time,
		  bool           *pb_offset_positive,
		  uint32_t       *pu32_offset_secs,
		  uint16_t       *pu16_offset_ms)
{
  Datetime        dt_now;
  ts2000_t        ts_old, ts_new;
  bool            offset_positive;
  uint32_t        offset_secs_abs;
  int32_t         offset_ms_abs;
  RTCDateWatcher *pv_watcher;
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;

  // If there are date watchers, then compute offset.
  if(_pv_rtc_date_whatchers || pb_offset_positive || pu32_offset_secs || pu16_offset_ms)
  {
    rtc_get_date(&dt_now);
    ts_old = datetime_to_timestamp_sec_2000(&dt_now);
    ts_new = datetime_to_timestamp_sec_2000(pv_time);
    if((offset_positive = (ts_new > ts_old)))
    {
      offset_secs_abs = ts_new - ts_old;
      offset_ms_abs   = pv_time->sub_seconds - dt_now.sub_seconds;
    }
    else
    {
      offset_secs_abs = ts_old - ts_new;
      offset_ms_abs   = dt_now.sub_seconds - pv_time->sub_seconds;
    }
    if(offset_ms_abs < 0)
    {
	offset_secs_abs--;
	offset_ms_abs = 1000 + offset_ms_abs;
    }
  }

  // Set up new time
  rtc_time.Hours          = pv_time->hours;
  rtc_time.Minutes        = pv_time->minutes;
  rtc_time.Seconds        = pv_time->seconds;
  rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  rtc_time.TimeFormat     = 0;
  rtc_time.SecondFraction = 0;
  rtc_time.StoreOperation = 0;
  rtc_time.SubSeconds     = PREDIV_S - rtc_ms_to_ticks(pv_time->sub_seconds);
  rtc_date.Date           = pv_time->day;
  rtc_date.Month          = pv_time->month;
  rtc_date.Year           = pv_time->year - RTC_YEARS_OFFSET;
  rtc_date.WeekDay        = 1;

  enable_write_in_backup_domain();
  HAL_RTC_SetTime(&_rtc, &rtc_time, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&_rtc, &rtc_date, RTC_FORMAT_BIN);
  disable_write_in_backup_domain();

  // Signal the date watchers, if there are any
  for(pv_watcher = _pv_rtc_date_whatchers; pv_watcher; pv_watcher = pv_watcher->pv_next)
  {
    pv_watcher->pf_signal_date_change(&dt_now,
				      pv_time,
				      offset_positive,
				      offset_secs_abs,
				      (uint16_t)offset_ms_abs);
  }

  // Output offset information if we are asked to
  if(pb_offset_positive) { *pb_offset_positive = offset_positive;         }
  if(pu32_offset_secs)   { *pu32_offset_secs   = offset_secs_abs;         }
  if(pu16_offset_ms)     { *pu16_offset_ms     = (uint16_t)offset_ms_abs; }

  log_info(logger,
	   "RTC time has been changed to: %04d-%02d-%02dT%02d:%02d:%02d.",
	   pv_time->year,  pv_time->month,   pv_time->day,
	   pv_time->hours, pv_time->minutes, pv_time->seconds);

  return true;
}


ts2000_t rtc_get_date_as_secs_since_2000(void)
{
  Datetime dt;

  rtc_get_date(&dt);
  return datetime_to_timestamp_sec_2000(&dt);
}

RTCTicks rtc_get_date_as_ticks_since_2000(void)
{
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;
  Datetime        dt;
  RTCTicks        res;

  rtc_read_date_and_time(&rtc_date, &rtc_time);

  // Set up Datetime object to get the number of seconds since 2000
  dt.year    = rtc_date.Year + RTC_YEARS_OFFSET;
  dt.month   = rtc_date.Month;
  dt.day     = rtc_date.Date;
  dt.hours   = rtc_time.Hours;
  dt.minutes = rtc_time.Minutes;
  dt.seconds = rtc_time.Seconds;
  res        = datetime_to_timestamp_sec_2000(&dt);

  // Add the sub-seconds
  res = (res << N_PREDIV_S) + (PREDIV_S - rtc_time.SubSeconds);

  return res;
}

/**
 * Convert a milliseconds value to a number of RTC ticks.
 *
 * @param[in] ms the number of milliseconds.
 *
 * @return the number of ticks.
 */
RTCTicks rtc_ms_to_ticks(uint32_t ms)
{
  return (RTCTicks)((((RTCTicks)ms) * (1u << N_PREDIV_S)) / 1000);
}

/**
 * Convert a seconds value to a number of RTC ticks.
 *
 * @param[in] secs the number of seconds.
 *
 * @return the number of ticks.
 */
RTCTicks rtc_secs_to_ticks(uint32_t secs)
{
  return (RTCTicks)(((RTCTicks)secs) * (1u << N_PREDIV_S));
}

/**
 * Convert a RTC tick count to the corresponding number of milliseconds.
 *
 * @param[in] ticks the number of ticks.
 *
 * @return the corresponding number of milliseconds.
 */
uint32_t rtc_ticks_to_ms(RTCTicks ticks)
{
  return (uint32_t)((ticks * 1000) / (1u << N_PREDIV_S));
}


/**
 * Stop a given alarm.
 *
 * @param[in] alarm the alarm to stop.
 */
void rtc_stop_alarm(RTCAlarmId alarm)
{
  switch(alarm)
  {
    case RTC_ALARM_ID_ALARM_A:
      // Clear RTC alarm flag.
      __HAL_RTC_ALARM_CLEAR_FLAG(&_rtc, RTC_FLAG_ALRAF);

      // Disable the alarm interrupt
      HAL_RTC_DeactivateAlarm(&_rtc, RTC_ALARM_A);
      break;

    case RTC_ALARM_ID_ALARM_B:
      // Clear RTC alarm flag.
      __HAL_RTC_ALARM_CLEAR_FLAG(&_rtc, RTC_FLAG_ALRBF);

      // Disable the alarm interrupt
      HAL_RTC_DeactivateAlarm(&_rtc, RTC_ALARM_B);
      break;

    default:
      // Do nothing
      ;
  }
}

/**
 * Set an alarm relative to current time using a time specified in RTC ticks.
 *
 * @param[in] alarm the alarm to set up.
 * @param[in] ticks the number of RTC ticks in the future to set up the alarm to.
 */
void rtc_set_alarm_relative_ticks(RTCAlarmId alarm, RTCTicks ticks)
{
  rtc_set_alarm_relative(alarm, 0, ticks);
}

/**
 * Set an alarm relative to current time using a time specified in seconds.
 *
 * @param[in] alarm the alarm to set up.
 * @param[in] secs  the number of seconds in the future to set up the alarm to.
 */
void rtc_set_alarm_relative_secs( RTCAlarmId alarm, uint32_t secs)
{
  rtc_set_alarm_relative(alarm, secs, 0);
}

static void rtc_set_alarm_relative(RTCAlarmId alarm, uint32_t secs, RTCTicks sub_secs)
{
  RTC_TimeTypeDef  rtc_time;
  RTC_DateTypeDef  rtc_date;
  RTC_AlarmTypeDef init;
  uint32_t         alarm_hal_id;
  char             alarm_c;

  rtc_stop_alarm(alarm);

  // Get current time
  rtc_read_date_and_time(&rtc_date, &rtc_time);

  // Set up sub-seconds; reverse counter
  if(sub_secs)
  {
    rtc_time.SubSeconds  = PREDIV_S - rtc_time.SubSeconds;
    rtc_time.SubSeconds += (((uint32_t)sub_secs) & PREDIV_S);
  }
  else { rtc_time.SubSeconds = 0; }

  // Add extra sub-seconds to seconds
  secs += (uint32_t)(sub_secs >> N_PREDIV_S);

  // Build alarm date from current date
  for( ; secs >= DT_SECONDS_IN_1DAY;    secs -= DT_SECONDS_IN_1DAY,    rtc_date.Date++);
  for( ; secs >= DT_SECONDS_IN_1HOUR;   secs -= DT_SECONDS_IN_1HOUR,   rtc_time.Hours++);
  for( ; secs >= DT_SECONDS_IN_1MINUTE; secs -= DT_SECONDS_IN_1MINUTE, rtc_time.Minutes++);
  rtc_time.Seconds += secs;

  // Correct for modulo
  for( ; rtc_time.SubSeconds >= PREDIV_S + 1;          rtc_time.SubSeconds -= PREDIV_S + 1,          rtc_time.Seconds++);
  for( ; rtc_time.Seconds    >= DT_SECONDS_IN_1MINUTE; rtc_time.Seconds    -= DT_SECONDS_IN_1MINUTE, rtc_time.Minutes++);
  for( ; rtc_time.Minutes    >= DT_MINUTES_IN_1HOUR;   rtc_time.Minutes    -= DT_MINUTES_IN_1HOUR,   rtc_time.Hours++);
  for( ; rtc_time.Hours      >= DT_HOURS_IN_1DAY;      rtc_time.Hours      -= DT_HOURS_IN_1DAY,      rtc_date.Date++);
  if(rtc_date.Year % 4 == 0)
  {
    if(rtc_date.Date > _rtc_days_in_month_leap_year[rtc_date.Month - 1])
    {
      rtc_date.Date = rtc_date.Date % _rtc_days_in_month_leap_year[rtc_date.Month - 1];
    }
  }
  else
  {
    if(rtc_date.Date > _rtc_days_in_month[rtc_date.Month - 1])
    {
      rtc_date.Date = rtc_date.Date % _rtc_days_in_month[rtc_date.Month - 1];
    }
  }

  // Set some alarm variables
  if(alarm == RTC_ALARM_ID_ALARM_A)
  {
    alarm_hal_id = RTC_ALARM_A;
    alarm_c      = 'A';
  }
  else
  {
    alarm_hal_id = RTC_ALARM_B;
    alarm_c      = 'B';
  }

  // Set up the alarm
  init.AlarmTime.SubSeconds     = PREDIV_S - rtc_time.SubSeconds;
  init.AlarmSubSecondMask       = ALARMSUBSECONDMASK;
  init.AlarmTime.Seconds        = rtc_time.Seconds;
  init.AlarmTime.Minutes        = rtc_time.Minutes;
  init.AlarmTime.Hours          = rtc_time.Hours;
  init.AlarmDateWeekDay         = (uint8_t)rtc_date.Date;
  init.AlarmTime.TimeFormat     = rtc_time.TimeFormat;
  init.AlarmDateWeekDaySel      = RTC_ALARMDATEWEEKDAYSEL_DATE;
  init.AlarmMask                = RTC_ALARMMASK_NONE;
  init.Alarm                    = alarm_hal_id;
  init.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  init.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  HAL_RTC_SetAlarm_IT(&_rtc, &init, RTC_FORMAT_BIN);

  log_trace(logger, "Alarm %c set to %04d-%02d-%02d %02d:%02d:%02d.%d",
	    alarm_c,
	    rtc_date.Year + 2000, rtc_date.Month, rtc_date.Date,
	    rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds, rtc_time.SubSeconds);
}


bool rtc_set_wkup_date(const Datetime *pv_time)
{
  // RTC interrupt Init
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

  // Initialisation de l'alarme
  __HAL_RTC_ALARM_CLEAR_FLAG(&_rtc, RTC_FLAG_ALRAF);

  RTC_AlarmTypeDef alarm;
  alarm.AlarmTime.Hours          = pv_time->hours;
  alarm.AlarmTime.Minutes        = pv_time->minutes;
  alarm.AlarmTime.Seconds        = pv_time->seconds;
  alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  alarm.AlarmTime.TimeFormat     = 0;
  alarm.AlarmTime.SecondFraction = 0;
  alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  alarm.AlarmTime.SubSeconds     = 0;
  alarm.Alarm                    = RTC_ALARM_A;
  alarm.AlarmDateWeekDaySel      = RTC_ALARMDATEWEEKDAYSEL_DATE;
  alarm.AlarmSubSecondMask       = RTC_ALARMSUBSECONDMASK_ALL;
  alarm.AlarmDateWeekDay         = pv_time->day;
  alarm.AlarmMask                = RTC_ALARMMASK_NONE;

  HAL_RTC_SetAlarm_IT(&_rtc, &alarm, RTC_FORMAT_BIN);
  return true;
}


static uint32_t rtc_read_backup_register(RTCBackupRegister reg)
{
  return *(volatile uint32_t *)(&_rtc.Instance->BKP0R + reg);
}

static void  rtc_write_backup_register(RTCBackupRegister reg, uint32_t v)
{
  enable_write_in_backup_domain();
  *(volatile uint32_t *)(&_rtc.Instance->BKP0R + reg) = v;
  disable_write_in_backup_domain();
}


/**
 * Indicate if geographical position values are stored in the RTC registers.
 *
 * @return true  if there are.
 * @return false otherwise.
 */
bool rtc_has_geoposition_data(void)
{
  return (rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS) &
      RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA)
      != 0;
}

/**
 * Clear the current geographical position values from the RTC registers.
 */
void rtc_clear_geoposition(void)
{
  uint32_t u32;

  u32 = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);
  if(u32 & RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA)
  {
    // Clear status flag
    u32 &= ~RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA;
    rtc_write_backup_register(RTC_BKUPREG_SYS_STATUS, u32);

    // Set geo values to 0 to be consistent
    rtc_write_backup_register(RTC_BKUPREG_GEOPOS_LATITUDE,  0);
    rtc_write_backup_register(RTC_BKUPREG_GEOPOS_LONGITUDE, 0);
    rtc_write_backup_register(RTC_BKUPREG_GEOPOS_ALTITUDE,  0);
  }
}

/**
 * Get the geographical position values from the RTC backup registers, if there are any.
 *
 * @param[out] pf_latitude  where the latitude value is written to.
 *                          Can be NULL if we are not interested in this value.
 * @param[out] pf_longitude where the longitude value is written to.
 *                          Can be NULL if we are not interested in this value.
 * @param[out] pf_altitude  where the altitude value is written to.
 *                          Can be NULL if we are not interested in this value.
 *
 * @return true  if there are values to get.
 * @return false otherwise. In which case the output variables are left untouched.
 */
bool rtc_geoposition(float *pf_latitude, float *pf_longitude, float *pf_altitude)
{
  uint32_t u32;
  bool     res;

  if((res = rtc_has_geoposition_data()))
  {
    if(pf_latitude)
    {
      u32 = rtc_read_backup_register(RTC_BKUPREG_GEOPOS_LATITUDE);
      *pf_latitude  = *(float *)&u32;
    }
    if(pf_longitude)
    {
      u32 = rtc_read_backup_register(RTC_BKUPREG_GEOPOS_LONGITUDE);
      *pf_longitude = *(float *)&u32;
    }
    if(pf_altitude)
    {
      u32 = rtc_read_backup_register(RTC_BKUPREG_GEOPOS_ALTITUDE);
      *pf_altitude  = *(float *)&u32;
    }
  }

  return res;
}

/**
 * Store a geographical position in the RTC backup registers.
 *
 * @param[in] latitude  the latitude.
 * @param[in] longitude the longitude.
 * @param[in] altitude  the altitude.
 */
void rtc_set_geoposition(float latitude, float longitude, float altitude)
{
  uint32_t u32;

  rtc_write_backup_register(RTC_BKUPREG_GEOPOS_LATITUDE,  *(uint32_t *)&latitude);
  rtc_write_backup_register(RTC_BKUPREG_GEOPOS_LONGITUDE, *(uint32_t *)&longitude);
  rtc_write_backup_register(RTC_BKUPREG_GEOPOS_ALTITUDE,  *(uint32_t *)&altitude);

  // Update status bit if needed
  u32 = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);
  if(!(u32 & RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA))
  {
    u32 |= RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA;
    rtc_write_backup_register(RTC_BKUPREG_SYS_STATUS, u32);
  }
}


/**
 * Get the software reset type from the RTC registers.
 *
 * @param[in] clear Clear the information after reading it (true) or keep it (false).
 *
 * @return the software reset type.
 * @return RTC_SYSTEM_STATUS_SOFTWARE_RESET_NONE if there was no software reset.
 */
RTCSystemStatusValue rtc_software_reset_type(bool clear)
{
  RTCSystemStatusValue res;
  uint32_t             u32;

  // Read system status register
  u32 = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);

  // Get software reset type
  res = u32 & RTC_SYSTEM_STATUS_SOFTWARE_RESET_MASK;

  if(clear) { rtc_software_reset_clear(); }

  return res;
}

/**
 * Set the software reset type in the RTC registers.
 *
 * @param[in] type The software reset type.
 */
void rtc_software_reset_set_type(RTCSystemStatusValue type)
{
  uint32_t u32;

  u32  = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);
  u32 &= ~RTC_SYSTEM_STATUS_SOFTWARE_RESET_MASK;
  u32 |= type;
  rtc_write_backup_register(      RTC_BKUPREG_SYS_STATUS, u32);
}

/**
 * Clear the software reset type stored in the RTC registers.
 */
extern void rtc_software_reset_clear(void)
{
  uint32_t u32;

  u32  = rtc_read_backup_register(RTC_BKUPREG_SYS_STATUS);
  u32 &= ~RTC_SYSTEM_STATUS_SOFTWARE_RESET_MASK;
  rtc_write_backup_register(      RTC_BKUPREG_SYS_STATUS, u32);
}



// ============== functions for time server =====================

uint32_t rtc_get_minimum_timeout_ticks()
{
  return RTC_MINIMUM_TIMEOUT_TICKS;
}



// ============== interruptions handling ========================


/**
 * Set/unset an interruption request handler.
 *
 * @param[in] irq_id     the interruption request identifier. MUST be a valid one.
 * @param[in] pf_handler the function to call on interruption.
 *                       Can be NULL to remove current handler.
 * @param[in] enable     enable interruption?
 */
void rtc_set_irq_handler(RTCIrqId irq_id, RTCIrqHandler pf_handler, bool enable)
{
  _rtc_irq_handlers[irq_id].pf_handler = pf_handler;

  if(!pf_handler) { enable = false; }
  rtc_irq_enable(irq_id, enable);
}

/**
 * Enable/disable interruptions.
 *
 * @param[in] irq_id the interruption request identifier of the interruption to enable or disable.
 * @param[in] enable enable (true) interruption, or disable (false) it?
 */
void rtc_irq_enable(RTCIrqId irq_id, bool enable)
{
  _rtc_irq_handlers[irq_id].enabled = enable;

  switch(irq_id)
  {
    case RTC_IRQ_ID_ALARM_A:
    case RTC_IRQ_ID_ALARM_B:
      if( _rtc_irq_handlers[RTC_IRQ_ID_ALARM_A].enabled ||
	  _rtc_irq_handlers[RTC_IRQ_ID_ALARM_B].enabled) { HAL_NVIC_EnableIRQ( RTC_Alarm_IRQn); }
      else                                               { HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn); }
      break;

    default:
      // Do nothing
      break;
  }
}


/**
 * The handler set in the interruption vector table.
 */
void RTC_Alarm_IRQHandler(void)
{
  RTCIrqCtx *pv_ctx;

  if(__HAL_RTC_ALARM_GET_IT_SOURCE(&_rtc, RTC_IT_ALRA))
  {
    // Interruption on alarm A is enabled
    if(__HAL_RTC_ALARM_GET_FLAG(   &_rtc, RTC_FLAG_ALRAF))
    {
      // Interruption pending for alarm A
      // Clear pending bit
      __HAL_RTC_ALARM_CLEAR_FLAG(  &_rtc, RTC_FLAG_ALRAF);
      // Clear EXTI line's flag for RTC alarm.
      __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();

      // Call handler
      pv_ctx = &_rtc_irq_handlers[RTC_IRQ_ID_ALARM_A];
      if(pv_ctx->enabled && pv_ctx->pf_handler) { pv_ctx->pf_handler(); }
    }
  }
  if(__HAL_RTC_ALARM_GET_IT_SOURCE(&_rtc, RTC_IT_ALRB))
  {
    // Interruption on alarm B is enabled
    if(__HAL_RTC_ALARM_GET_FLAG(   &_rtc, RTC_FLAG_ALRBF))
    {
      // Interruption pending for alarm B
      // Clear pending bit
      __HAL_RTC_ALARM_CLEAR_FLAG(  &_rtc, RTC_FLAG_ALRBF);
      // Clear EXTI line's flag for RTC alarm.
      __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();

      // Call handler
      pv_ctx = &_rtc_irq_handlers[RTC_IRQ_ID_ALARM_B];
      if(pv_ctx->enabled && pv_ctx->pf_handler) { pv_ctx->pf_handler(); }
    }
  }
}



// ============================ use RTC to count ticks =====================

/**
 * Overrides the weak HAL HAL_InitTick function. We use RTC for ticks and delays.
 *
 * @return HAL_OK;
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  UNUSED(TickPriority);
  // Do nothing

  return HAL_OK;
}

/**
 * Get the tick count. Overrides the weak HAL HAL_GetTick() function.
 *
 * @return RTC tick count.
 */
uint32_t HAL_GetTick(void)
{
  return _rtc_has_been_initialised ? rtc_ticks_to_ms(rtc_get_date_as_ticks_since_2000()) : 0;
}


#ifdef __cplusplus
}
#endif

