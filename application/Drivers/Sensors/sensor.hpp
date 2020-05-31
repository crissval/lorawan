/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Sensor Parent Class for ConnecSenS Device **************/
/**********************************************************/
#pragma once

#include <stdio.h>
#include <string.h>
#include "buffer.hpp"
#include "periodic.hpp"
#include "json.hpp"
#include "cnssrf.h"
#include "cnssintclient.hpp"
#include "logger.h"
#include "board.h"

#define	SENSOR_NAME_MAX_SIZE              (CNSSRF_DT_CONFIG_VALUE_LEN_MAX + 1)
#define SENSOR_STATE_FILENAME_SIZE_MAX    48
#define SENSOR_UNIQUE_ID_SIZE_MAX         (CNSSRF_DT_CONFIG_VALUE_LEN_MAX + 1)
#define SENSOR_FIRMWARE_VERSION_SIZE_MAX  (CNSSRF_DT_CONFIG_VALUE_LEN_MAX + 1)

// Define some logging macros dedicated to sensors so that the sensor's name is is automatically added.
#define log_fatal_sensor(nm, msg, ...)  log_fatal(nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))
#define log_error_sensor(nm, msg, ...)  log_error(nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))
#define log_warn_sensor( nm, msg, ...)  log_warn( nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))
#define log_info_sensor( nm, msg, ...)  log_info( nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))
#define log_debug_sensor(nm, msg, ...)  log_debug(nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))
#define log_trace_sensor(nm, msg, ...)  log_trace(nm, "%s: " msg, name() _VARGS_(__VA_ARGS__))


class Sensor : public ClassPeriodic, CNSSIntClient
{
public:
  typedef enum
  {
    POWER_NONE         = BOARD_POWER_NONE,
    POWER_INTERNAL     = BOARD_POWER_INTERNAL_SENSORS,
    POWER_EXTERNAL     = BOARD_POWER_EXT_ADJ,
    POWER_EXTERNAL_INT = BOARD_POWER_EXT_INT,
    POWER_SDI12        = BOARD_POWER_SDI12
  }
  PowerFlag;
  typedef BoardPower Power;

  /**
   * List what the sensor is capable of.
   */
  typedef enum Feature
  {
    FEATURE_BASE                         = 0x00,     ///< The sensor provides the basic features.
    FEATURE_CAN_DETECT_UNIQUE_ID         = 1u << 0,  ///< The sensor can find a unique id by himself.
    FEATURE_CAN_DETECT_FIRMWARE_VERSION  = 1u << 1,  ///< The sensor can report its firmware version.
    FEATURE_CAN_FOLLOW_SENSOR_S_PACE     = 1u << 2   ///< The reading period can be set to follow the sensor's output pace.
  }
  Feature;
  typedef uint8_t Features;   ///< Type used to store features.

  /**
   * Defines the type of periods that a sensor can use.
   */
  typedef enum PeriodType
  {
    PERIOD_TYPE_FROM_NODE_REGULAR,  ///< The sensor's reading period is regular and driven by the node.
    PERIOD_TYPE_AT_SENSOR_S_FLOW    ///< The reading period is given by the sensor and it's the sensor output flow rhythm.
  }
  PeriodType;

  typedef uint32_t TypeHash;  ///< The type used to store the sensor's type hash.

  /**
   * Defines the values that can be prepended to the CSV header values.
   */
  typedef enum CSVHeaderPreprend
  {
    CSV_HEADER_PREPEND_NONE,  ///< Do not prepend anything.
    CSV_HEADER_PREPEND_NAME   ///< Prepend sensor's name.
  }
  CSVHeaderPreprend;


protected:
  /**
   * Defines the sensor's state base class.
   */
  typedef struct State
  {
    uint8_t version   : 4;  ///< The state format version.
    uint8_t isInAlarm : 1;  ///< Is the sensor in alarm?
    uint8_t unused    : 3;  ///< Unused bits; reserved for future needs.
  }
  State;


public :
  Sensor(Power power, Power powerSleep, Features features = FEATURE_BASE);
  virtual ~Sensor();

  void 	          setName(const char *psName);
  const char *    name() const                                        { return  this->_name;                         }
  Features        features()                                          { return this->_features;                      }
  bool            hasFeature(Feature  feature)                        { return (this->_features & feature);          }
  bool            isTriggerable() const                               { return  this->_intSensitivityRead          != CNSSInt::INT_FLAG_NONE; }
  bool            isTriggerableBy(CNSSInt::Interruptions flags) const { return (this->_intSensitivityRead & flags) != CNSSInt::INT_FLAG_NONE; }
  Power           powerConfig() const                                 { return  this->_power;                        }
  void            addPower(Power power)                               { this->_power |= power; }
  Power           powerConfigSleep() const                            { return  this->_powerSleep;                   }
  bool            usePower(Power power) const                         { return (this->_power & power) != POWER_NONE; }
  bool            hasNewData() const                                  { return  this->_hasNewData;                   }
  void            clearHasNewData()                                   { this->_hasNewData = false;                   }
  bool            isOpened() const                                    { return  this->_isOpened;                     }
  bool            needsToBeConstantlyOpened() const                   { return  this->_periodType == PERIOD_TYPE_AT_SENSOR_S_FLOW; }
  ts2000_t        readingsTimestamp() const                           { return  this->_readingsTs2000;               }
  ts2000_t        interruptionTimestamp() const                       { return this->_interruptionTs2000;            }
  void            clearInterruptionTimestamp()                        { this->_interruptionTs2000 = 0;               }
  bool            isInAlarm();
  bool            alarmStatusHasJustChanged(bool clear = true);
  virtual bool    readOnAlarmChange();
  bool            requestToSendData(        bool clear = true);
  bool            setConfiguration(const JsonObject& json);
  void            setWriteTypeHashToCNSSRFFrames(bool write = true) { this->_writeTypeHashToCNSSRF = write; }

  void              setCNSSRFDataChannel(CNSSRFDataChannel channel) { this->_dataChannel = channel; }
  CNSSRFDataChannel cnssrfDataChannel() const                       { return this->_dataChannel;    }
  bool              writeDataToCNSSRFDataFrame(CNSSRFDataFrame *pv_frame);

  const char *getStateFilename(bool create = false);
  static bool removeAllStates();

  PeriodType periodType() const { return this->_periodType; }
  void       setPeriodSec(uint32_t secs);
  void       setPeriodAtSensorSFlow();
  bool       itsTime(bool updateNextTime = true);

  /**
   * Function called to process tasks awaiting to be done, if there are any.
   *
   * @return true  if there are more processing left to do.
   * @return false if all the tasks have been done.
   */
  virtual bool process();

  bool open();
  void close();
  bool read();
  bool configAlert();

  virtual void        processInterruption(CNSSInt::Interruptions ints);

  TypeHash            typeHash(bool *pb_ok = NULL);
  virtual const char *uniqueId();
  virtual void        setUniqueId(const char *psUID);
  bool                hasUniqueId() { return *uniqueId() != '\0'; }

  virtual const char *firmwareVersion();
  virtual void        setFirmwareVersion(const char *psFwv);

  uint32_t csvHeader(char *ps_header, uint32_t size, CSVHeaderPreprend prepend);
  int32_t  csvData(  char *ps_data,   uint32_t size);


protected:
  void  setReadingsTimestamp(    ts2000_t ts = 0);
  void  setInterruptionTimestamp(ts2000_t ts = 0);
  void  setHasNewData(       bool hasNewData = true);
  void  setHasAlarmToSet(    bool set        = true) { this->_hasAlarmToSet   = set;        }
  void  setRequestSendData(  bool request    = true) { this->_requestSendData = request;    }
  void  setIsInAlarm(        bool alarm      = true);

  void  setWriteTypeHash(bool set = true) { this->_writeTypeHashToCNSSRF = set; }

  void  setSpecificIntSensitivity(CNSSInt::Interruptions sensitivity);

  State *state();
  bool   saveState(bool force = false);

  uint8_t csvNbValues() const { return this->_csvNbValues; }
  int32_t csvMakeStringUsingStringValues(char        *ps_data,
					 uint32_t     size,
					 const char **pps_values,
					 uint32_t     nb_values);


private:
  void  setIntSensitivity(CNSSInt::Interruptions sensitivity, bool append);
  void  setCurrentStateAsReference();
  bool  stateHasChanged();

  virtual bool configAlertSpecific();
  virtual bool jsonAlarmSpecific(           const JsonObject&       json,
					    CNSSInt::Interruptions *pvAlarmInts);
  virtual bool jsonSpecificHandler(         const JsonObject&       json);
  virtual void processAlarmInterruption(    CNSSInt::Interruptions  ints);
  virtual void processInterruptionSpecific( CNSSInt::Interruptions  ints);
  virtual bool currentValuesAreInAlarmRange();

  /**
   * This function returns the state object to use as default state object.
   *
   * @param[in] pu32_size where the state object's size, in bytes, is written to. IS not NULL.
   *
   * @return the default state object to use. It MUST be valid as long as the Sensor's object
   *         exists.
   * @return NULL if the sensor do not use state.
   */
  virtual State *defaultState(uint32_t *pu32_size);


  //=============== Functions that MUST be implemented by sub-classes.
public:
  virtual const char *type()   = 0;

protected:
  virtual bool     openSpecific()  = 0;
  virtual void     closeSpecific() = 0;
  virtual bool     readSpecific()  = 0;
  virtual bool     writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame) = 0;

  /**
   * Return the CSV header values.
   *
   * @return the list of header values, as strings. The list is NULL terminated.
   * @return NULL in case of error.
   */
  virtual const char **csvHeaderValues() = 0;

  /**
   * Write CSV data to a buffer.
   *
   * This function is called when the sensor has data to write.
   * So no need to handle the case where no data are available.
   *
   * @param[out] ps_data where the data are written to. A string is written. MUST be NOT NULL.
   * @param[in]  size    ps_data's size.
   *
   * @return the CSV data string length. Can be 0 if the sensor outputs only one value and
   *         no value is available.
   * @return -1 if ps_data is too small.
   * @return -1 for any other type of error.
   */
  virtual int32_t csvDataSpecific(char *ps_data, uint32_t size) = 0;


private:
  Features               _features;               ///< The sensor's features.
  char 	                 _name[SENSOR_NAME_MAX_SIZE];
  char                   *_psStateFilename;
  char                   _uniqueId[       SENSOR_UNIQUE_ID_SIZE_MAX];        ///< The sensor's unique identifier.
  char                   _firmwareVersion[SENSOR_FIRMWARE_VERSION_SIZE_MAX]; ///< The sensor's firmware version.
  PeriodType             _periodType;             ///< The type of sensor reading period to use.
  CNSSInt::Interruptions _intSensitivity;         ///< The interruption flags this sensor is sensitive to
  CNSSInt::Interruptions _intSensitivityRead;     ///< The interruption flags that trigger a sensor reading.
  CNSSInt::Interruptions _intSensitivityToSend;   ///< The interruption flags that trigger a data transmission.
  CNSSInt::Interruptions _intSensitivityToAlarm;  ///< The interruption flags that correspond to the alarm being triggered.
  CNSSInt::Interruptions _intSensitivitySpecific; ///< The interruption flags that trigger specific processing.
  bool                   _sendOnInterrupt;        ///< Send data on interruption?
  bool                   _sendOnAlarmSet;         ///< Send data when the alarm has been triggered?
  bool                   _sendOnAlarmCleared;     ///< Send data when the alarm has been cleared?
  bool                   _requestSendData;        ///< Do the sensor request to send data?
  Power                  _power;                  ///< Active power configuration
  Power                  _powerSleep;             ///< Sleep mode power configuration
  bool                   _hasNewData;             ///< Indicate if new data have been red from the sensor
  CNSSRFDataChannel      _dataChannel;            ///< The ConnecSenS RF format data channel assigned to this sensor.
  bool                   _isOpened;               ///< Indicate if the sensor is opened or not.
  bool                   _hasAlarmToSet;          ///< Is there an alarm to set?
  bool                   _alarmStatusJustChanged; ///< Has the alarm status just changed?
  State                 *_pvState;                ///< The sensor's state.
  uint32_t               _stateSize;              ///< The state object's size, in bytes.
  uint8_t               *_pu8StateRef;            ///< State reference.
  uint32_t               _periodNormalSec;        ///< The sensor's normal period, in seconds.
  uint32_t               _periodAlarmSec;         ///< The sensor's period, in seconds, when it is in alarm.
  ts2000_t               _readingsTs2000;         ///< The last reading's timestamp. 0 if no timestamp.
  ts2000_t               _interruptionTs2000;     ///< Timestamp of the last interruption. 0 mean no timestamp.
  TypeHash               _typeHash;               ///< The sensor's type hash.
  bool                   _writeTypeHashToCNSSRF;  ///< Write the sensor's type hash to CNSSRF frames?
  bool                   _typeHashComputed;       ///< Indicate if the sensor's type hash has been computed or not.
  uint8_t                _csvNbValues;            ///< The number of CSV values generated by this sensor.
};

