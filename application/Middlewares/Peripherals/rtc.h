/**
 * RTC interface.
 *
 * @note the RTC is used to get 'ticks', the HAL functions using Systicks
 * are overwritten here by functions using the RTC.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#ifndef RTC_H_
#define RTC_H_

#include "datetime.h"
#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the backup register identifiers
   */
  typedef enum RTCBackupRegister
  {
    RTC_BKUPREG_0,  RTC_BKUPREG_1,  RTC_BKUPREG_2,  RTC_BKUPREG_3,
    RTC_BKUPREG_4,  RTC_BKUPREG_5,  RTC_BKUPREG_6,  RTC_BKUPREG_7,
    RTC_BKUPREG_8,  RTC_BKUPREG_9,  RTC_BKUPREG_10, RTC_BKUPREG_11,
    RTC_BKUPREG_12, RTC_BKUPREG_13, RTC_BKUPREG_14, RTC_BKUPREG_15,
    RTC_BKUPREG_16, RTC_BKUPREG_17, RTC_BKUPREG_18, RTC_BKUPREG_19,
    RTC_BKUPREG_20, RTC_BKUPREG_21, RTC_BKUPREG_22, RTC_BKUPREG_23,
    RTC_BKUPREG_24, RTC_BKUPREG_25, RTC_BKUPREG_26, RTC_BKUPREG_27,
    RTC_BKUPREG_28, RTC_BKUPREG_29, RTC_BKUPREG_30, RTC_BKUPREG_31,
    RTC_BKUPREG_COUNT, ///< Not an actual backup register id; used to count them
    RTC_BKUPREG_SYS_STATUS       = RTC_BKUPREG_0,
    RTC_BKUPREG_GEOPOS_LATITUDE  = RTC_BKUPREG_1,
    RTC_BKUPREG_GEOPOS_LONGITUDE = RTC_BKUPREG_2,
    RTC_BKUPREG_GEOPOS_ALTITUDE  = RTC_BKUPREG_3
  }
  RTCBackupRegister;

  /**
   * Defines the values that are stored in the system status backup register.
   */
  typedef enum RTCSystemStatusValue
  {
    RTC_SYSTEM_STATUS_NONE                         = 0x00,
    RTC_SYSTEM_STATUS_NOT_FIRST_RUN_SINCE_POWER_UP = 1u,
    RTC_SYSTEM_STATUS_HAS_GEOPOS_DATA              = 1u << 1,
    RTC_SYSTEM_STATUS_SOFTWARE_RESET_NONE          = 0,
    RTC_SYSTEM_STATUS_SOFTWARE_RESET_OK            = 1u << 2,
    RTC_SYSTEM_STATUS_SOFTWARE_RESET_ERROR         = 2u << 2
  }
  RTCSystemStatusValue;
#define RTC_SYSTEM_STATUS_SOFTWARE_RESET_MASK  (3u << 2)

  /**
   * Define the RTC interruption requests identifiers.
   */
  typedef enum RTCIrqId
  {
    RTC_IRQ_ID_ALARM_A,
    RTC_IRQ_ID_ALARM_B
  }
  RTCIrqId;
#define RTC_IRQ_ID_COUNT  2  // The number of RTCIrqId values.

  /**
   * Defined the alarm identifiers.
   */
  typedef enum RTCAlarmId
  {
    RTC_ALARM_ID_ALARM_A = RTC_IRQ_ID_ALARM_A,
    RTC_ALARM_ID_ALARM_B = RTC_IRQ_ID_ALARM_B
  }
  RTCAlarmId;


  /**
   * Defines the type of the function used to handle interruption requests, without argument.
   */
  typedef void (*RTCIrqHandler)(void);

  /**
   * The type used to store RTC ticks count.
   */
  typedef uint32_t RTCTicks;


  /**
   * Defines a date watcher, that is something interested in date changes.
   */
  struct RTCDateWatcher;
  typedef struct RTCDateWatcher RTCDateWatcher;
  struct RTCDateWatcher
  {
    const char *ps_name;  ///< The watcher's name.

    /**
     * Function called to signal a date change.
     *
     * This function pointer MUST be NOT NULL.
     *
     * @param[in] pv_dt_old        the old date. Is NOT NULL.
     *                             Pointer is only valid while the function is called.
     *                             If you want to keep it then you'll have to copy it.
     * @param[in] pv_dt_new        the new date. Is NOT NULL.
     *                             Pointer is only valid while the function is called.
     *                             If you want to keep it then you'll have to copy it.
     * @param[in] offset_positive  indicate if the offset is positive or not
     *                             (new date is greater than old one or not).
     * @param[in] offset_secs_abs  the offset's absolute value number of seconds.
     * @param[in] offset_ms_abs    the offset's absolute value number of microseconds, range [0..999].
     */
    void (*pf_signal_date_change)(
	const Datetime *pv_dt_old,
	const Datetime *pv_dt_new,
	bool            offset_positive,
	uint32_t        offset_secs_abs,
	uint16_t        offset_ms_abs);

    RTCDateWatcher *pv_next;  ///< Points to the next watcher in the watchers list. Is NULL is last of the list.
  };


  extern void     rtc_init(void);
  extern void     rtc_get_date(Datetime *pv_time);
  extern bool     rtc_set_date_get_offset(const Datetime *pv_time,
			       bool           *pb_offset_positive,
			       uint32_t       *pu32_offset_secs,
			       uint16_t       *pu16_offset_ms);
#define rtc_set_date(pv_time)  rtc_set_date_get_offset(pv_time, NULL, NULL, NULL)
  extern ts2000_t rtc_get_date_as_secs_since_2000( void);
  extern RTCTicks rtc_get_date_as_ticks_since_2000(void);
  extern RTCTicks rtc_ms_to_ticks(  uint32_t ms);
  extern RTCTicks rtc_secs_to_ticks(uint32_t secs);
  extern uint32_t rtc_ticks_to_ms(  RTCTicks ticks);

  extern void     rtc_register_date_watcher(RTCDateWatcher *pv_watcher);

  extern bool     rtc_set_calibration_offset_ppm(int16_t ppm, bool save);
  extern bool     rtc_load_and_set_calibration();
  extern int32_t  rtc_calibration_offset_ppm();

  extern bool     rtc_has_geoposition_data(void);
  extern void     rtc_clear_geoposition(  void);
  extern bool     rtc_geoposition(        float *pf_latitude, float *pf_longitude, float *pf_altitude);
  extern void     rtc_set_geoposition(    float  latitude,     float  longitude,   float  altitude);

  extern RTCSystemStatusValue rtc_software_reset_type(bool clear);
  extern void                 rtc_software_reset_set_type(RTCSystemStatusValue type);
  extern void                 rtc_software_reset_clear(void);

  extern void     rtc_stop_alarm(              RTCAlarmId alarm);
  extern void     rtc_set_alarm_relative_ticks(RTCAlarmId alarm, RTCTicks ticks);
  extern void     rtc_set_alarm_relative_secs( RTCAlarmId alarm, uint32_t secs);
  extern bool     rtc_set_wkup_date(           const Datetime *pv_time);

  extern bool     rtc_is_first_run_since_power_up(void);
  extern void     rtc_set_is_not_first_run(void);

  extern uint32_t rtc_get_minimum_timeout_ticks();

#define           rtc_remove_irq_handler(irq_id)  rtc_set_irq_handler(irq_id, NULL, false)
  extern void     rtc_set_irq_handler(RTCIrqId irq_id, RTCIrqHandler pf_handler, bool enable);
  extern void     rtc_irq_enable(     RTCIrqId irq_id, bool          enable);


#ifdef __cplusplus
}
#endif
#endif  // RTC_H_
