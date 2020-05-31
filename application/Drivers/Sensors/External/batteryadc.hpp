/*
 * Battery monitoring using the µC ADC.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_EXTERNAL_BATTERYADC_HPP_
#define SENSORS_EXTERNAL_BATTERYADC_HPP_

#include "config.h"
#ifdef USE_SENSOR_BATTERY_ADC
#include "sensor_adc.hpp"
#include "battery.h"


class BatteryADC : SensorADC
{
  public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


public:
  BatteryADC();
  ~BatteryADC();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool readSpecific();

  bool jsonSpecificHandler(               const JsonObject& json);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame  *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  ADCLine _adcLine;        ///< The ADC line we are using.
  float   _voltageDivider; ///< The voltage divider ratio used to read the battery's voltage.
  Battery _battery;        ///< The battery object

  static const char   *_CSV_HEADER_VALUES[];
};

#endif // USE_SENSOR_BATTERY_ADC
#endif /* SENSORS_EXTERNAL_BATTERYADC_HPP_ */
