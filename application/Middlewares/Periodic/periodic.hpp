/**
 * Base class for periodic tasks.
 *
 * The periods are absolute, ie they are synchronised on midnight.
 *
 * @date   2019
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#pragma once

#include "defs.h"
#include "datetime.h"


class ClassPeriodic
{
public:
  ClassPeriodic();

  virtual void  setPeriodSec(uint32_t secs);
  uint32_t      periodSec(void) const { return this->_periodSec; }

  ts2000_t     nextTime( void) const { return this->_ts_next;   }
  void         setNextTime(ts2000_t ts = 0);
  virtual bool itsTime(bool updateNextTime = true);

  void     setPeriodSpreadSec(uint32_t plusSec);


private:
  uint32_t _periodSec;             ///< The period, in seconds.
  uint32_t _periodSpreadPlusSec;   ///< Period plus spreading, in seconds.
  ts2000_t _ts_next;               ///< Timestamp of the next period. 0 if no next time is set.
};
