/*
 * Base class for sensors using SDI-12 communication
 *
 *  Created on: Jul 18, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef SENSORS_SENSOR_SDI12_HPP_
#define SENSORS_SENSOR_SDI12_HPP_

#include "sensor.hpp"
#include "sdi12gen.h"


class SensorSDI12 : public Sensor
{


public:
  SensorSDI12(Power                         power,
	      Power                         powerSleep,
	      const SDI12GenSensorCommands *pvSensorCmds,
	      Features                      features =
		  FEATURE_CAN_DETECT_UNIQUE_ID | FEATURE_CAN_DETECT_FIRMWARE_VERSION);
  virtual ~SensorSDI12();

  virtual const char *uniqueId();
  virtual const char *firmwareVersion();

  char address() { return _address; }


protected:
  virtual bool jsonSpecificHandler(const JsonObject& json);

  bool sendCommandUsingName(const char *psCmdName,
			    const char *psCmdVarValues,
			    const SDI12Command **ppvCmd);

  bool sendCommandAcknowledgeActive( const SDI12Command **ppvCmd);
  bool sendCommandSendIdentification(const SDI12Command **ppvCmd);
  bool sendCommandAdressQuery(       const SDI12Command **ppvCmd);
  bool sendCommandChangeAddress(     char newAddress,          const SDI12Command **ppvCmd);
  bool sendCommandStartMeasurement(  uint8_t id, bool withCRC, const SDI12Command **ppvCmd);


private:
  bool         getUniqueIdAndFirmwareVersion();
  virtual bool setUniqueIdAndFirmwareVersionFromGetIdCmd(const SDI12Command *pvCmd);

  virtual bool openSpecific();
  virtual void closeSpecific();

  void        clearCmdContext();
  static void cmdSuccessCallback(SDI12Command *pvCmd, void *pvArgs);
  static void cmdFailedCallback( SDI12Command *pvCmd, void *pvArgs);


private:
  const SDI12GenSensorCommands *_pvSensorCmds; ///< The SDI-12 commands specific to this sensor. Can be NULL.

  char _address; ///< the sensor's address

  bool                _processingCommand;         ///< Are we currently processing a command.
  bool                _lastCommandWasSuccessful;  ///< Was the last command successful?
  const SDI12Command *_pvLastCmd;                 ///< The last command processed.
};


#endif /* SENSORS_SENSOR_SDI12_HPP_ */
