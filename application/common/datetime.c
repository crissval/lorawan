/*
 * datetime.c
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#include "datetime.h"


#ifdef __cplusplus
extern "C" {
#endif


#define  DAYS_IN_MONTH_CORRECTION_NORM  ((uint32_t) 0x99AAA0)
#define  DAYS_IN_MONTH_CORRECTION_LEAP  ((uint32_t) 0x445550)


/* Calculates ceiling(X/N) */
#define DIVC(X, N) (((X) + (N) - 1) / (N))


  static uint8_t _datetime_days_in_month_norm_year[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  static uint8_t _datetime_days_in_month_leap_year[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


  /**
   * Clear a Datetime object. Set all it's fields to 0.
   *
   * @param[in] pv_dt the Datetime object to clear. MUST be NOT NULL.
   */
  void datetime_clear(Datetime *pv_dt)
  {
    pv_dt->year  = 0;
    pv_dt->month = pv_dt->day     = 0;
    pv_dt->hours = pv_dt->minutes = pv_dt->seconds = pv_dt->sub_seconds = 0;
  }

  /**
   * Copy a Datetime object.
   *
   * @param[out] pv_dest the Datetime object to copy to. MUST be NOT NULL.
   * @param[in]  pv_src  the Datetime object to copy from. MUST be NOT NULL.
   */
  void datetime_copy(Datetime *pv_dest, const Datetime *pv_src)
  {
    pv_dest->year        = pv_src->year;
    pv_dest->month       = pv_src->month;
    pv_dest->day         = pv_src->day;
    pv_dest->hours       = pv_src->hours;
    pv_dest->minutes     = pv_src->minutes;
    pv_dest->seconds     = pv_src->seconds;
    pv_dest->sub_seconds = pv_src->sub_seconds;
  }

  /**
   * Indicate if two Datetime object are equals (contains the same values).
   *
   * @param[in] pv_dt1           the first Datetime object to compare. MUST be NOT NULL.
   * @param[in] pv_dt2           the second Datetime object to compare. MUST be NOT NULL.
   * @param[in] compare_sub_secs compare sub-seconds?
   *
   * @return true  if the objects have the same values.
   * @return false otherwise.
   */
  bool datetime_equals(const Datetime *pv_dt1, const Datetime *pv_dt2, bool compare_sub_secs)
  {
    return
	pv_dt1->seconds     == pv_dt2->seconds &&
	pv_dt1->minutes     == pv_dt2->minutes &&
	pv_dt1->hours       == pv_dt2->hours   &&
	pv_dt1->day         == pv_dt2->day     &&
	pv_dt1->month       == pv_dt2->month   &&
	pv_dt1->year        == pv_dt2->year    &&
	(!compare_sub_secs || pv_dt1->sub_seconds == pv_dt2->sub_seconds);
  }


  /**
   * Convert a timestamp stored in a Datetime object into a timestamp that
   * is the number of seconds elapsed since 2000.
   *
   * @param[in] pv_dt the Datetime object to convert. MUST be NOT NULL and MUST have been initialised.
   *
   * @return the timestamp in seconds since 2000.
   */
  ts2000_t datetime_to_timestamp_sec_2000(const Datetime *pv_dt)
  {
    ts2000_t res;
    uint32_t correction;
    uint32_t year = pv_dt->year - 2000;

    // Calculate amount of elapsed days since 2000/01/01
    correction = (year % 4) ? DAYS_IN_MONTH_CORRECTION_NORM : DAYS_IN_MONTH_CORRECTION_LEAP;
    res        = DIVC((DT_DAYS_IN_YEAR  * 3 + DT_DAYS_IN_LEAP_YEAR) * year , 4);
    res       += DIVC( (((uint32_t)pv_dt->month) - 1) * (30 + 31), 2) -
	         ((correction >> ((pv_dt->month  - 1) * 2)) & 0x3);
    res       += pv_dt->day - 1;

    // Convert from days to seconds
    res       *= DT_SECONDS_IN_1DAY;
    res       += ((uint32_t)pv_dt->seconds)                         +
	         ((uint32_t)pv_dt->minutes) * DT_SECONDS_IN_1MINUTE +
		 ((uint32_t)pv_dt->hours)   * DT_SECONDS_IN_1HOUR;

    return res;
  }

  /**
   * Convert a timestamp, in seconds since 2000, to a date and time.
   *
   * @param[out] pv_dt  where the date and the time are written to. MUST be NOT NULL.
   * @param[in]  ts2000 the timestamp, in seconds since 2000.
   *
   * @return The Datetime object.
   */
  Datetime *datetime_from_sec_2000(Datetime *pv_dt, ts2000_t ts2000)
  {
    uint8_t        i, leap_days;
    uint16_t       days;
    const uint8_t *days_in_month;

    pv_dt->sub_seconds = 0;
    pv_dt->seconds     = ts2000 % 60; ts2000 /= 60;
    pv_dt->minutes     = ts2000 % 60; ts2000 /= 60;
    pv_dt->hours       = ts2000 % 24; ts2000 /= 24;

    // ts2000 is now the number of days since 2000. Remember 2000 is a leap year.
    leap_days   = (ts2000 + DT_DAYS_IN_YEAR * 3 + 1) / (DT_DAYS_IN_YEAR * 3 + DT_DAYS_IN_LEAP_YEAR);
    pv_dt->year = (ts2000 - leap_days)               /  DT_DAYS_IN_YEAR;
    ts2000     -= pv_dt->year * DT_DAYS_IN_YEAR + (pv_dt->year + 3) / 4;

    // ts is now the number of days since the start of the year
    days_in_month = (pv_dt->year % 4) ? _datetime_days_in_month_norm_year : _datetime_days_in_month_leap_year;
    ts2000       += 1; // Because first day is 1 and not 0.
    for(i = 0, days = 0; i < 12; days += days_in_month[i], i++)
    {
      if(ts2000 <= days + days_in_month[i])
      {
	pv_dt->month = i      + 1;
	pv_dt->day   = ts2000 - days;
	break;
      }
    }

    // Add year 2000 offset
    pv_dt->year += 2000;

    return pv_dt;
  }

  /**
   * Convert datetime values into a timestamp that is the number of seconds elapsed since 2000.
   *
   * @param[in] year    the year.
   * @param[in] month   the month.
   * @param[in] day     the day of the month.
   * @param[in] hours   the hours.
   * @param[in] minutes the minutes.
   * @param[in] seconds the seconds.
   *
   * @return the timestamp in seconds since 2000.
   */
  ts2000_t datetime_values_to_timestamp_sec_2000(uint16_t year,  uint8_t month,   uint8_t day,
  							uint8_t  hours, uint8_t minutes, uint8_t seconds)
  {
    Datetime dt;

    datetime_init(&dt, year, month, day, hours, minutes, seconds, 0);
    return datetime_to_timestamp_sec_2000(&dt);
  }

#ifdef __cplusplus
}
#endif
