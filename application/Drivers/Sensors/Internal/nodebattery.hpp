/**
 * This sensor is used to read the node's battery voltage using
 * the voltage divisor R26/R27 and an ADC input from the µC.
 *
 * This sensor is not added to the sensor factory because the user cannot directly use it.
 * It's a 'system' sensor.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2019
 */
#ifndef SENSORS_INTERNAL_NODEBATTERY_HPP_
#define SENSORS_INTERNAL_NODEBATTERY_HPP_

#include "config.h"
#include "sensor_adc.hpp"


class NodeBattery : public SensorADC
{
protected:
  typedef enum BattVSource
  {
    BATTV_SOURCE_UNKNOWN,
    BATTV_SOURCE_BQ2589X,
    BATTV_SOURCE_ADC
  }
  BattVSource;


public:
  static NodeBattery *instance();
  const char    *type();

  uint16_t voltageMV()  { return this->_voltageMV;                   }
  float    voltageV()   { return ((float)this->_voltageMV) / 1000.0; }
  bool     isCharging() { return _isCharging;                        }

  void setVoltageLowMV(uint16_t mv) { this->_voltageLowMV = mv;                     }
  void setVoltageLowV( float    v)  { this->_voltageLowMV = (uint16_t)(v * 1000.0); }

protected:
  NodeBattery();
  virtual ~NodeBattery();

  bool  openSpecific();
  void  closeSpecific();
  bool  readSpecific();

  void  clearReadings();

  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame  *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  uint16_t    _voltageMV;     ///< The battery's voltage in milliVolts.
  uint16_t    _voltageLowMV;  ///< The low battery voltage threshold, in milliVolts.
  BattVSource _battVSource;   ///< What source to use to get the battery voltage?
  bool        _isCharging;    ///< Is the battery being charged?

  static const char *_CSV_HEADER_VALUES[];

  static NodeBattery _instance;  ///< The unique instance of this class.
};



#endif /* SENSORS_INTERNAL_NODEBATTERY_HPP_ */
