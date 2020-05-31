/*
 * Base class for sensors using SDI-12 communication
 *
 *  Created on: Jul 18, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include <string.h>
#include "sensor_sdi12.hpp"
#include "sdi12.h"
#include "sdi12gen_manager.h"
#include "sdi12gen_standardcommands.h"
#include "board.h"


  CREATE_LOGGER(sensor_sdi12);
#undef  logger
#define logger  sensor_sdi12


/**
 * Constructor.
 *
 * @param[in] power        the power configuration required when the node is active.
 * @param[in] powerSleep   the power configuration required when the node is asleep.
 * @param[in] pvSensorCmds the sensor's specific commands' generic descriptions.
 *                         If the sensor has no specific commands then set it to NULL.
 * @param[in] features     the features provided by the sensor.
 */
SensorSDI12::SensorSDI12(Power                         power,
			 Power                         powerSleep,
			 const SDI12GenSensorCommands *pvSensorCmds,
			 Features                      features) :
// Add POWER_EXTERNAL_INT because the SDI-12 interface is powered by the external interruptions
// power source ?!!
Sensor(power | POWER_SDI12, powerSleep, features)
{
  this->_address = 0;

  this->_pvSensorCmds = pvSensorCmds;
  if(pvSensorCmds) { sdi12_gen_mgr_register_sensor_command_descriptions(pvSensorCmds); }

  clearCmdContext();
}

/**
 * Destructor.
 */
SensorSDI12::~SensorSDI12()
{
  // Do nothing
}


/**
 * Parse the configuration for SDI-12 sensor general configuration parameters.
 */
bool SensorSDI12::jsonSpecificHandler(const JsonObject& json)
{
  const char *psValue = json["address"].as<const char *>();
  if(psValue && !psValue[1] && sdi12_is_address_valid(psValue[0])) { this->_address = psValue[0]; }
  if(!this->_address)
  {
    if(psValue) { log_error_sensor(logger, "Invalid SDI-12 sensor address: '%s'.", psValue); }
    else        { log_error_sensor(logger, "SDI-12 sensor must have an 'address' configuration parameter."); };
    return false;
  }

  return true;
}


const char *SensorSDI12::uniqueId()
{
  if(hasFeature(FEATURE_CAN_DETECT_UNIQUE_ID))
  {
    getUniqueIdAndFirmwareVersion();
  }

  return Sensor::uniqueId();
}

const char *SensorSDI12::firmwareVersion()
{
  if(hasFeature(FEATURE_CAN_DETECT_FIRMWARE_VERSION))
  {
    getUniqueIdAndFirmwareVersion();
  }

  return Sensor::firmwareVersion();
}

/**
 * Get the unique identifier and the firmware version using the Get Identifier SDI-12 command.
 *
 * @return true  if the command has been executed with success.
 * @return false otherwise.
 */
bool SensorSDI12::getUniqueIdAndFirmwareVersion()
{
  const SDI12Command *pvCmd;
  bool                closeOnExit;
  bool                ok = true;

  if(*Sensor::firmwareVersion()) { goto exit; }

  // Open the sensor if it's not already the case.
  if(!isOpened())
  {
    if(!open()) { goto exit; }
    closeOnExit = true;
  }

  // Get the serial number from SDI-12 identification command
  if(!sendCommandUsingName(SDI12_CMD_STD_SEND_ID, NULL, &pvCmd))
  {
    log_error_sensor(logger, "Failed to get identification from sensor.");
    ok = false;
    goto exit;
  }
  //ConnecSenS::addVerboseSyslogEntry(name(), "Identification: %s.", (const char *)pvCmd->pu8_buffer + SDI12_ADDRESS_LEN);

  ok = setUniqueIdAndFirmwareVersionFromGetIdCmd(pvCmd);

  exit:
  if(closeOnExit) { close(); }
  return ok;
}

/**
 * Set the sensor's unique identifier and the firmware version using the data obtained from
 * calling the SDI-12 Send Identification command.
 *
 * @param[in] pvCmd the command's response. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::setUniqueIdAndFirmwareVersionFromGetIdCmd(const SDI12Command *pvCmd)
{
  char       *buffer;
  const char *pc;
  uint8_t    *pu8Data, size, i;
  bool        ok = true;

  // Get the command's data
  if(!(pu8Data = sdi12_command_response_data(pvCmd, &size)) || size < 19)
  {
    log_error_sensor(logger, "Send Identification response is too short.");
    ok = false;
    goto exit;
  }

  // Set the firmware version
  buffer = (char *)Sensor::firmwareVersion();
  strlcpy(buffer, (const char *)pu8Data + 16, 4);

  // Set the unique id if not already set
  if(*(buffer = (char *)Sensor::uniqueId())) { goto exit; }
  // Copy vendor id
  for(pc = (const char *)pu8Data + 2,  i = 0; i < 8 && *pc != ' '; i++) { *buffer++ = *pc++; }
  *buffer++ = '-';
  // Copy model id
  for(pc = (const char *)pu8Data + 10, i = 0; i < 6 && *pc != ' '; i++) { *buffer++ = *pc++; }
  // Copy serial number if one is set
  if(size > 19)
  {
    *buffer++ = '-';
    for(pc = (const char *)pu8Data + 19, i = 19; i < size; i++) { *buffer++ = *pc++; }
  }
  *buffer = '\0';

  exit:
  return ok;
}


bool SensorSDI12::openSpecific()
{
  board_sdi12_init();
  return true;
}

void SensorSDI12::closeSpecific()
{
  // Do nothing
}


/**
 * Clear the SDI-12 command execution context.
 */
void SensorSDI12::clearCmdContext()
{
  this->_processingCommand        = false;
  this->_lastCommandWasSuccessful = false;
  this->_pvLastCmd                = NULL;
}


/**
 * Send an SDI-12 command using it's name.
 *
 * @param[in]  psCmdName      the name of the command to send. MUST be NOT NULL.
 * @param[in]  psCmdVarValues the string containing the list of key-value pairs used to
 *                            replace the variables in the command description with actual values.
 *                            Can only be NULL or empty if there is no variable to replace
 *                            in the command string.
 * @param[out] ppvCmd         the the command data, including the result are set to.
 *                            This will be set to point to the address of some memory somewhere
 *                            and its is only valid inside this function.
 *                            It can be recycled when we are outside of this function.
 *                            In particular, it may be recycled when the next SDI-12 command is sent.
 *                            So, if you need to keep values then you need to copy them.
 *                            Can be set to NULL if you don't need it.
 *                            *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandUsingName(const char *psCmdName,
				       const char *psCmdVarValues,
				       const SDI12Command **ppvCmd)
{
  if(ppvCmd) { *ppvCmd = NULL; }
  if(this->_processingCommand) { return false; }

  // Send the command
  clearCmdContext();
  if(!sdi12_gen_mgr_send_command(SDI12_INTERFACE_NAME,
				 psCmdName,
				 (this->_pvSensorCmds ? this->_pvSensorCmds->ps_sensor_name : NULL),
				 this->_address,
				 psCmdVarValues,
				 &cmdSuccessCallback, this,
				 &cmdFailedCallback,  this))
  {
    return false;
  }
  this->_processingCommand = true;

  // Animate SDI-12 layer
  while(this->_processingCommand) { while(sdi12_gen_mgr_process()) { /* Do nothing */ } }

  if(ppvCmd) { *ppvCmd = this->_pvLastCmd; }
  return this->_lastCommandWasSuccessful;
}

/**
 * Send a SDI-12 Acknowledge Active command and wait for response.
 *
 * @param[out] ppvCmd the the command data, including the result are set to.
 *                    This will be set to point to the address of some memory somewhere
 *                    and its is only valid inside this function.
 *                    It can be recycled when we are outside of this function.
 *                    In particular, it may be recycled when the next SDI-12 command is sent.
 *                    So, if you need to keep values then you need to copy them.
 *                    Can be set to NULL if you don't need it.
 *                    *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandAcknowledgeActive(const SDI12Command **ppvCmd)
{
  return sendCommandUsingName(SDI12_CMD_STD_ACK_ACTIVE, NULL, ppvCmd);
}

/**
 * Send a SDI-12 Send Identification command and wait for response.
 *
 * @param[out] ppvCmd the the command data, including the result are set to.
 *                    This will be set to point to the address of some memory somewhere
 *                    and its is only valid inside this function.
 *                    It can be recycled when we are outside of this function.
 *                    In particular, it may be recycled when the next SDI-12 command is sent.
 *                    So, if you need to keep values then you need to copy them.
 *                    Can be set to NULL if you don't need it.
 *                    *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandSendIdentification(const SDI12Command **ppvCmd)
{
  return sendCommandUsingName(SDI12_CMD_STD_SEND_ID, NULL, ppvCmd);
}

/**
 * Send a SDI-12 Address Query command and wait for response.
 *
 * @param[out] ppvCmd the the command data, including the result are set to.
 *                    This will be set to point to the address of some memory somewhere
 *                    and its is only valid inside this function.
 *                    It can be recycled when we are outside of this function.
 *                    In particular, it may be recycled when the next SDI-12 command is sent.
 *                    So, if you need to keep values then you need to copy them.
 *                    Can be set to NULL if you don't need it.
 *                    *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandAdressQuery(const SDI12Command **ppvCmd)
{
  return sendCommandUsingName(SDI12_CMD_STD_QUERY_ADDRESS, NULL, ppvCmd);
}

/**
 * Send a SDI-12 Address Query command and wait for response.
 *
 * @param[in]  newAddress the sensor's new address to set.
 * @param[out] ppvCmd     the the command data, including the result are set to.
 *                        This will be set to point to the address of some memory somewhere
 *                        and its is only valid inside this function.
 *                        It can be recycled when we are outside of this function.
 *                        In particular, it may be recycled when the next SDI-12 command is sent.
 *                        So, if you need to keep values then you need to copy them.
 *                        Can be set to NULL if you don't need it.
 *                       *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandChangeAddress(char newAddress, const SDI12Command **ppvCmd)
{
  char varValues[14];

  strlcpy(varValues, "new_address:a", sizeof(varValues));
  varValues[12] = newAddress;

  return sendCommandUsingName(SDI12_CMD_STD_CHANGE_ADDRESS, varValues, ppvCmd);
}

/**
 * Send a SDI-12 Start Measurement or Start Additional Measurement command and wait for response.
 *
 * @param[in]  id         the Start Measurement command identifier.
 *                        If 0 then send a Start Measurement command.
 *                        If in range [1..9] then send a Start Additional Measurement with id
 *                        as identifier.
 * @param[in]             Use a communication with CRCs (true), or not (false)?
 * @param[out] ppvCmd     the the command data, including the result are set to.
 *                        This will be set to point to the address of some memory somewhere
 *                        and its is only valid inside this function.
 *                        It can be recycled when we are outside of this function.
 *                        In particular, it may be recycled when the next SDI-12 command is sent.
 *                        So, if you need to keep values then you need to copy them.
 *                        Can be set to NULL if you don't need it.
 *                       *pvCmd can be set to NULL if we were unable to send the command.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorSDI12::sendCommandStartMeasurement(uint8_t id, bool withCRC, const SDI12Command **ppvCmd)
{
  const char *psCmd;

  if(id > 9) { return false; }

  if(id) { psCmd = withCRC ? SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS_WITH_CRC : SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS; }
  else   { psCmd = withCRC ? SDI12_CMD_STD_START_MEASUREMENT_WITH_CRC       : SDI12_CMD_STD_START_MEASUREMENT;       }

  return sendCommandUsingName(psCmd, sdi12_gen_stdcmds_get_additionalm_varvalues_using_id(id), ppvCmd);
}

/**
 * Function called when the last command sent was successful.
 *
 * @param[in] pvCmd  the SDI-12 command object. Is not NULL.
 * @param[in] pvArgs the SDI-12 sensor object that sent the command.
 */
void SensorSDI12::cmdSuccessCallback(SDI12Command *pvCmd, void *pvArgs)
{
  SensorSDI12 *pvSensor = (SensorSDI12 *)pvArgs;

  pvSensor->_lastCommandWasSuccessful = true;
  pvSensor->_processingCommand        = false;
  pvSensor->_pvLastCmd                = (const SDI12Command*)pvCmd;
}

/**
 * Function called when the last command sent failed.
 *
 * @param[in] pvCmd  the SDI-12 command object. Is not NULL.
 * @param[in] pvArgs the SDI-12 sensor object that sent the command.
 */
void SensorSDI12::cmdFailedCallback(SDI12Command *pvCmd, void *pvArgs)
{
  SensorSDI12 *pvSensor = (SensorSDI12 *)pvArgs;

  pvSensor->_lastCommandWasSuccessful = false;
  pvSensor->_processingCommand        = false;
  pvSensor->_pvLastCmd                = (const SDI12Command*)pvCmd;
}

