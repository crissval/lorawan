/*
 * Base class for internal sensors
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */

#include <string.h>
#include "sensor_internal.hpp"
#include "board.h"
#include "nodeinfo.h"


#define SENSOR_INTERNAL_UNIQUE_ID_PREFIX  "CNSS" SENSOR_INTERNAL_UNIQUE_ID_SEP_STR


SensorInternal::SensorInternal(I2CAddress address, Endianness endianness, Features features) :
SensorI2C(POWER_INTERNAL,     POWER_NONE,
	  INTERFACE_INTERNAL, address, endianness, OPTION_FIXED_ADDR,
	  features)
{
  // Do nothing
}

SensorInternal::~SensorInternal()
{
  // Do nothing
}

const char *SensorInternal::uniqueId()
{
  char        buffer[SENSOR_UNIQUE_ID_SIZE_MAX];
  const char *psValue;

  // If a value already is set or cannot get sensor board serial number then keep current value.
  if(*Sensor::uniqueId() || !*(psValue = nodeinfo_sensors_board_SN())) { goto exit; }

  // Build unique id.
  strcpy(buffer, SENSOR_INTERNAL_UNIQUE_ID_PREFIX);
  if( strlcat(buffer, psValue,                           sizeof(buffer)) >= sizeof(buffer) ||
      strlcat(buffer, SENSOR_INTERNAL_UNIQUE_ID_SEP_STR, sizeof(buffer)) >= sizeof(buffer) ||
      strlcat(buffer, type(),                            sizeof(buffer)) >= sizeof(buffer))
  { goto exit; }

  // Set the id
  setUniqueId(buffer);

  exit:
  return Sensor::uniqueId();
}

void SensorInternal::processAlarmInterruption(CNSSInt::Interruptions ints)
{
  setIsInAlarm(true);
}




