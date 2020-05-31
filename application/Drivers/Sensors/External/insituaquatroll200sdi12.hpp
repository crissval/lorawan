/*
 * Driver for InSitu Aqua TROLL-200 sensor.
 *
 *  Created on: Jul 18, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_EXTERNAL_INSITUAQUATROLL200SDI12_HPP_
#define SENSORS_EXTERNAL_INSITUAQUATROLL200SDI12_HPP_


#include "config.h"
#ifdef USE_SENSOR_INSITU_AQUATROLL_200_SDI12
#include "sensor_sdi12.hpp"


#define SENSOR_INSITU_AQUATROLL_200_SDI12_READINGS_FORMAT_LEN_MAX  24


class InSituAquaTROLL200SDI12 : public SensorSDI12
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


private:
  /**
   * Defines flags to identify readings
   */
  typedef enum ReadingFlag
  {
    READING_FLAG_NONE                  = 0,
    READING_FLAG_TEMPERATURE           = 1u << 0,
    READING_FLAG_CONDUCTIVITY          = 1u << 1,
    READING_FLAG_SPECIFIC_CONDUCTIVITY = 1u << 4,
    READING_FLAG_LEVEL                 = 1u << 5,
    READING_FLAG_PRESSURE              = 1u << 6
  }
  ReadingFlag;
  typedef uint8_t ReadingFlags;


public:
  InSituAquaTROLL200SDI12();
  ~InSituAquaTROLL200SDI12();
  static Sensor *getNewInstance();
  const char    *type();

  bool readOnAlarmChange();
  bool readSpecific();
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  void        clearReadings();
  const char *readingsFormat();
  void        clearReadingsFormat();


private:
  float                            _tempDegC;                  ///< Water temperature, in Celsius degrees.
  float                            _conductivity;              ///< Water conductivity.
  CNSSRFDTSolutionConductivityType _conductivityType;          ///< Indicate the conductivity units.
  float                            _specificConductivity;      ///< Water specific conducticity.
  CNSSRFDTSolutionConductivityType _specificConductivityType;  ///< Indicate the specific conductivity units.
  float                            _levelM;                    ///< Water level, in meters.
  double                           _pressKPa;                  ///< Water pressure, in kilopascals.
  bool                             _pressAbs;                  ///< Water pressure is absolute (true), or gauged to atmospheric pressure (false).
  ReadingFlags                     _readingFlags;              ///< A Ored combination of the reasdings we have.

  /// The format of the readings we get from the sensor. It's the string returned by the aXPR0! command
  char  _readingsFormat[SENSOR_INSITU_AQUATROLL_200_SDI12_READINGS_FORMAT_LEN_MAX];

  static const SDI12GenSensorCommands _commandsDescription;  ///< The description of the commands specific to this sensor.
  static const char                  *_CSV_HEADER_VALUES[];
};


#endif // USE_SENSOR_INSITU_AQUATROLL_200_SDI12
#endif /* SENSORS_EXTERNAL_INSITUAQUATROLL200SDI12_HPP_ */
