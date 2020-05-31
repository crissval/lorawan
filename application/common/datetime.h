/*
 * datetime.h
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#ifndef DATETIME_H_
#define DATETIME_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif


#define  DT_DAYS_IN_LEAP_YEAR  ((uint32_t) 366)
#define  DT_DAYS_IN_YEAR       ((uint32_t) 365)
#define  DT_SECONDS_IN_1DAY    ((uint32_t) 86400)
#define  DT_SECONDS_IN_1HOUR   ((uint32_t) 3600)
#define  DT_SECONDS_IN_1MINUTE ((uint32_t) 60)
#define  DT_MINUTES_IN_1HOUR   ((uint32_t) 60)
#define  DT_HOURS_IN_1DAY      ((uint32_t) 24)



#define timestamps_u32_diff(t1, t2)  (((t1) >= (t2)) ? (t1) - (t2) : UINT32_MAX - (t1) + (t2))


  /**
   * Defines the type used to hold time and date
   */
  typedef struct Datetime
  {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t sub_seconds;  // milliseconds in fact, in range [0..999]
  }
  Datetime;

  /// Defines the type for the timestamp expressed in seconds since 2000/01/01 00:00:00
  typedef uint32_t ts2000_t;

#define datetime_init(pv_dt, _year, _month, _day, _hours, _minutes, _seconds, _sub_seconds)  \
  (pv_dt)->year        = (_year);     \
  (pv_dt)->month       = (_month);    \
  (pv_dt)->day         = (_day);      \
  (pv_dt)->hours       = (_hours);    \
  (pv_dt)->minutes     = (_minutes);  \
  (pv_dt)->seconds     = (_seconds);  \
  (pv_dt)->sub_seconds = (_sub_seconds)

  extern void      datetime_clear(       Datetime *pv_dt);
  extern void      datetime_copy(        Datetime *pv_dest, const Datetime *pv_src);
  extern bool      datetime_equals(const Datetime *pv_dt1,
				   const Datetime *pv_dt2,
				   bool            compare_sub_secs);
  extern ts2000_t  datetime_to_timestamp_sec_2000(const Datetime *pv_dt);
  extern Datetime *datetime_from_sec_2000(Datetime *pv_dt, ts2000_t ts2000);
  extern ts2000_t  datetime_values_to_timestamp_sec_2000(uint16_t year,  uint8_t month,   uint8_t day,
							 uint8_t  hours, uint8_t minutes, uint8_t seconds);



#ifdef __cplusplus
}
#endif

#endif /* DATETIME_H_ */
