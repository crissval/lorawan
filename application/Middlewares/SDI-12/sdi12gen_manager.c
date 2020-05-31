/**
 * @file  sdi12gen_manager.c
 * @brief 
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "sdi12gen_manager.h"
#include "logger.h"
#include "sdi12gen.h"
#include "sdi12gen_standardcommands.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

  CREATE_LOGGER(sdi12_gen_manager);
#undef  _logger
#define _logger sdi12_gen_manager

#ifndef SDI12_CMD_TIMEOUT_MS
#define SDI12_CMD_TIMEOUT_MS 2000
#endif


#define SDI12_GEN_MGR_CMD_CMD_BUFFER_SIZE_MIN  (\
    SDI12_ADDRESS_LEN +\
    10                +\
    SDI12_COMMAND_END_OF_COMMAND_PATTERN)
#define SDI12_GEN_MGR_CMD_RESP_BUFFER_SIZE_MIN (\
    SDI12_ADDRESS_LEN        +\
    SDI12_CMD_VALUES_LEN_MAX +\
    SDI12_COMMAND_CRC_LEN    +\
    SDI12_COMMAND_END_OF_RESPONSE_LEN)
#define SDI12_GEN_MGR_CMD_BUFFER_SIZE_MIN ( \
    SDI12_GEN_MGR_CMD_CMD_BUFFER_SIZE_MIN  +\
    SDI12_GEN_MGR_CMD_RESP_BUFFER_SIZE_MIN +\
    SDI12_COMMAND_RX_BUFFER_EXTRA_LEN       \
  )

#define ALL_DATA_LEN_MAX (               \
    SDI12_ADDRESS_LEN                  + \
    SDI12_CMD_VALUES_LEN_MAX * 10      + \
    SDI12_COMMAND_END_OF_RESPONSE_LEN  + \
    SDI12_COMMAND_RX_BUFFER_EXTRA_LEN)

#define SDI12_MGR_ALL_DATA_BUFFER_SIZE_MIN ( \
    SDI12_ADDRESS_LEN                 +      \
    SDI12_COMMAND_VALUE_LEN_MAX       +      \
    SDI12_COMMAND_END_OF_RESPONSE_LEN +      \
    SDI12_COMMAND_RX_BUFFER_EXTRA_LEN)

#ifndef SDI12_GEN_MGR_CMD_BUFFER_SIZE
#define SDI12_GEN_MGR_CMD_BUFFER_SIZE  128
#else
#if SDI12_GEN_MGR_CMD_BUFFER_SIZE < SDI12_GEN_MGR_CMD_BUFFER_SIZE_MIN
#error SDI12_GEN_MGR_CMD_BUFFER_SIZE is too small; it must be at least SDI12_GEN_MGR_CMD_BUFFER_SIZE_MIN bytes.
#endif
#endif

#ifdef SDI12_MGR_ALL_DATA_BUFFER_SIZE
#if SDI12_MGR_ALL_DATA_BUFFER_SIZE <= SDI12_MGR_ALL_DATA_BUFFER_SIZE_MIN
#error SDI12_MGR_ALL_DATA_BUFFER_SIZE is too small. The buffer must be able to contain at least one value, address and end of response sequence.
#endif
#else
#define SDI12_MGR_ALL_DATA_BUFFER_SIZE  ALL_DATA_LEN_MAX
#endif

#ifndef SDI12_GEN_MGR_EXECCTX_STR_BUILDER_BUFFER_SIZE
#define SDI12_GEN_MGR_EXECCTX_STR_BUILDER_BUFFER_SIZE  64
#endif


  struct SDI12GenMgrInterface;
  typedef struct SDI12GenMgrInterface SDI12GenMgrInterface;


  /**
   * Defines the state machine states.
   */
  typedef enum SDI12GenMgrState
  {
    SDI12_GEN_MGR_STATE_IDLE_UNKNOWN = 0,
    SDI12_GEN_MGR_STATE_IDLE,
    SDI12_GEN_MGR_STATE_EXECUTING_COMMAND,
    SDI12_GEN_MGR_STATE_LAST_COMMAND_SUCCESS,
    SDI12_GEN_MGR_STATE_LAST_COMMAND_FAILED,
    SDI12_GEN_MGR_STATE_START_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT,
    SDI12_GEN_MGR_STATE_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT,
    SDI12_GEN_MGR_STATE_RECEIVED_SERVICE_REQUEST,
    SDI12_GEN_MGR_STATE_IT_S_TIME_TO_GET_THE_VALUES,
    SDI12_GEN_MGR_STATE_GETTING_VALUES,
    SDI12_GEN_MGR_STATE_GOT_DATA_FRAME
  }
  SDI12GenMgrState;


  /**
   * Defines a Command execution context.
   */
  typedef struct SDI12GenMgrCmdExecContext
  {
    SDI12GenMgrInterface *p_mgr_interface;   ///< The information on the interface used to send the command.

    SDI12Command command;                                   ///< The command.
    uint8_t      cmd_buffer[SDI12_GEN_MGR_CMD_BUFFER_SIZE]; ///< The buffer used by the command.

    uint8_t nb_values_left_to_get; ///< The number of values we still have to retrieve.
    uint8_t next_data_frame;       ///< The identifier of the next data frame to get.

    /**
     * All the data retrieved from one sensor.
     * First byte is the sensor's address.
     * Last two bytes are are <CR><LF>.
     * There is no CRC, even if the values were retrieved with CRC.
     */
    uint8_t  pu8_all_data[SDI12_MGR_ALL_DATA_BUFFER_SIZE];
    uint16_t all_data_length;      ///< The number of data bytes in <code>pu8_all_data</code>
    uint32_t timeout_ms;           ///< A timeout value, in milliseconds.

    const char *ps_next_cmd_name;             ///< The name of the next command to call. Can be NULL.
    const char *ps_next_cmd_sensor_type_name; ///< The type name of the sensor the next command to call belongs to. Can be NULL.

    char ps_string_builder_buffer[SDI12_GEN_MGR_EXECCTX_STR_BUILDER_BUFFER_SIZE]; ///< Buffer used to build strings; in particular the key-value pairs used to initialise generic commands.

    sdi12_command_result_cb_t success_cb;        ///< The end result command success callback function to call. Can be NULL.
    void                     *pv_success_cbargs; ///< The arguments to pass to the success callback function. Can be NULL.
    sdi12_command_result_cb_t failed_cb;         ///< The end result command failure callback function to call. Can be NULL.
    void                     *pv_failed_cbargs;  ///< The arguments to pass to the failure callback function. Can be NULL.
  }
  SDI12GenMgrCmdExecContext;


  /**
   * Structure used to store interfaces' informations.
   */
  struct SDI12GenMgrInterface
  {
    const char      *ps_name;     ///< The interface's name.
    SDI12Interface  *p_interface; ///< The SDI-12 interface.
    SDI12GenMgrState state;       ///< The state the interface is into.

    // TODO: use several execution contexts so that we can use concurrent commands.
    SDI12GenMgrCmdExecContext exec_ctx; ///< Command execution context.
  };


  /**
   * Defines the type used to store all the sensor's command descriptions and the standard commands.
   */
  typedef struct SDI12GenMgrSensorsCommandDescriptions
  {
    const SDI12GenCommandDescription *p_standard; ///< The descriptions of the standard command.

    /**
     * The list of sensors' command descriptions.
     */
    const SDI12GenSensorCommands *sensors[SDI12_GEN_NB_SENSOR_CMD_DESCS_MAX];
    uint8_t                       nb_sensors;
  }
  SDI12GenMgrSensorsCommandDescriptions;

#ifndef SDI12_MANAGER_NB_INTERFACES_MAX
#error SDI12_MANAGER_NB_INTERFACES_MAX must be defined.
#endif
  /// The list of all the interfaces.
  static SDI12GenMgrInterface sdi12_gen_mgr_interfaces[SDI12_MANAGER_NB_INTERFACES_MAX];

  /// The number of registered interfaces.
  static uint8_t sdi12_gen_mgr_nb_interfaces;

  /// The sensors' command descriptions.
  static SDI12GenMgrSensorsCommandDescriptions sdi12_gen_mgr_sensors_cmds;

  /// Indicate if the manager has been initialised or not.
  static bool sdi12_gen_mgr_has_been_initialised = false;


  static void sdi12_gen_mgr_go_to_idle(       SDI12GenMgrInterface *p_i);
  static bool sdi12_gen_mgr_process_interface(SDI12GenMgrInterface *p_i);

  static void sdi12_gen_mgr_exec_cmd_success(SDI12Command *p_cmd, void *pv_args);
  static void sdi12_gen_mgr_exec_cmd_failed( SDI12Command *p_cmd, void *pv_args);

  static void sdi12_gen_mgr_service_request_received_or_timeout(SDI12Interface *p_interface,
								char            addr,
								void           *pv_args);
  static void sdi12_gen_mgr_received_data_frame(SDI12Command *p_cmd, void *pv_args);
  static void sdi12_gen_mgr_get_next_data_frame(SDI12Command *p_cmd, void *pv_args);



  /**
   * Initialises the SDI-12 interface manager.
   */
  void sdi12_gen_mgr_init(void)
  {
    uint8_t i;

    if(!sdi12_gen_mgr_has_been_initialised)
    {
      // Set the standard commands' descriptions.
      // The number of sensors' command descriptions and the sensors' command descriptions
      // already is set to 0 and NULLs.
      sdi12_gen_mgr_sensors_cmds.p_standard = sdi12_gen_standard_commands_get_descriptions()->commands;


      for(i = 0; i < SDI12_MANAGER_NB_INTERFACES_MAX; i++)
      {
	sdi12_gen_mgr_go_to_idle(&sdi12_gen_mgr_interfaces[i]);
      }

      sdi12_gen_mgr_has_been_initialised = true;
    }
  }


  /**
   * Register a sensor's command descriptions.
   *
   * @param[in] p_sensor_cmds the sensor's command descriptions. MUST be NOT NULL.
   *
   * @return true  if the registration has been successful.
   * @return false if the sensor's name is not set.
   */
  bool sdi12_gen_mgr_register_sensor_command_descriptions(const SDI12GenSensorCommands *p_sensor_cmds)
  {
    uint8_t i;

    // Check the sensor's name
    if(!p_sensor_cmds->ps_sensor_name || !p_sensor_cmds->ps_sensor_name[0])
    {
      log_error(_logger, "Cannot register sensor's command descriptions with no name.");
      return false;
    }

    // See if it already is registered
    for(i = 0; i < sdi12_gen_mgr_sensors_cmds.nb_sensors; i++)
    {
      if(sdi12_gen_mgr_sensors_cmds.sensors[i] == p_sensor_cmds) { return true; } // Already registered
    }

    // Append to the list
    sdi12_gen_mgr_sensors_cmds.sensors[sdi12_gen_mgr_sensors_cmds.nb_sensors++] = p_sensor_cmds;

    return true;
  }

  /**
   * Get the description of a command using it's name and eventually the sensor's type.
   *
   * This function will first search for a command in the sensor's command list.
   * If no description is found, then we'll look in the standard command list.
   *
   * @param[in] ps_cmd_name         the command's name. MUST be NOT NULL.
   * @param[in] ps_sensor_type_name the sensor's type name. Can be NULL if the command is a standard one.
   *
   * @return the description.
   * @return NULL if no description has been found for given name and sensor.
   */
  const SDI12GenCommandDescription *sdi12_gen_mgr_get_description_for_command(
      const char *ps_cmd_name,
      const char *ps_sensor_type_name)
  {
    uint8_t i;
    const SDI12GenCommandDescription *p_cmd_desc;

    if(ps_sensor_type_name)
    {
      for(i = 0; i < sdi12_gen_mgr_sensors_cmds.nb_sensors; i++)
      {
	if(strcasecmp(ps_sensor_type_name, sdi12_gen_mgr_sensors_cmds.sensors[i]->ps_sensor_name) == 0)
	{
	  // Sensor found
	  // Look for command
	  for(p_cmd_desc = sdi12_gen_mgr_sensors_cmds.sensors[i]->commands;
	      p_cmd_desc->ps_cmd_name;
	      p_cmd_desc++)
	  {
	    if(strcasecmp(ps_cmd_name, p_cmd_desc->ps_cmd_name) == 0)
	    {
	      return p_cmd_desc;
	    }
	  }
	}
      }
    }

    // Try the standard commands
    for(p_cmd_desc = sdi12_gen_mgr_sensors_cmds.p_standard; p_cmd_desc->ps_cmd_name; p_cmd_desc++)
    {
      if(strcasecmp(ps_cmd_name, p_cmd_desc->ps_cmd_name) == 0)
      {
	return p_cmd_desc;
      }
    }

    return NULL;
  }


  /**
   * Register a SDI-12 interface.
   *
   * @param[in] ps_name the interface's name. MUST be NOT NULL and NOT EMPTY.
   * @param[in] p_interface the interface object to register. MUST be NOT NULL.
   * @param[in] p_phy       the physical interface used by the SDI-12 interface. MUST be NOT NULL.
   *
   * @return true  if the interface has been registered.
   * @return false if there already is an interface with the given name.
   * @return false if the SDI-12 interface object already is registered.
   * @return false if the physical interface already is registered.
   * @return false if no other interface can be registered because the registration list is full.
   */
  bool sdi12_gen_mgr_register_interface(const char     *ps_name,
					SDI12Interface *p_interface,
					SDI12PHY       *p_phy)
  {
    uint8_t i;
    SDI12GenMgrInterface *p_i;

    // Check that the name, the SDI-12 interface object or the physical interface are not already used.
    for(i = 0; i < SDI12_MANAGER_NB_INTERFACES_MAX; i++)
    {
      p_i = &sdi12_gen_mgr_interfaces[i];
      if(strcmp(ps_name, p_i->ps_name) == 0)
      {
	log_error(_logger, "Cannot register SDI-12 interface; name '%s' already used.", ps_name);
	return false;
      }
      if(p_interface == p_i->p_interface)
      {
	log_error(_logger, "SDI-12 interface object already is registered under the name '%s'.", p_i->ps_name);
	return false;
      }
      if(p_phy == p_i->p_interface->p_phy)
      {
	log_error(_logger, "SDI-12 physical interface already is registered under the name '%s'.", p_i->ps_name);
	return false;
      }
    }

    // Check that the registration list is not full
    if(sdi12_gen_mgr_nb_interfaces >= SDI12_MANAGER_NB_INTERFACES_MAX)
    {
      log_error(_logger, "Cannot register SDI-12 interface; interface list is full.");
      return false;
    }

    // Register
    // Initialise the SDI-12 interface
    sdi12_init(p_interface, p_phy);

    // Look for an empty cell in the registration list
    for(i = 0; i < SDI12_MANAGER_NB_INTERFACES_MAX; i++)
    {
      p_i = &sdi12_gen_mgr_interfaces[i];
      if(!p_i->p_interface) break;  // Found an empty cell.
    }

    // Set up the cell
    p_i->ps_name     = ps_name;
    p_i->p_interface = p_interface;

    // Take into account the newly registered interface
    sdi12_gen_mgr_nb_interfaces++;

    return true;
  }

  /**
   * Create an interface and register it.
   *
   * The interface object is created using memory allocation.
   *
   * @param[in] ps_name the interface's name. MUST be NOT NULL and NOT EMPTY.
   * @param[in] p_phy   the physical interface used by the SDI-12 interface. MUST be NOT NULL.
   *
   * @return true  if the interface has been registered.
   * @return false if there already is an interface with the given name.
   * @return false if the physical interface already is registered.
   * @return false if could not allocate enough memory for the SDI-12 interface.
   */
  bool sdi12_gen_mgr_create_and_register_interface(const char *ps_name, SDI12PHY *p_phy)
  {
    bool  res;
    void *p_i = malloc(sizeof(SDI12Interface));

    if(!p_i)
    {
      log_error(_logger, "Failed to allocate memory to create new SDI-12 interface.");
      return false;
    }

    if(!(res = sdi12_gen_mgr_register_interface(ps_name, (SDI12Interface *)p_i, p_phy)))
    {
      // Free allocated memory
      free(p_i);
    }

    return res;
  }

  /**
   * Get and return the list's cell containing the SDI-12 interface with a given name.
   *
   * @param[in] ps_name the interface's name. Case sensitive. MUST be NOT NULL and NOT EMPTY.
   *
   * @return the list's cell.
   * @return NULL if no interface has been found.
   */
  static SDI12GenMgrInterface *sdi12_gen_mgr_get_interface_list_cell_by_name(const char *ps_name)
  {
    uint8_t i;
    SDI12GenMgrInterface *p_i;

    for(i = 0; i < SDI12_MANAGER_NB_INTERFACES_MAX; i++)
    {
      p_i = &sdi12_gen_mgr_interfaces[i];
      if(strcmp(ps_name, p_i->ps_name) == 0) { return p_i; }
    }

    log_error(_logger, "Cannot find SDI-12 interface with name '%s'.", ps_name);
    return NULL;
  }

  /**
   * Get and return a SDI-12 interface using it's name.
   *
   * @param[in] ps_name the interface's name. Case sensitive. MUST be NOT NULL and NOT EMPTY.
   *
   * @return the interface  with the given name.
   * @return NULL if no interface has been found.
   */
  SDI12Interface *sdi12_gen_mgr_get_interface_by_name(const char *ps_name)
  {
    SDI12GenMgrInterface *p_i = sdi12_gen_mgr_get_interface_list_cell_by_name(ps_name);

    return p_i ? p_i->p_interface : NULL;
  }


  /**
   * Switch an interface to idle state.
   *
   * @param[in] p_i the interface. MUST be NOT NULL.
   */
  static void sdi12_gen_mgr_go_to_idle(SDI12GenMgrInterface *p_i)
  {
    p_i->state = SDI12_GEN_MGR_STATE_IDLE;
  }


  /**
   * Animate the state machine.
   *
   * @return true if there are things left to process.
   * @return false otherwise.
   */
  bool sdi12_gen_mgr_process(void)
  {
    uint8_t i;
    bool    res = false;

    for(i = 0; i < SDI12_MANAGER_NB_INTERFACES_MAX; i++)
    {
      res |= sdi12_gen_mgr_process_interface(&sdi12_gen_mgr_interfaces[i]);
    }

    return res;
  }

  /**
   * Animate the state machine for a given interface.
   *
   * @param[in] p_i the interface. MUST be NOT NULL.
   *
   * @return true if there are things left to process.
   * @return false otherwise.
   */
  static bool sdi12_gen_mgr_process_interface(SDI12GenMgrInterface *p_i)
  {
    bool          res   = false;
    SDI12Command *p_cmd = &p_i->exec_ctx.command;

    res |= sdi12_process(p_i->p_interface);

    switch(p_i->state)
    {
      case SDI12_GEN_MGR_STATE_IDLE:
      case SDI12_GEN_MGR_STATE_EXECUTING_COMMAND:
      case SDI12_GEN_MGR_STATE_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT:
      case SDI12_GEN_MGR_STATE_GETTING_VALUES:
	/* Do nothing */
	break;

      case SDI12_GEN_MGR_STATE_LAST_COMMAND_SUCCESS:
	sdi12_gen_mgr_go_to_idle(p_i);
	if(p_i->exec_ctx.success_cb) { p_i->exec_ctx.success_cb(p_cmd, p_i->exec_ctx.pv_success_cbargs); }
	res = true; // Because the callback may have triggered some more additional work to do.
	break;

      case SDI12_GEN_MGR_STATE_LAST_COMMAND_FAILED:
	sdi12_gen_mgr_go_to_idle(p_i);
	if(p_i->exec_ctx.failed_cb)  { p_i->exec_ctx.failed_cb( p_cmd, p_i->exec_ctx.pv_failed_cbargs);  }
	res = true; // Because the callback may have triggered some more additional work to do.
	break;

      case SDI12_GEN_MGR_STATE_START_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT:
	p_i->state = SDI12_GEN_MGR_STATE_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT;
	if(!sdi12_wait_for_service_request(p_i->p_interface,
					   p_i->exec_ctx.command.address,
					   p_i->exec_ctx.timeout_ms,
					   sdi12_gen_mgr_service_request_received_or_timeout, &p_i->exec_ctx))
	{
	  log_error(_logger, "Failed to wait for service request.");
	  sdi12_gen_mgr_exec_cmd_failed(p_cmd, &p_i->exec_ctx);
	}
	res = true;
	break;

      case SDI12_GEN_MGR_STATE_RECEIVED_SERVICE_REQUEST:
      case SDI12_GEN_MGR_STATE_IT_S_TIME_TO_GET_THE_VALUES:
      case SDI12_GEN_MGR_STATE_GOT_DATA_FRAME:
	// It's time to get the data or to get the next ones.
	sdi12_gen_mgr_get_next_data_frame(p_cmd, &p_i->exec_ctx);
	res = true;
	break;

      default:
	/* We should never get here */
	log_fatal(_logger, "Unknown SDI12 manager state: '%u'.", p_i->state);
	res = true; // Just in case.
	break;
    }

    return res;
  }

  /**
   * Function called when a command has been successful.
   *
   * @param[in] p_cmd the command and it's response. MUST be NOT NULL.
   * @param[in] pv_args the execution context. MUST be NOT NULL.
   *                    Real type is <code>SDI12MgrCmdExecContext</code>.
   */
  static void sdi12_gen_mgr_exec_cmd_success(SDI12Command *p_cmd, void *pv_args)
  {
    bool ok;
    SDI12GenMgrCmdExecContext *p_ctx = (SDI12GenMgrCmdExecContext *)pv_args;

    switch(p_ctx->p_mgr_interface->state)
    {
      case SDI12_GEN_MGR_STATE_EXECUTING_COMMAND:
	if(sdi12_command_cfg_use_send_data_pattern(p_cmd))
	{
	  // Get the timeout value and the number of data to get from the response.
	  // Get the maximum time to wait for the measurements
	  p_ctx->timeout_ms = sdi12_gen_get_cmd_response_value_as_uint(
	      p_cmd, SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_TIMEOUT_SEC, 0, &ok);
	  if(!ok)
	  {
	    log_error(_logger, "Failed to get time to get measurements from response: '%s'.", p_cmd->pu8_buffer);
	    sdi12_gen_mgr_exec_cmd_failed(p_cmd, pv_args);
	    return;
	  }
	  p_ctx->timeout_ms *= 1000; // Because the time we get from the response is in seconds.

	  // Get the number of values to retrieve
	  p_ctx->nb_values_left_to_get = sdi12_gen_get_cmd_response_value_as_uint(
	      p_cmd, SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_NB_VALUES, 0, &ok);
	  if(!ok)
	  {
	    log_error(_logger, "Failed to get the number of values to expect from response: '%s'.", p_cmd->pu8_buffer);
	    sdi12_gen_mgr_exec_cmd_failed(p_cmd, pv_args);
	    return;
	  }
	  p_ctx->next_data_frame = 0;  // Next command increment value will be 0.

	  // Wait for the service request, event if the command does not have this flag set.
	  // Because it also wait for the timeout value.
	  p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_START_WAITING_FOR_SERVICE_REQUEST_OR_TIMEOUT;
	}
	else
	{
	  // The command is a simple command
	  p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_LAST_COMMAND_SUCCESS;
	}
#if !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACK_CMD_SUCCESS
	sdi12_mgr_process_interface(p_ctx->p_mgr_interface);
#endif
	break;

      default:
	log_error(_logger, "We should never be in command success callback when in state '%i'.", p_ctx->p_mgr_interface->state);
	sdi12_gen_mgr_exec_cmd_failed(p_cmd, pv_args);
	break;
    }
  }

  /**
   * Function called when a command failed.
   *
   * @param[in] p_cmd the command and it's response. MUST be NOT NULL.
   * @param[in] pv_args the execution context. MUST be NOT NULL.
   *                    Real type is <code>SDI12MgrCmdExecContext</code>.
   */
  static void sdi12_gen_mgr_exec_cmd_failed(SDI12Command *p_cmd, void *pv_args)
  {
    SDI12GenMgrCmdExecContext *p_ctx = (SDI12GenMgrCmdExecContext *)pv_args;

    p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_LAST_COMMAND_FAILED;
#if !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACK_CMD_FAILED
    sdi12_gen_mgr_process_interface(p_ctx->p_mgr_interface);
#endif
  }

  /**
   * Function called when it's time to retrieve the data from a sensor,
   * because of a service or because enough time has passed.
   *
   * @param[in] p_interface the SDI-12 interface. This parameter is unused.
   * @param[in] addr        the sensor's address.
   *                        the address is '\0' (invalid address) if we have been called because of a timeout.
   *                        This parameter is unused.
   * @param[in] pv_args     the command execution context. Real type is <code>SDI12MgrCmdExecContext</code>.
   *                        MUST be NOT NULL.
   */
  static void sdi12_gen_mgr_service_request_received_or_timeout(SDI12Interface *p_interface,
								char            addr,
								void           *pv_args)
  {
    SDI12GenMgrCmdExecContext *p_ctx = (SDI12GenMgrCmdExecContext *)pv_args;
    SDI12Command              *p_cmd = &p_ctx->command;

    UNUSED(p_interface);
    UNUSED(addr);

    // Set up the data buffer
    p_ctx->pu8_all_data[0] = p_cmd->address;
    p_ctx->all_data_length = 1;
    p_ctx->next_data_frame = 0;

    // Set state
    p_ctx->p_mgr_interface->state =
	(addr == '\0' ?
	    SDI12_GEN_MGR_STATE_IT_S_TIME_TO_GET_THE_VALUES :
	    SDI12_GEN_MGR_STATE_RECEIVED_SERVICE_REQUEST);

#if !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACK_SRVREQ
    sdi12_gen_mgr_process_interface(p_ctx->p_mgr_interface);
#endif
  }

  /**
   * Function called when we have received a data frame response.
   *
   * @param[in] p_cmd   the command and it's response. MUST be NOT NULL.
   * @param[in] pv_args the execution context. MUST be NOT NULL.
   *                   Real type is <code>SDI12MgrCmdExecContext</code>.
   */
  static void sdi12_gen_mgr_received_data_frame(SDI12Command *p_cmd, void *pv_args)
  {
    uint8_t                    len;
    SDI12CmdValuesIterator     it;
    SDI12GenMgrCmdExecContext *p_ctx = (SDI12GenMgrCmdExecContext *)pv_args;

    // Get an iterator on the values
    if(!sdi12_cmd_get_iterator_on_values(p_cmd, &it))
    {
      log_error(_logger, "Cannot get iterator on values from response '%s'.", (const char *)p_cmd->pu8_buffer);
      goto return_error;
    }

    // Get the number of values present in the data frame;
    if(!it.nb_values)
    {
      // Something is wrong
      log_error(_logger, "Received data frame '%s' with no values from sensor '%c' on interface '%s'. Command execution failed.", (const char *)p_cmd->pu8_buffer, p_ctx->command.address, p_ctx->p_mgr_interface->ps_name);
      goto return_error;
    }
    if(it.nb_values > p_ctx->nb_values_left_to_get)
    {
      // We received too many values?
      log_error(_logger, "Received too many values from sensor '%c' on interface '%s'. Command execution failed.", p_ctx->command.address, p_ctx->p_mgr_interface->ps_name);
      goto return_error;
    }

    // Copy values to all data buffer. And if there is more than just value then copy it too.
    if(p_cmd->data_len)
    {
      len = p_cmd->data_len - SDI12_ADDRESS_LEN - SDI12_COMMAND_END_OF_RESPONSE_LEN;
      if(sdi12_command_cfg_response_has_crc(p_cmd)) { len -= SDI12_COMMAND_CRC_LEN; }
      memcpy(&p_ctx->pu8_all_data[p_ctx->all_data_length],
	     &p_cmd->pu8_buffer[SDI12_ADDRESS_LEN],  // Do not copy sensor's address.
	     len);
      p_ctx->all_data_length += len;
    }

    // Are there more data to retrieve from the sensor?
    p_ctx->nb_values_left_to_get -= it.nb_values;
    if(p_ctx->nb_values_left_to_get != 0)
    {
      // More data to retrieve from the sensor.
      // Set the state we're in.
      p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_GOT_DATA_FRAME;
    }
    else
    {
      // No more data to retrieve from the sensor
      // Set the data buffer end
      p_ctx->pu8_all_data[p_ctx->all_data_length++] = '\r';
      p_ctx->pu8_all_data[p_ctx->all_data_length++] = '\n';
      p_ctx->pu8_all_data[p_ctx->all_data_length]   = '\0';  // So that we can use it as a string as well.

      // Change the command buffer to point to the one with all the data
      p_cmd->pu8_buffer  = p_ctx->pu8_all_data;
      p_cmd->data_len    = p_ctx->all_data_length;
      p_cmd->buffer_size = SDI12_MGR_ALL_DATA_BUFFER_SIZE;

      // Set the state we're in.
      p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_LAST_COMMAND_SUCCESS;
    }
#if !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACK_DATA_FRAME_RECEIVED
	sdi12_mgr_process_interface(p_ctx->p_mgr_interface);
#endif
    return;

    return_error:
    sdi12_gen_mgr_exec_cmd_failed(p_cmd, pv_args);
    return;
  }

  /**
   * Function called to get the next data frame.
   *
   * @param[in] p_cmd   the previous command and it's result. NOT USED. MUST be NOT NULL.
   * @param[in] pv_args the execution context. MUST be NOT NULL.
   *                    Real type is <code>SDI12MgrCmdExecContext</code>.
   */
  static void sdi12_gen_mgr_get_next_data_frame(SDI12Command *p_cmd, void *pv_args)
  {
    char                             *ps;
    const SDI12GenCommandDescription *p_cmd_desc;
    SDI12GenMgrCmdExecContext        *p_ctx = (SDI12GenMgrCmdExecContext *)pv_args;

    // Set the state we're in.
    p_ctx->p_mgr_interface->state = SDI12_GEN_MGR_STATE_GETTING_VALUES;

    // Get the command description
    p_cmd_desc = sdi12_gen_mgr_get_description_for_command(p_ctx->ps_next_cmd_name,
							   p_ctx->ps_next_cmd_sensor_type_name);
    if(!p_cmd_desc)
    {
      log_error(_logger, "Cannot send command '%s' command to sensor '%c' on interface '%s'. Cannot find command description.", p_ctx->ps_next_cmd_name, p_cmd->address, p_ctx->p_mgr_interface->ps_name);
      goto return_error;
    }

    // Set the command data buffer
    p_ctx->command.pu8_buffer  = p_ctx->cmd_buffer;
    p_ctx->command.buffer_size = SDI12_GEN_MGR_CMD_BUFFER_SIZE;

    // Initialise the Get Values command.
    ps = strncpy_return_end(p_ctx->ps_string_builder_buffer,
			    SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_INCREMENT,
			    SDI12_GEN_MGR_EXECCTX_STR_BUILDER_BUFFER_SIZE,
			    false);
    *ps++ = SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_KEY_VALUE_PAIR_SEP;
    *ps++ = p_ctx->next_data_frame + '0';
    *ps   = '\0';
    if(!sdi12_gen_command_init_from_description(&p_ctx->command,
						p_cmd_desc,
						p_cmd->address,
						p_ctx->ps_string_builder_buffer,
						sdi12_gen_mgr_received_data_frame, pv_args,
						sdi12_gen_mgr_exec_cmd_failed,     pv_args))
    {
      log_error(_logger, "Cannot send command '%s' command to sensor '%c' on interface '%s'. Initialisation failed.", p_ctx->ps_next_cmd_name, p_cmd->address, p_ctx->p_mgr_interface->ps_name);
      goto return_error;
    }
    p_ctx->next_data_frame++; // Set increment value for next data frame, if we need one.

    // Send the command
    if(!sdi12_send_command(p_ctx->p_mgr_interface->p_interface, p_cmd, SDI12_CMD_TIMEOUT_MS))
    {
      // It means that the interface is busy.
      log_error(_logger, "Failed to send command '%s' with parameters '%s' for sensor '%c' on interface '%s'.", p_ctx->ps_next_cmd_name, p_ctx->ps_string_builder_buffer, p_cmd->address, p_ctx->p_mgr_interface->ps_name);
      goto return_error;
    }
    return;

    return_error:
    sdi12_gen_mgr_exec_cmd_failed(p_cmd, pv_args);
    return;
  }


  /**
   * Send a command to a sensor.
   *
   * @param[in] ps_interface_name   the name of the SDI-12 interface to use.
   *                                MUST be NOT NULL and NOT EMPTY.
   * @param[in] ps_cmd_name         the name of the command to send. MUST be NOT NULL.
   * @param[in] ps_sensor_type_name the name of the sensor's type.
   *                                Can be NULL or empty.
   *                                Only used in case there are several commands with the same name.
   *                                If you do not provide it,
   *                                and there are several commands with the same name registered,
   *                                then this function will fail (return false).
   * @param[in] addr                the address to send the command to. MUST be a valid address.
   * @param[in] ps_cmd_var_values   the string containing the list of key-value pairs used to
   *                                replace the variables in the command description with actual values.
   *                                Can only be NULL or empty if there is no variable to replace
   *                                in the command string.
   * @param[in] success_cb          the function to call on a successful communication. Can be NULL.
   * @param[in] pv_success_cbargs   the argument to pass to the success callback. Can be NULL.
   * @param[in] failed_cb           the function to call if the communication failed. Can be NULL.
   * @param[in] pv_failed_cbargs    the argument to pass to the the failure callback. Can be NULL.
   */
  bool sdi12_gen_mgr_send_command(const char               *ps_interface_name,
				  const char               *ps_cmd_name,
				  const char               *ps_sensor_type_name,
				  char                      addr,
				  const char               *ps_cmd_var_values,
				  sdi12_command_result_cb_t success_cb,
				  void                     *pv_success_cbargs,
				  sdi12_command_result_cb_t failed_cb,
				  void                     *pv_failed_cbargs)
  {
    const SDI12GenCommandDescription *p_cmd_desc;
    SDI12GenMgrInterface             *p_i;

    // Get the command description
    p_cmd_desc = sdi12_gen_mgr_get_description_for_command(ps_cmd_name, ps_sensor_type_name);
    if(!p_cmd_desc)
    {
      log_error(_logger, "Cannot send '%s' command to sensor '%c' on interface '%s'. Cannot find command description.", ps_cmd_name, addr, ps_interface_name);
      return false;
    }

    // Check interface
    p_i = sdi12_gen_mgr_get_interface_list_cell_by_name(ps_interface_name);
    if(!p_i)
    {
      log_error(_logger, "Cannot send '%s' command to sensor '%c' on interface '%s'. Interface not found.", ps_cmd_name, addr, ps_interface_name);
      return false;
    }
#if defined SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS && SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS > 0
    // FIXME: See why we seem to be needing this to call consecutive commands.
    // Adding  a delay in the low SDI-12 layer with SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS changes nothing,
    // So it appears that problem is related to the manager because introducing the delay here makes
    // it possible to send several commands one after another, or at least spaced by the delay fixed here.
    HAL_Delay(SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS);
#endif
    if(p_i->state != SDI12_GEN_MGR_STATE_IDLE)
    {
      log_error(_logger, "Cannot send '%s' command to sensor '%c' on interface '%s'. We are busy.", ps_cmd_name, addr, ps_interface_name);
      return false;
    }
    p_i->state = SDI12_GEN_MGR_STATE_EXECUTING_COMMAND;

    // Set up the command execution context
    p_i->exec_ctx.p_mgr_interface              = p_i;
    p_i->exec_ctx.nb_values_left_to_get        = 0;
    p_i->exec_ctx.next_data_frame              = 0;
    p_i->exec_ctx.all_data_length              = 0;
    p_i->exec_ctx.timeout_ms                   = 0;
    p_i->exec_ctx.ps_next_cmd_name             = p_cmd_desc->ps_next_cmd_name;
    p_i->exec_ctx.ps_next_cmd_sensor_type_name = p_cmd_desc->ps_next_command_sensor_type_name;
    p_i->exec_ctx.success_cb                   = success_cb;
    p_i->exec_ctx.pv_success_cbargs            = pv_success_cbargs;
    p_i->exec_ctx.failed_cb                    = failed_cb;
    p_i->exec_ctx.pv_failed_cbargs             = pv_failed_cbargs;

    // Set the command data buffer
    p_i->exec_ctx.command.pu8_buffer  = p_i->exec_ctx.cmd_buffer;
    p_i->exec_ctx.command.buffer_size = SDI12_GEN_MGR_CMD_BUFFER_SIZE;

    // Initialises the command using the description and the variables' values.
    if(!sdi12_gen_command_init_from_description(&p_i->exec_ctx.command,
						p_cmd_desc,
						addr,
						ps_cmd_var_values,
						sdi12_gen_mgr_exec_cmd_success, &p_i->exec_ctx,
						sdi12_gen_mgr_exec_cmd_failed,  &p_i->exec_ctx))
    {
      log_error(_logger, "Cannot send '%s' command to sensor '%c' on interface '%s'. Command initialisation failed.", ps_cmd_name, addr, ps_interface_name);
      sdi12_gen_mgr_go_to_idle(p_i);
      return false;
    }

    // Send the command
    if(!sdi12_send_command(p_i->p_interface, &p_i->exec_ctx.command, SDI12_CMD_TIMEOUT_MS)) \
    {
      log_error(_logger, "Cannot send '%s' command to sensor '%c' on interface '%s'. Interface is busy.", ps_cmd_name, addr, ps_interface_name);
      sdi12_gen_mgr_go_to_idle(p_i);
      return false;
    }

    return true;
  }


#ifdef __cplusplus
}
#endif
