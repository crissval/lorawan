/*
 * rtdsensor.cpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#include <string.h>
#include "rtdsensor.hpp"
#include "rtd_pt.hpp"
#include "logger.h"
#include "cnssrf-dt_temperature.h"

CREATE_LOGGER(  rtdsensor);  // Create logger
#undef  _logger
#define _logger rtdsensor

// We'll implement deltaT later.
const char *RTDSensor::_CSV_HEADER_VALUES[] = {
    "TemperatureDegC", "deltaTDegC", NULL
};

// Constructor.
RTDSensor::RTDSensor(Power power, Power powerSleep, Features features)
: Sensor(power, powerSleep, features) {
  this->_pvRTD   = NULL;
  this->_nbWires = NBWIRES_NONE;
  this->_refOhms = 0.0;
}

// Destructor.
RTDSensor::~RTDSensor() {
  if(this->_pvRTD) {
    delete this->_pvRTD;
    this->_pvRTD = NULL;
  }
}

bool RTDSensor::setRTDInstanceUsingTypeName(const char *psTypeName){
  size_t len;
  bool   res = false;

  if(this->_pvRTD) {
    delete this->_pvRTD;
    this->_pvRTD = NULL;
  }
  if(!psTypeName || !*psTypeName) { goto exit; }

  // Try platinum RTDs
  if((len = strlen(psTypeName)) >= 3                &&
     (psTypeName[0] == 'P' || psTypeName[0] == 'p') &&
     (psTypeName[1] == 'T' || psTypeName[1] == 't') &&
     (psTypeName[2] >= '1' && psTypeName[2] <= '9')) {
    if(!(this->_pvRTD = new RTDPT())) {
      log_error_sensor(_logger, "Cannot allocate RTDPT object.");
      goto exit;
    }
    if(((RTDPT *)this->_pvRTD)->setOhmsAt0DegCUsingName(psTypeName)) {
      res = true; goto exit; // Ok; our instance has been created.
    }
    // Failed to set up instance; try something else.
    delete this->_pvRTD;  // Free memory
    this->_pvRTD = NULL;
    log_error_sensor(_logger, "Failed to get resistance at 0°C using "
                              "PT RTD sensor name '%s'.", psTypeName);
  } // End of the line, nothing matching has been found.

  exit:
  return res;
}


bool RTDSensor::jsonSpecificHandler(const JsonObject& json)
{
  const char *psValue;
  float       f;
  int32_t     i32;
  bool        res = true;

  // Get the RTD instance
  if(json["RTDType"].success()) {
    psValue = json["RTDType"].as<const char *>();
    if(!setRTDInstanceUsingTypeName(psValue)) {
      log_error_sensor(_logger, "Unknown RTDType: '%s'.", psValue);
      res = false;
    }
  } else {
    log_error_sensor(_logger,
                     "You must specify the type of RTD sensor "
                     "using the configuration parameter 'RTDType'.");
    res = false;
  }

  // Get the number of wires.
  if(json["nbWires"].success()) {
    i32 = json["nbWires"].as<int32_t>();
    if(i32 >= 2 && i32 <= 4) {
      setNbWires((NbWires)i32); // enum values were chosen for this.
    } else {
      log_error_sensor(_logger,
		       "Value for configuration parameter 'nbWires' "
		       "can only be '2', '3' or '4'.");
      res = false;
    }
  } else {
    log_error_sensor(_logger,
		     "You must indicate the number of wires used to "
		     "read the sensor's resistance with configuration "
		     "parameter 'nbWires'.");
    res = false;
  }

  // Get reference resistor value.
  if(json["RrefOhms"].success()) {
    f = json["RrefOhms"].as<float>();
    if(f > 0) { this->_refOhms = f; }
    else {
      log_error_sensor(_logger, "'RrefOhms' value must be a floating "
		       "number > 0.");
      res = false;
    }
  }
  else {
    log_error_sensor(_logger,
		     "You must specify the reference resistor value in "
		     "Ohms using configuration parameter 'RrefOhms'.");
    res = false;
  }

  return res;
}


bool RTDSensor::setRTDResistance(float ohms)
{
  bool res;

  // Update temperature using resistance value
  res = this->_pvRTD &&
        this->_pvRTD->updateTemperatureUsingResistance(ohms);
  if(res) {
    log_debug_sensor(_logger, "Temperature is: %.1f°C",
                     this->_pvRTD->temperatureDegC());
  }

  return res;
}


bool RTDSensor::writeDataToCNSSRFDataFrameSpecific(
                        CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_temperature_write_degc_to_frame(pvFrame,
                           this->_pvRTD->temperatureDegC(),
                           CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE);
}


const char **RTDSensor::csvHeaderValues() { return _CSV_HEADER_VALUES; }


int32_t RTDSensor::csvDataSpecific(char *psData, uint32_t size) {
  uint32_t len;

  len = snprintf(psData, size, "%.1f%c%c",  // No deltaT to write yet.
                 this->_pvRTD->temperatureDegC(),
                 OUTPUT_DATA_CSV_SEP,
                 OUTPUT_DATA_CSV_SEP);

  return len >= size ? -1 : (int32_t)len;
}



