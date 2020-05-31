/**
 * Base class for periodic tasks.
 *
 * The periods are absolute, ie they are synchronised on midnight.
 *
 * @date   2019
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "periodic.hpp"
#include "rtc.h"
#include "rng.h"


ClassPeriodic::ClassPeriodic()
{
  this->_periodSec           = 0;
  this->_ts_next             = 0;
  this->_periodSpreadPlusSec = 0;
}

/**
 * Set the period.
 *
 * @post Update the next time timestamp.
 *
 * @param[in] secs the period, in seconds.
 */
void ClassPeriodic::setPeriodSec(uint32_t secs)
{
  if(secs != this->_periodSec)
  {
    this->_periodSec = secs;
    setNextTime();
  }
}


/**
 * Set the next time.
 *
 * @param ts the timestamp of the next time.
 *           If 0 then the next time is computed using the current time and the period.
 */
void ClassPeriodic::setNextTime(ts2000_t ts)
{
  uint32_t secs, plusSec;

  if(ts) { this->_ts_next = ts; }
  else
  {
    if(this->_periodSec)
    {
      // Compute next time using current time and period.
      ts             = rtc_get_date_as_secs_since_2000();
      this->_ts_next = ts + (this->_periodSec - ts % this->_periodSec);

      // Spread the period if we have to
      if(this->_periodSpreadPlusSec)
      {
	// Make sure that the spreading values do not exceed half of the period.
	secs    = this->_periodSec / 2;
	plusSec = this->_periodSpreadPlusSec;
	if(plusSec  > secs) { plusSec  = secs; }

	// Generate a random number and spread period
	if(plusSec) { this->_ts_next = this->_ts_next + rng_u32() % plusSec; }
      }
    }
    else { this->_ts_next = 0; }
  }
}


/**
 * Indicate if current time is greater or equal to 'next time'.
 *
 * @param[in] updateNextTime update 'next time' if current value is in the past (true) or
 *                           keep current value?
 *
 * @return true  if the current 'next time' is in the past.
 * @return false if no 'next time' is set.
 * @return false otherwise.
 */
bool ClassPeriodic::itsTime(bool updateNextTime)
{
  ts2000_t ts_now;
  bool     res = false;

  if(this->_ts_next)
  {
    ts_now  = rtc_get_date_as_secs_since_2000();
    if((res = (ts_now >= this->_ts_next)) && updateNextTime) { setNextTime(); }
  }

  return res;
}


/**
 * Set the period spreading range. Use 0 value to deactivate period spreading.
 *
 * Using period spreading it is possible to change the period of an event from one
 * cycle to the next. The minimum period is 'period' and the maximum period is 'period + plus',
 * where 'plus' is the spreading plus value.
 * The actual period value for the next time is computed using a random number and with a
 * formula that looks like this:
 * <code>nextPeriod = period + rand() % plus</code>.
 *
 * @param[in] plusSec  the plus spread value, in seconds.
 */
void ClassPeriodic::setPeriodSpreadSec(uint32_t plusSec)
{
  this->_periodSpreadPlusSec = plusSec;

  // Update next time
  setNextTime();
}


