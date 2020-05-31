/*
 * Base sensor class for all Gill MaxiMets weather stations, using SDI-12 communication.
 *
 *  @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date:   2018
 */

#ifndef SENSORS_EXTERNAL_GILLMAXIMETSDI12_H_
#define SENSORS_EXTERNAL_GILLMAXIMETSDI12_H_

#include "config.h"
#ifdef USE_SENSOR_GILL_MAXIMET_SDI12
#include "sensor_sdi12.hpp"
#include "datetime.h"


class GillMaximMetSDI12 : public SensorSDI12
{
public:
  static const char *TYPE_NAME;  ///< The sensor's type name.

  /**
   * Defines the measurement identifiers.
   */
  typedef enum MeasurementId
  {
    M_NONE                     = 0,
    M_RELATIVE_WIND_DIRECTION  = (1u << 0),
    M_RELATIVE_WIND_SPEED      = (1u << 1),
    M_CORRECTED_WIND_DIRECTION = (1u << 2),
    M_CORRECTED_WIND_SPEED     = (1u << 3),
    M_TEMPERATURE              = (1u << 4),
    M_RELATIVE_HUMIDITY        = (1u << 5),
    M_DEWPOINT                 = (1u << 6),
    M_PRESSURE                 = (1u << 7),
    M_WIND_CHILL               = (1u << 8),
    M_HEAT_INDEX               = (1u << 9),
    M_AIR_DENSITY              = (1u << 10),
    M_WET_BULB_TEMPERATURE     = (1u << 11),
    M_PRECIPITATION_INTENSITY  = (1u << 12),
    M_TOTAL_PRECIPITATION      = (1u << 13),
    M_SOLAR_RADIATION          = (1u << 14),
    M_SUNSHINE_HOURS           = (1u << 15),
    M_X_Y_TILT_Z_ORIENTATION   = (1u << 16),
    M_GEO_POSITION             = (1u << 17),
    M_DATETIME                 = (1u << 18),
    M_SUNRISE_TIME             = (1u << 19),
    M_SOLAR_NOON               = (1u << 20),
    M_SUNSET_TIME              = (1u << 21),
    M_SUN_POSITION             = (1u << 22),
    M_TWILIGHT_CIVIL           = (1u << 23),
    M_TWILIGHT_NAUTICAL        = (1u << 24),
    M_TWILIGHT_ASTRONOMICAL    = (1u << 25)
  }
  MeasurementId;
  typedef uint32_t Measurements;  ///< Type used to store several measurement ids.
#define GILL_MAXIMET_SDI12_NB_MEASUREMENTS  11


private:
  /**
   * Type used to represent and store a SDI-12 Start Measurement command identifier and flag.
   */
  typedef enum CommandMIdFlagEnum
  {
    CMD_M_IDFLAG_NONE = 0,
    CMD_M_IDFLAG_M    = 0u + (1u << 4),
    CMD_M_IDFLAG_M1   = 1u + (1u << 5),
    CMD_M_IDFLAG_M2   = 2u + (1u << 6),
    CMD_M_IDFLAG_M3   = 3u + (1u << 7),
    CMD_M_IDFLAG_M4   = 4u + (1u << 8),
    CMD_M_IDFLAG_M5   = 5u + (1u << 9),
    CMD_M_IDFLAG_M6   = 6u + (1u << 10),
    CMD_M_IDFLAG_M7   = 7u + (1u << 11),
  }
  CommandMIdFlagEnum;
  typedef uint16_t CommandMIdFlag;  ///< More compact storage
  typedef uint16_t CommandMFlags;   ///< Used to store several flags from CommandMIdFlagEnum values.
  typedef uint8_t  CommandMId;      ///< Type to use if we just want to store the identifier from a CommandMIdFlag.
  typedef uint8_t  CommandMFlag;    ///< Type to use if we just want to store the flag from a CommandMIdFlag.
#define gill_maximet_sdi12_cmdm_idflag_id(  idFlag)  (( idFlag) & 0x00F)
#define gill_maximet_sdi12_cmdm_idflag_flag(idFlag)  (((idFlag) & 0xFF0) >> 4)

  /**
   * Defines the types used to store the measurements' data.
   */
  typedef enum MeasurementType
  {
    M_TYPE_NONE,
    M_TYPE_FLOAT,
    M_TYPE_INT,
    M_TYPE_COMPOUND
  }
  MeasurementType;


  /**
   * Store informations about a measurement.
   */
  typedef struct MeasurementInfos
  {
    MeasurementId   measurement; ///< The measurement
    const char     *psName;      ///< The measurement's name.

    MeasurementType valueType;   ///< The measurement's value type.
    CommandMIdFlag  cmdIdFlag;   ///< The flag and identifier of the SDI-12 Start Measurement comma d to use to get the measurement.
    uint8_t         valuePos;    ///< The position in the SDI-12 data response. First value has index 0.

    bool            requireGPS;  ///< Indicate if the GPS is required to get this measurement.
  }
  MeasurementInfos;

  /**
   * Used to store informations about a MaxiMet sensor model.
   */
  typedef struct Model
  {
    const char  *psName;        ///< The model's name. For example; "GMX101", "GMX550", "GMX600", ...
    Measurements measurements;  ///< The measurements provided by this model.
  }
  Model;
#define GILL_MAXIMET_SDI12_NB_MODELS  14  ///< Number of sensor models

#define GILL_MAXIMET_SDI12_PART_NUMBER_LEN          16  ///< Length of the sensor part number.
#define GILL_MAXIMET_SDI12_PART_NUMBER_MODEL_POS    5   ///< Position of the model number first digit in the part number
#define GILL_MAXIMET_SDI12_PART_NUMBER_GPS_IND_POS  13  ///< Position of the GPS presence indication digit in the part number

#define GILL_MAXIMET_SDI12_TYPE_SIZE   16  ///< The type returned by the sensor: "Gill-GMXxxx" or "Gill-GMXxxx-GPS"

  /**
   * Defines the data type stored in the sensor's state file
   */
  typedef struct StateSpecific
  {
    State    base;                 ///< The state base.

    ts2000_t tsLastRead;           ///< Timestamp of the last sensor's read.
    float    lastPrecipitationMM;  ///< The precipitation, in millimiters, form sensor's last read.
    uint32_t sensorDate;           ///< The sensor's date. Is a number composed like this: YYYYMMDD.
  }
  StateSpecific;


public:
  GillMaximMetSDI12();
  ~GillMaximMetSDI12();
  static Sensor *getNewInstance();
  const char    *type();


protected:
  bool jsonSpecificHandler(const JsonObject& json);


private:
  void          clearReadings();
  const Model  *getModelFromPartNumber( const char *psPartNumber, bool *pbHasGPS = NULL);
  MeasurementId getMesurementIdFromName(const char *psName);

  bool setUniqueIdAndFirmwareVersionFromGetIdCmd(const SDI12Command *pvCmd);

  bool getMeasurementsFromCmdResponse(CommandMId          cmdId,
				      const SDI12Command *pvCmd,
				      Measurements        measurementsToGet);

  State *defaultState(uint32_t *pu32_size);

  bool readSpecific();
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  const Model  *_pvModel;                    ///< The sensor's model.
  char          _psType[GILL_MAXIMET_SDI12_TYPE_SIZE];  ///< The sensor's type.
  bool          _hasGPS;                     ///< Indicate if the sensor has a GPS or not.
  bool          _resetRainCountAtMidnight;   ///< Is the station's rain count reseted at midnight (true), or does it use a continuous count.
  Measurements  _measurementsToGet;          ///< Measurements to get from the sensor.
  Measurements  _measurementsWeGot;          ///< The measurements we got from the sensor.

  uint16_t      _windDirectionDegRelative;   ///< In degrees clockwise from North.
  float         _windSpeedMPerSecRelative;   ///< Wind speed, in m/s.
  uint16_t      _windDirectionDegCorrected;  ///< In degrees clockwise from North.
  float         _windSpeedMPerSecCorrected;  ///< Wind speed, in m/s.
  float         _temperatureDegC;            ///< Temperature, in °C.
  uint8_t       _relHumidityPercent;         ///< Relative humidity, in %.
  float         _pressureHPa;                ///< Pressure, in hectopascals (hPa).
  float         _precipitationMM;            ///< Precipitation, in millimeters.
  uint32_t      _precipitationPeriodSec;     ///< The amount of time, in seconds, the precipitation value corresponds to.
  uint32_t      _solarRadiationWPerM2;       ///< Solar radiations, in Watts per square meter.
  float         _latitudeDegDeci;            ///< position's latitude, in decimal degrees.
  float         _longitudeDegDeci;           ///< position's longitude, in decimal degrees.
  float         _altitudeM;                  ///< position's altitude, in meters from mean sea level.
  Datetime      _datetime;                   ///< The sensor's time.

  const char   *_csvHeaderValues[GILL_MAXIMET_SDI12_NB_MEASUREMENTS + 1];

  StateSpecific _state;  ///< The sensor's state.

  /**
   * List the measurements' infos.
   */
  static MeasurementInfos _MeasurementsInfos[GILL_MAXIMET_SDI12_NB_MEASUREMENTS];

  /**
   * List the sensor models.
   */
  static Model _models[GILL_MAXIMET_SDI12_NB_MODELS];
};

#endif // USE_SENSOR_GILL_MAXIMET_SDI12
#endif /* SENSORS_EXTERNAL_GILLMAXIMETSDI12_H_ */
