/*
 * Reading of one of the node's digital inputs.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2019
 */
#ifndef SENSORS_EXTERNAL_DIGITALINPUT_HPP_
#define SENSORS_EXTERNAL_DIGITALINPUT_HPP_

#include "config.h"
#ifdef USE_SENSOR_DIGITAL_INPUT
#include "sensor.hpp"
#include "extensionport.h"

class DigitalInput : public Sensor
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


public:
  DigitalInput();
  ~DigitalInput();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool openSpecific();
  void closeSpecific();
  bool readSpecific();
  bool jsonSpecificHandler(               const JsonObject& json);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame  *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  uint8_t           _id;         ///< The input's identifier.
  bool              _state;      ///< The input's state.
  const ExtPortPin *_pvExtPort;  ///< The node extension port used as digital input.

  static const char *_CSV_HEADER_VALUES[];  ///< The CSV header values.
};

#endif // USE_SENSOR_DIGITAL_INPUT
#endif /* SENSORS_EXTERNAL_DIGITALINPUT_HPP_ */
