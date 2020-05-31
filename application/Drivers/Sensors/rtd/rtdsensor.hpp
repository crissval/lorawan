/*
 * rtdsensor.hpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#ifndef SENSORS_RTD_RTDSENSOR_HPP_
#define SENSORS_RTD_RTDSENSOR_HPP_


#include "sensor.hpp" // To sub-class it.
#include "rtd.hpp"    // To use the generic RTD system that we've done.

class RTDSensor : protected Sensor {
protected:
  typedef enum NbWires {  // Defines the number of reading wires.
    NBWIRES_NONE = 0,
    NBWIRES_2    = 2,
    NBWIRES_3    = 3,
    NBWIRES_4    = 4
  } NbWires;

protected:
  RTDSensor(Power    power, Power powerSleep, Features features = FEATURE_BASE);
  virtual ~RTDSensor();

protected:
  bool         setRTDResistance(float ohms); // Called by sub-class.
  NbWires      nbWires() const        { return this->_nbWires; }
  void         setNbWires(NbWires nb) { this->_nbWires = nb;   }
  float        refOhms() const        { return this->_refOhms; }
  void         setRefOhms(float ohms) { this->_refOhms = ohms; }

  virtual bool jsonSpecificHandler(const JsonObject& json);
  virtual bool writeDataToCNSSRFDataFrameSpecific(
                       CNSSRFDataFrame *pvFrame);
  virtual const char **csvHeaderValues();
  virtual int32_t      csvDataSpecific(char *psData, uint32_t size);

private:
  bool setRTDInstanceUsingTypeName(const char *psTypeName);

private:
  RTD    *_pvRTD;   // The RTD instance.
  NbWires _nbWires; // The number of wires used to read the sensor.
  float   _refOhms; // Reference resistor value, in Ohms.

  static const char *_CSV_HEADER_VALUES[3];  // The CSV header.
};



#endif /* SENSORS_RTD_RTDSENSOR_HPP_ */
