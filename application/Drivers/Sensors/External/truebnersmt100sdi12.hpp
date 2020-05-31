/*
 * Driver for Truebner soil humidity sensor SMT100 using SDI-12 interface.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date 2018
 */
#ifndef SENSORS_EXTERNAL_TRUEBNERSMT100SDI12_HPP_
#define SENSORS_EXTERNAL_TRUEBNERSMT100SDI12_HPP_

#include "config.h"
#ifdef USE_SENSOR_TRUEBNER_SMT100_SDI12
#include "sensor_sdi12.hpp"


class TruebnerSMT100SDI12 : public SensorSDI12
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.


private:
  /**
   * The measurement identifiers.
   */
  typedef enum MeasurementId
  {
    MEASUREMENT_ID_NONE         = 0,
    MEASUREMENT_ID_RAW          = 1u << 0,
    MEASUREMENT_ID_PERMITTIVITY = 1u << 2,
    MEASUREMENT_ID_WATER        = 1u << 3,
    MEASUREMENT_ID_TEMPERATURE  = 1u << 4,
    MEASUREMENT_ID_VOLTAGE      = 1u << 5,
    MEASUREMENT_ID_ALL          = 0xFF
  }
  MeasurementId;
  typedef uint8_t MeasurementIds;  ///< Type used to store a ORed combination of MeasurementId values.
#define TRUEBNER_SMT100_SDI12_MEASUREMENTS_COUNT  5

  /**
   * Store informations about a measurement.
   */
  typedef struct MeasurementInfos
  {
    MeasurementId id;      ///< The measurement's identifier.
    const char   *psName;  ///< The measurement's name.
  }
  MeasurementInfos;


public:
  TruebnerSMT100SDI12();
  ~TruebnerSMT100SDI12();
  static Sensor *getNewInstance();
  const char    *type();

  bool jsonSpecificHandler(const JsonObject& json);
  bool readSpecific();
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  void          clearReadings();
  MeasurementId getMeasurementIdUsingName(const char *psName);

private:
  MeasurementIds _measurementsToGet;      ///< ORed combination of the measurements to get.
  uint32_t       _depthCm;                ///< The sensor's depth, in centimeters.
  uint32_t       _raw;                    ///< Uncalibrated raw measurement value
  float          _dielectricPermittivity; ///< Calibrated dielectric permittivity
  float          _waterPercent;           ///< Calibrated volumic water content in percents.
  float          _temperatureDegC;        ///< Temperature, in degrees Celsius.
  float          _voltageV;               ///< The supply voltage in volts.

  /**
   * Store the measurement's infos.
   */
  static const MeasurementInfos       _measurementInfos[TRUEBNER_SMT100_SDI12_MEASUREMENTS_COUNT];

  static const SDI12GenSensorCommands _commandsDescription;  ///< The description of the commands specific to this sensor.

  static const char *_CSV_HEADER_VALUES[];
};


#endif // USE_SENSOR_TRUEBNER_SMT100_SDI12
#endif /* SENSORS_EXTERNAL_TRUEBNERSMT100SDI12_HPP_ */
