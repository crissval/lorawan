/*
 * Base class for internal sensors
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */

#ifndef SENSORS_INTERNAL_SENSOR_INTERNAL_HPP_
#define SENSORS_INTERNAL_SENSOR_INTERNAL_HPP_

#include "sensor_i2c.hpp"


#define SENSOR_INTERNAL_UNIQUE_ID_SEP_STR "-"


class SensorInternal : public SensorI2C
{

protected:
  SensorInternal(I2CAddress address, Endianness endianness, Features features = FEATURE_BASE);
  virtual ~SensorInternal();

  virtual const char *uniqueId();

  void processAlarmInterruption(CNSSInt::Interruptions ints);
};

#endif /* SENSORS_INTERNAL_SENSOR_INTERNAL_HPP_ */
