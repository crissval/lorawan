/*
 * Base class for sensors using the µC ADC line(s)
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_SENSOR_ADC_HPP_
#define SENSORS_SENSOR_ADC_HPP_

#include "sensor.hpp"
#include "board.h"


class SensorADC : public Sensor
{
protected:
  /**
   * Defines the ADC line identifiers.
   */
  typedef enum ADCLine
  {
    ADC_LINE_NONE,
    ADC_LINE_ANA1,
    ADC_LINE_ANA2,
    ADC_LINE_BATV
  }
  ADCLine;
#define SENSOR_ADC_LINES_COUNT  3

  /**
   * Defines the ADC line identification flags.
   */
  typedef enum ADCLineFlag
  {
    ADC_LINE_FLAG_NONE = 0x00,
    ADC_LINE_FLAG_ANA1 = 1u << ADC_LINE_ANA1,
    ADC_LINE_FLAG_ANA2 = 1u << ADC_LINE_ANA2,
    ADC_LINE_FLAG_BATV = 1u << ADC_LINE_BATV
  }
  ADCLineFlag;
  typedef uint8_t ADCLineFlags;  ///< The type used to store ADC line identifier flags.


  /**
   * Describe an ADC line
   */
  typedef struct ADCLineDesc
  {
    ADCLine       id;
    ADCLineFlag   idFlag;
    const char   *psName;
    GPIOId        gpio;
    ADC_TypeDef  *pvADC;
    uint32_t      adcChannel;
  }
  ADCLineDesc;


protected:
  SensorADC(Power power, Power powerSleep, ADCLineFlags adcLines);
  SensorADC(Power power, Power powerSleep, ADCLine      adcLine);
  virtual ~SensorADC();

  void         setADCLines(ADCLineFlags adcLines);
  void         setADCLine( ADCLine      adcLine);
  ADCLineFlags adcLines() const               { return this->_adcLines;                          }
  uint32_t     adcValueMV(ADCLine line) const { return this->_adcValuesMV[line - ADC_LINE_ANA1]; }

  static const ADCLineDesc *lineDescUsingName(const char *psName);

  virtual bool openSpecific();
  virtual void closeSpecific();
  virtual bool readSpecific();


private:
  void init(ADCLineFlags adcLines);

  ADC_HandleTypeDef *getADCHandleForLineDesc(const ADCLineDesc *pvDesc, bool create);


private:
  ADCLineFlags      _adcLines;                            ///< The ADC lines used by the sensor.
  uint32_t          _adcValuesMV[SENSOR_ADC_LINES_COUNT]; ///< The ADC values, in millivolts.
  ADC_HandleTypeDef _hadcs[      SENSOR_ADC_LINES_COUNT]; ///< The ADC handles.

  static const ADCLineDesc _adcDescs[SENSOR_ADC_LINES_COUNT]; ///< Descriptions of the ADC lines.
};



#endif /* SENSORS_SENSOR_ADC_HPP_ */
