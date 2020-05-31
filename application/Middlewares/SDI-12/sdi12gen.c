/**
 * @file  sdi12_generic.c
 * @brief 
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "sdi12gen.h"
#include "sdi12.h"
#include "logger.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


  CREATE_LOGGER(sdi12_generic);
#undef  _logger
#define _logger  sdi12_generic

#define CMD_VAR_BEGIN_CHAR          '$'
#define CMD_VAR_PARAMS_BEGIN        '{'
#define CMD_VAR_PARAMS_END          '}'
#define CMD_VAR_PARAMS_SEP          ','

#define CMD_VAR_PARAM_KEY_VALUE_SEP ':'

#define CMD_VAR_PARAM_KEY_NAME      'n'
#define CMD_VAR_PARAM_KEY_TYPE      't'
#define CMD_VAR_PARAM_KEY_LENGTH    'l'
#define CMD_VAR_PARAM_KEY_RANGE     'r'

#define CMD_VAR_PARAM_VALUE_TYPE_CHAR    SDI12_GEN_CMDDESC_VAR_TYPE_CHAR
#define CMD_VAR_PARAM_VALUE_TYPE_INT     SDI12_GEN_CMDDESC_VAR_TYPE_INT
#define CMD_VAR_PARAM_VALUE_TYPE_STRING  SDI12_GEN_CMDDESC_VAR_TYPE_STRING

#define CMD_VAR_PARAM_RANGE_BEGIN    '['
#define CMD_VAR_PARAM_RANGE_END      ']'
#define CMD_VAR_PARAM_RANGE_SEP      '.'
#define CMD_VAR_PARAM_RANGE_ITEM_SEP '|'

#define CMD_VALUES_STRING_PADDING ' '

#define CMD_DESC_CRC_PATTERN "<CRC>"


  /**
   * Defines the error values that can be set by the generic initialiser.
   */
/*  typedef enum SDI12CommandGenError
  {
  }
  SDI12CommandGenError;
 */


  static bool sdi12_gen_get_var_field_len_from_response_and_desc(uint8_t                *pu8_data,
								 uint16_t                data_len,
								 const char             *p_var_params,
								 uint16_t                var_params_len,
								 uint16_t               *p_len_min,
								 uint16_t               *p_len_max,
								 SDI12GenCmdDescVarType *p_type);

  static bool sdi12_gen_copy_cmd_string_and_replace_variables(uint8_t    *pu8_buffer,
  							      uint16_t    buffer_size,
  							      const char *ps_cmd,
  							      const char *ps_var_values,
  							      uint16_t   *pu16_len);
  static bool sdi12_gen_replace_var_in_cmd_string(       uint8_t   **ppu8_buffer,
						         uint16_t    buffer_size,
						         char      **pps_cmd,
						         const char *ps_var_values);
  static bool sdi12_gen_replace_char_var_in_cmd_string(  uint8_t **ppu8_buffer,
  						         uint16_t  buffer_size,
  						         char     *ps_cmd,
  						         char     *ps_value,
  						         uint16_t  value_len);
  static bool sdi12_gen_replace_string_var_in_cmd_string(uint8_t **ppu8_buffer,
							 uint16_t  buffer_size,
							 char     *ps_cmd,
							 char     *ps_value,
							 uint16_t  value_len);
  static bool sdi12_gen_replace_int_var_in_cmd_string(   uint8_t **ppu8_buffer,
						         uint16_t  buffer_size,
						         char     *ps_cmd,
						         char     *ps_value,
						         uint16_t  value_len);

  static char *sdi12_gen_get_cmdstr_var_param_value_using_key(char     *ps_var_str,
							      char      key,
							      uint16_t *pu16_len);
  static const char *sdi12_gen_get_value_from_var_params_using_key(const char *p_params,
								   uint16_t    params_len,
								   char        key,
								   uint16_t   *pu16_len);
  static bool sdi12_gen_get_field_length_from_var_params(const char *p_params,
							 uint16_t    params_len,
							 uint16_t   *pu16_min,
							 uint16_t   *pu16_max);
  static char *sdi12_gen_get_value_from_values_list(const char *ps_var_values,
  						    const char *ps_key,
  						    uint16_t    key_len,
  						    uint16_t   *pu16_len);

  static const char *sdi12_gen_get_cmdstr_next_var(const char **pps_cmdstr,
						   bool         only_get_params,
						   uint16_t    *pu16_len,
						   uint16_t    *pu16_prefix_len);
  /*static const char *sdi12_gen_get_cmdstr_var_params_using_var_name(const char *ps_cmdstr,
								    const char *ps_var_name,
								    uint16_t   *pu16_len);
*/



  /**
   * Initialise a command object using a command's description.
   *
   * @pre  The command object MUST have it's buffer configuration set
   *       (<code>pu8_buffer</code> and <code>buffer_size</code> set).
   *
   * @post the <code>pv_arg</code> command member is set to the command description <code>p_desc</code>.
   *
   *
   * @param[in,out] p_cmd             the command object to initialise. MUST be NOT NULL.
   * @param[in]     p_desc            the description to use. MUST be NOT NULL and valid.
   * @param[in]     addr              the address of the sensor to communicate with.
   * @param[in]     ps_var_values     the command's variables' values.
   *                                  Can be NULL or EMPTY if the command has no variable.
   *                                  The string is a comma list separated of key-value pairs,
   *                                  where the key is the variable name and the value is the
   *                                  variable's value, both are separated by a colon.
   *                                  Example: <code>"var1:v1_int,var_2:v2_string,var3:v3_char"</code>.
   *                                  If the variable's value is smaller than ir's expected length
   *                                  then is is padded, accordingly with the variable's type.
   *                                  If it is longer than the expected length,
   *                                  then the initialisation fails.
   * @param[in]     success_cb        the callback function called when the command has
   *                                  successfully been executed.
   *                                  Can be NULL.
   * @param[in]     pv_success_cbargs the arguments passed to the callback function <code>success_cb</code>.
   *                                  Can be NULL.
   * @param[in]     failed_cb         the callback function called when the command has failed.
   *                                  Can be NULL.
   * @param[in]     pv_failed_cbargs  the arguments passed to the callback function <code>failed_cb</code>.
   *                                  Can be NULL.
   *
   * @return true  on success.
   * @return false otherwise. The <code>p_cmd->error</code> is also set with an error code.
   */
  bool sdi12_gen_command_init_from_description(SDI12Command                      *p_cmd,
					       const SDI12GenCommandDescription  *p_desc,
					       char                               addr,
					       const char                        *ps_var_values,
					       sdi12_command_result_cb_t          success_cb,
					       void                              *pv_success_cbargs,
					       sdi12_command_result_cb_t          failed_cb,
					       void                              *pv_failed_cbargs)
  {
    // Set default values
    p_cmd->error = SDI12_COMMAND_ERROR_NONE;

    // Set the command configuration
    p_cmd->config = p_desc->config;
    // See if the addresses in the command and the response have to be the same.
    if(p_desc->ps_cmd[0] == p_desc->ps_response[0]) { p_cmd->config |=  SDI12_CMD_CONFIG_SAME_ADDRESSES; }
    else                                            { p_cmd->config &= ~SDI12_CMD_CONFIG_SAME_ADDRESSES; }
    // Set the CRC config
    if(strstr(p_desc->ps_response, CMD_DESC_CRC_PATTERN)) { p_cmd->config |=  SDI12_CMD_CONFIG_RESPONSE_HAS_CRC; }
    else                                                  { p_cmd->config &= ~SDI12_CMD_CONFIG_RESPONSE_HAS_CRC; }

    // Make sure that if a Send Data communication pattern is set then we have a next command set up.
    if(sdi12_command_cfg_use_send_data_pattern(p_cmd) && !p_desc->ps_next_cmd_name){
      log_error(_logger, "Cannot initialise command %s; a Send Command pattern is expected but no 'next command' is set in the command's description.", p_desc->ps_full_name);
      return false;
    }

    // Set the sensor's address.
    if(p_desc->ps_cmd[0] == '?') { addr = '?'; }
    if(!sdi12_is_address_valid(addr))
    {
      log_error(_logger, "Cannot initialise %s command using invalid address '%c'.", p_desc->ps_full_name, addr);
      p_cmd->error = SDI12_COMMAND_ERROR_INVALID_ADDRESS;
      return false;
    }
    p_cmd->address       = addr;
    p_cmd->pu8_buffer[0] = addr;

    // Copy and substitute variables with values.
    if(!sdi12_gen_copy_cmd_string_and_replace_variables(&p_cmd ->pu8_buffer[SDI12_ADDRESS_LEN],
							 p_cmd ->buffer_size - SDI12_ADDRESS_LEN,
							&p_desc->ps_cmd[SDI12_ADDRESS_LEN],
							ps_var_values,
							&p_cmd->data_len))
    {
      p_cmd->error = SDI12_COMMAND_ERROR_CMD_INITIALISATION_FAILED;
      return false;
    }
    p_cmd->data_len += SDI12_ADDRESS_LEN;      // To take into account the address that we wrote first.
    p_cmd->pu8_buffer[p_cmd->data_len] = '\0'; // So that we can use the buffer as a string as well.

    // Set other parameters
    p_cmd->pu8_cmd                = p_cmd->pu8_buffer;
    p_cmd->pv_arg                 = (void *)p_desc;
    p_cmd->process_response       = p_desc->process_response;
    p_cmd->signal_success         = success_cb;
    p_cmd->pv_signal_success_args = pv_success_cbargs;
    p_cmd->signal_failed          = failed_cb;
    p_cmd->pv_signal_failed_args  = pv_failed_cbargs;

    log_debug(_logger, "Command %s for sensor with address '%c' has been initialised from command description.", p_desc->ps_full_name, addr);
    return true;
  }

  /**
   * Get a value from a command response using the variable's name.
   *
   * @param[in]  p_cmd          the command, with it's response. MUST be NOT NULL.
   *                            MUST have been initialised and MUST contains a response.
   * @param[in]  ps_var_name    the name of the variable whose value we want to get. MUST be NOT NULL.
   *                            If empty then this function will fail.
   * @param[out] pu16_value_len where the value's length is written to. MUST be NOT NULL.
   *                            Set to 0 in case of error.
   * @param[out] p_type         where the value's type is written to.
   *                            Can be NULL if we  are not interested in this information.
   *                            Set to SDI12_GEN_CMDDESC_VAR_TYPE_UNDEFINED if the function fails.
   *
   * @return the variable's value. What is returned is not a '\0' terminated string;
   *         and it's length is <code>*pu16_value_len</code>.
   * @return NULL if no variable with the given name has been found.
   */
  const char *sdi12_gen_get_cmd_response_value(const SDI12Command     *p_cmd,
					       const char             *ps_var_name,
					       uint16_t               *pu16_value_len,
					       SDI12GenCmdDescVarType *p_type)
  {
    const SDI12GenCommandDescription *p_cmd_desc;
    const char *ps_resp_desc, *p_params, *p_v;
    uint16_t    params_len, vlen, prefix_len, len_min, len_max, nb_chars_left;
    uint8_t    *pu8_value, *pu8_end;
    SDI12GenCmdDescVarType value_type = SDI12_GEN_CMDDESC_VAR_TYPE_UNDEFINED;

    // First get the command description
    if(!p_cmd->pv_arg)
    {
      log_error(_logger, "Command '%s' has no description set.", p_cmd->pu8_cmd);
      goto return_error;
    }
    p_cmd_desc = (const SDI12GenCommandDescription  *)p_cmd->pv_arg;

    // Then go through the response in search for the variable with the given name,
    // and keep track of where to find the value in the given response.
    ps_resp_desc = p_cmd_desc->ps_response;
    pu8_value    = p_cmd->pu8_buffer;
    pu8_end      = p_cmd->pu8_buffer + p_cmd->data_len - SDI12_COMMAND_END_OF_RESPONSE_LEN;
    if(sdi12_command_cfg_response_has_crc(p_cmd)) { pu8_end -= SDI12_COMMAND_CRC_LEN; }
    while((p_params = sdi12_gen_get_cmdstr_next_var(&ps_resp_desc, true, &params_len, &prefix_len)))
    {
      // Get the field length
      nb_chars_left = (uint16_t)(pu8_end - pu8_value);
      if(!sdi12_gen_get_var_field_len_from_response_and_desc(pu8_value, nb_chars_left,
							     p_params,  params_len,
							     &len_min,  &len_max,
							     &value_type))
      {
	log_error(_logger, "Failed to get field length, or deduce the length, from variable parameters: %.*s.", params_len, p_params);
	goto return_error;
      }
      if(nb_chars_left < len_min)
      {
	log_error(_logger, "Not enough data in response '%s' for variable '%s'.", p_cmd->pu8_buffer, ps_var_name);
	goto return_error;
      }

      // Check if the variable is the one we are looking for
      p_v = sdi12_gen_get_value_from_var_params_using_key(p_params, params_len,
							  CMD_VAR_PARAM_KEY_NAME ,
							  &vlen);
      if(p_v && strncmp(ps_var_name, p_v, vlen) == 0)
      {
	// We found the variable with the name we are looking for
	// Update the pointer positions
	pu8_value     += prefix_len;
	nb_chars_left -= prefix_len;
	if(p_type) { *p_type = value_type; }
	break;
      }
      // Else move in the reception buffer of the current variable's length
      if(prefix_len >= nb_chars_left)
      {
	log_error(_logger, "The response '%s' has less data that what is expected from the description '%s'.", p_cmd->pu8_buffer, p_cmd_desc->ps_response);
	goto return_error;
      }
      pu8_value     += prefix_len;
      nb_chars_left -= prefix_len;
      if(len_min == len_max) { pu8_value += len_min;                                           }
      else                   { pu8_value += len_max < nb_chars_left ? len_max : nb_chars_left; }
    }
    if(!p_params)
    {
      log_info(_logger, "There is no variable named '%s' in command response description '%s'.", ps_var_name, p_cmd_desc->ps_response);
      goto return_error;
    }

    // We have found our variable, get the value from the response.
    if(len_min == len_max) { *pu16_value_len = len_min;                                           }
    else                   { *pu16_value_len = len_max < nb_chars_left ? len_max : nb_chars_left; }
    return (const char *)pu8_value;

    return_error:
    if(pu16_value_len) { *pu16_value_len = 0;          }
    if(p_type)         { *p_type         = value_type; }
    return NULL;
  }

  /**
   * Get a character value from a command response using the variable's name.
   *
   * @param[in]  p_cmd          the command, with it's response. MUST be NOT NULL.
   *                            MUST have been initialised and MUST contains a response.
   * @param[in]  ps_var_name    the name of the variable whose value we want to get. MUST be NOT NULL.
   *                            If empty then this function will fail.
   * @param[in]  default_value  the value to return in case this function fails.
   * @param[out] pb_ok          Set to true on success; set to false otherwise.
   *                            Can be NULL if we are not interested in this information.
   *
   * @return the character value.
   * @return the <code>default_value</code> if no such value could be found.
   */
  char sdi12_gen_get_cmd_response_value_as_char(const SDI12Command *p_cmd,
						const char         *ps_var_name,
						char                default_value,
						bool               *pb_ok)
  {
    const char            *p_v;
    uint16_t               vlen;
    SDI12GenCmdDescVarType type;

    if(!(p_v = sdi12_gen_get_cmd_response_value(p_cmd, ps_var_name, &vlen, &type)))
    {
      goto return_error;
    }
    if(type != SDI12_GEN_CMDDESC_VAR_TYPE_CHAR)
    {
      log_error(_logger, "We were expecting variable '%s' to be of type character but it is of type '%c'.", ps_var_name, type);
      goto return_error;
    }

    if(pb_ok) { *pb_ok = true;  }
    return p_v[0];

    return_error:
    if(pb_ok) { *pb_ok = false; }
    return default_value;
  }

  /**
   * Get an integer value from a command response using the variable's name.
   *
   * @param[in]  p_cmd          the command, with it's response. MUST be NOT NULL.
   *                            MUST have been initialised and MUST contains a response.
   * @param[in]  ps_var_name    the name of the variable whose value we want to get. MUST be NOT NULL.
   *                            If empty then this function will fail.
   * @param[in]  default_value  the value to return in case this function fails.
   * @param[out] pb_ok          Set to true on success; set to false otherwise.
   *                            Can be NULL if we are not interested in this information.
   *
   * @return the integer value.
   * @return the <code>default_value</code> if no such value could be found.
   */
  int32_t sdi12_gen_get_cmd_response_value_as_int(const SDI12Command *p_cmd,
						  const char         *ps_var_name,
						  int32_t             default_value,
						  bool               *pb_ok)
  {
    const char            *p_v;
    uint16_t               vlen;
    int32_t                value;
    SDI12GenCmdDescVarType type;

    if(!(p_v = sdi12_gen_get_cmd_response_value(p_cmd, ps_var_name, &vlen, &type)))
    {
      goto return_error;
    }
    if(type != SDI12_GEN_CMDDESC_VAR_TYPE_INT)
    {
      log_error(_logger, "We were expecting variable '%s' to be of type integer but it is of type '%c'.", ps_var_name, type);
      goto return_error;
    }
    value = strn_string_to_int(p_v, vlen, pb_ok);
    if(!pb_ok)
    {
      log_error(_logger, "Failed to convert variable '%s''s value '%.*s' to integer.", ps_var_name, vlen, p_v);
      goto return_error;
    }

    if(pb_ok) { *pb_ok = true;  }
    return value;

    return_error:
    if(pb_ok) { *pb_ok = false; }
    return default_value;
  }

  /**
   * Get an unsigned integer value from a command response using the variable's name.
   *
   * @param[in]  p_cmd          the command, with it's response. MUST be NOT NULL.
   *                            MUST have been initialised and MUST contains a response.
   * @param[in]  ps_var_name    the name of the variable whose value we want to get. MUST be NOT NULL.
   *                            If empty then this function will fail.
   * @param[in]  default_value  the value to return in case this function fails.
   * @param[out] pb_ok          Set to true on success; set to false otherwise.
   *                            Can be NULL if we are not interested in this information.
   *
   * @return the unsigned integer value.
   * @return the <code>default_value</code> if no such value could be found.
   */
  uint32_t sdi12_gen_get_cmd_response_value_as_uint(const SDI12Command *p_cmd,
						    const char         *ps_var_name,
						    uint32_t            default_value,
						    bool               *pb_ok)
  {
    const char            *p_v;
    uint16_t               vlen;
    uint32_t               value;
    SDI12GenCmdDescVarType type;

    if(!(p_v = sdi12_gen_get_cmd_response_value(p_cmd, ps_var_name, &vlen, &type)))
    {
      goto return_error;
    }
    if(type != SDI12_GEN_CMDDESC_VAR_TYPE_INT)
    {
      log_error(_logger, "We were expecting variable '%s' to be of type integer but it is of type '%c'.", ps_var_name, type);
      goto return_error;
    }
    value = strn_string_to_uint(p_v, vlen, pb_ok);
    if(!pb_ok)
    {
      log_error(_logger, "Failed to convert variable '%s''s value '%.*s' to unsigned integer.", ps_var_name, vlen, p_v);
      goto return_error;
    }

    if(pb_ok) { *pb_ok = true;  }
    return value;

    return_error:
    if(pb_ok) { *pb_ok = false; }
    return default_value;
  }

  /**
   * Get a string value from a command response using the variable's name.
   *
   * @param[in]  p_cmd          the command, with it's response. MUST be NOT NULL.
   *                            MUST have been initialised and MUST contains a response.
   * @param[in]  ps_var_name    the name of the variable whose value we want to get. MUST be NOT NULL.
   *                            If empty then this function will fail.
   * @param[out] ps_buffer      the buffer where the string is written to. MUST be NOT NULL.
   *                            We copy the value, we do not return a pointer to the reception buffer.
   *                            The value written to the buffer is a string; it is \0 terminated.
   * @param[in]  buffer_size    the buffer's size.
   * @param[in]  default_value  the value to return in case this function fails.
   *                            The default value is also copied to the buffer.
   *                            The default_value pointer is returned.
   *                            So comparing the addresses of <code>default_value</code>
   *                            and the value returned by the function is enough to detect that
   *                            the default value has been used and returned.
   * @param[out] pb_ok          Set to true on success; set to false otherwise.
   *                            Can be NULL if we are not interested in this information.
   *
   * @return the unsigned integer value.
   * @return the <code>default_value</code> if no such value could be found.
   */
  char *sdi12_gen_get_cmd_response_value_as_string(const SDI12Command *p_cmd,
						   const char         *ps_var_name,
						   char               *ps_buffer,
						   uint16_t            buffer_size,
						   const char         *default_value,
						   bool               *pb_ok)
  {
    const char            *p_v;
    uint16_t               vlen;
    SDI12GenCmdDescVarType type;

    if(!(p_v = sdi12_gen_get_cmd_response_value(p_cmd, ps_var_name, &vlen, &type)))
    {
      goto return_error;
    }
    if(vlen >= buffer_size)
    {
      log_error(_logger, "Cannot copy value of variable '%s' to buffer; value is too long: value's length is %d while buffer's length is %d.", ps_var_name, vlen, buffer_size - 1);
      goto return_error;
    }

    // Copy to buffer
    strncpy(ps_buffer, p_v, vlen);
    ps_buffer[vlen] = '\0';
    if(pb_ok) { *pb_ok = true; }
    return ps_buffer;

    return_error:
    strncpy(ps_buffer, default_value, buffer_size); ps_buffer[buffer_size - 1] = '\0';
    if(pb_ok) { *pb_ok = false; }
    return (char *)default_value;
  }

  /**
   * Get, or try to guess, a response field length.
   *
   * We use the variable description's parameters to do so.
   * If there is a length parameter then use it.
   * If not then try to guess it from the other parameters.
   *
   * @param[in] pu8_data the data from the response. MUST be NOT NULL.
   *                     and must point to the first character that could belong to the variable.
   * @param[in] data_len the number of response data bytes.
   * @param[in] p_var_params the variable's parameters. MUST be NOT NULL.
   * @param[in] var_params_len the length of the variable's parameters string.
   * @param[out] p_len_min      where the field's minimum length will be written to.
   *                            MUST be NOT NULL.
   *                            Set to 0 if the function fails.
   * @param[out] p_len_max      where the field's maximum length will be written to.
   *                            MUST be NOT NULL.
   *                            Set to 0 if the function fails.
   * @param[out] p_type         Where the variable's type is written to.
   *                            Can be NULL if we are not interested in this information.
   *                            In case of error is set to SDI12_GEN_CMDDESC_VAR_TYPE_UNDEFINED.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdi12_gen_get_var_field_len_from_response_and_desc(uint8_t                *pu8_data,
								 uint16_t                data_len,
								 const char             *p_var_params,
								 uint16_t                var_params_len,
								 uint16_t               *p_len_min,
								 uint16_t               *p_len_max,
								 SDI12GenCmdDescVarType *p_type)
  {
    const char            *p_v, *p_s;
    uint16_t               vlen, range_value_len;
    SDI12GenCmdDescVarType type = SDI12_GEN_CMDDESC_VAR_TYPE_UNDEFINED;

    // Get the variable's type.
    p_v = sdi12_gen_get_value_from_var_params_using_key(p_var_params, var_params_len,
							CMD_VAR_PARAM_KEY_TYPE,
							&vlen);
    if(!p_v || vlen != 1)
    {
      log_error(_logger, "Failed to get variable's type from parameters '%.*s'.", var_params_len, p_var_params);
      goto return_error;
    }
    type = p_v[0];
    if(p_type) { *p_type = type; }

    // If we have a length parameter then we're done.
    if(sdi12_gen_get_field_length_from_var_params(p_var_params, var_params_len, p_len_min, p_len_max))
    {
      // We found the length parameter
      return true;
    }

    // No length parameter. So try to guess it.
    // See if we have a character variable
    if(type == CMD_VAR_PARAM_VALUE_TYPE_CHAR)
    {
      // Character variables always have a length of 1.
      *p_len_min = *p_len_max = 1;
      return true;
    }

    // See if we have a string variable
    if(type == CMD_VAR_PARAM_VALUE_TYPE_STRING)
    {
      // With string we can only try to guess the length if we have a range of values.
      if(!(p_v = sdi12_gen_get_value_from_var_params_using_key(p_var_params, var_params_len,
							       CMD_VAR_PARAM_KEY_RANGE,
							       &vlen)))
      {
	// No range, so we cannot guess the length
	goto return_error;
      }

      // We have a range so try to find a matching string
      if(*p_v == CMD_VAR_PARAM_RANGE_BEGIN)
      {
	if(p_v[vlen - 1] != CMD_VAR_PARAM_RANGE_END)
	{
	  log_error(_logger, "Invalid range specification: '%.*s'.", vlen, p_v);
	  goto return_error;
	}
	p_v++;
	vlen -= 2;
      }

      // Try to find a range value matching the beginning of the data.
      while(vlen)
      {
	for(p_s = p_v; vlen && *p_v != CMD_VAR_PARAM_RANGE_ITEM_SEP; p_v++, vlen--) { /* Do nothing */ }
	range_value_len = (uint16_t)(p_v - p_s);
	if(strncmp((const char *)pu8_data,
		   p_s,
		   range_value_len < data_len ? range_value_len : data_len) == 0)
	{
	  // We found a matching range value
	  *p_len_min = *p_len_max = range_value_len;
	  return true;
	}
	if(vlen) { p_v++; vlen--;     } // To jump over the range separator character
	else     { goto return_error; }
      }
    }
    // The type variable is not one we can guess the length of.

    return_error:
    if(p_type) { *p_type = type; }
    *p_len_min = *p_len_max = 0;
    return false;
  }


  /**
   * Copy a command string to a buffer and replace the variables from the command string
   * with actual variable values.
   *
   * @param[out] pu8_buffer    the buffer to write to. MUST be NOT NULL.
   * @param[in]  buffer_size   the buffer's size.
   * @param[in]  ps_cmd        the command string to copy and transform. MUST be NOT NULL and NOT EMPTY.
   * @param[in]  ps_var_values the command's variables' values.
   *                           Can be NULL or EMPTY if the command has no variable.
   *                           The string is a comma list separated of key-value pairs,
   *                           where the key is the variable name end the value is the
   *                           variable's value, both are separated by a colon.
   *                           Example: <code>"var1:v1_int,var_2:v2_string,var3:v3_char"</code>.
   *                           If the variable's value is smaller than ir's expected length
   *                           then is is padded, accordingly with the variable's type.
   *                           If it is longer than the expected length,
   *                           then the function fails.
   * @param[out] pu16_len      where the length of the resulting string is written to.
   *                           Can be NULL if we do not want this information.
   *                           Set to 0 in case of error.
   *
   * @return true  on success.
   * @return false if the buffer is too small.
   * @return false if not all variables from the command string could be replaced
   *               because of a missing value.
   * @return false if a variable's value number of characters is too big.
   * @return false if a variable as a value range set and the specified value is not in that range.
   */
  static bool sdi12_gen_copy_cmd_string_and_replace_variables(uint8_t    *pu8_buffer,
							      uint16_t    buffer_size,
							      const char *ps_cmd,
							      const char *ps_var_values,
							      uint16_t   *pu16_len)
  {
    uint8_t *pc      = (uint8_t *)ps_cmd;
    uint8_t *pu8     = pu8_buffer;
    uint8_t *pu8_end = pu8 + buffer_size;

    while(*pc && pu8 < pu8_end)
    {
      if(*pc == CMD_VAR_BEGIN_CHAR && *(pc + 1) == CMD_VAR_PARAMS_BEGIN)
      {
	// Make sure that we have values to replace the variable with
	if(str_is_null_or_empty(ps_var_values))
	{
	  log_error(_logger, "Cannot replace variables in command string '%s'; we have no values to replace them with.", ps_cmd);
	  return false;
	}

	// Replace variable.
	if(!sdi12_gen_replace_var_in_cmd_string(&pu8,
						buffer_size - (uint16_t)(pu8 - pu8_buffer),
						(char **)&pc,
						ps_var_values))
	{
	  log_error(_logger, "Failed to replace variables in command string '%s' using values '%s'.", ps_cmd, ps_var_values ? ps_var_values : "NULL");
	  return false;
	}
      }
      else
      {
	// Just copy the character
	*pu8++ = *pc++;
      }
    }
    if(pu8 >= pu8_end)
    {
      // The buffer is too small
      log_error(_logger, "Failed to replace variables in command string '%s' using values '%s'. Buffer of size %u is too small.", ps_cmd, ps_var_values ? ps_var_values : "NULL", buffer_size);
      if(pu16_len) { *pu16_len = 0; }
      return false;
    }

    // Add a ending '\0' character so that we can use the command as a string
    *pu8 = '\0';

    if(pu16_len) { *pu16_len = (uint16_t)(pu8 - pu8_buffer); }
    return true;
  }


  /**
   * Replace a variable with it's value in a command string.
   *
   * @param[out] ppu8_buffer   a pointer to the pointer on a buffer. MUST be NOT NULL.
   *                           This double pointer allowed us to move an outside pointer on the buffer.
   * @param[in]  buffer_size   the number of bytes left in the buffer.
   * @param[in]  pps_cmd       part of the command string. MUST be NOT NULL.
   *                           <code>*pps_cmd</code> points to the beginning of the variable.
   *                           Here again, we use a double pointer to that we can move the exterior
   *                           pointer.
   * @param[in]  ps_var_values the list of the variables and their values. MUST be NOT NULL.
   *
   * @return true  if the variable has been replaced.
   *               And *ppu8_buffer points to the next character to write in the buffer,
   *               and *pps_cmd points to the next character after the variable in the command string.
   * @return false otherwise.
   */
  static bool sdi12_gen_replace_var_in_cmd_string(uint8_t   **ppu8_buffer,
						  uint16_t    buffer_size,
						  char      **pps_cmd,
						  const char *ps_var_values)
  {
    char    *p_name,  *p_value,  *p_type;
    uint16_t name_len, value_len, type_len;
    char    *p_c = *pps_cmd;
    bool     res = false;

    if(*p_c == CMD_VAR_BEGIN_CHAR)   { p_c++; }
    if(*p_c == CMD_VAR_PARAMS_BEGIN) { p_c++; }

    // First get the variable's name.
    p_name = sdi12_gen_get_cmdstr_var_param_value_using_key(p_c, CMD_VAR_PARAM_KEY_NAME, &name_len);
    if(!p_name)
    {
      log_error(_logger, "Cannot get name parameter from '%s'.", *pps_cmd);
      return res;
    }

    // See if we can find a value for this parameter in the values list
    p_value = sdi12_gen_get_value_from_values_list(ps_var_values, p_name, name_len, &value_len);
    if(!p_value)
    {
      log_error(_logger, "Cannot find value for variable in values list: '%s'.", ps_var_values);
      return res;
    }

    // Get the type
    p_type  = sdi12_gen_get_cmdstr_var_param_value_using_key(p_c, CMD_VAR_PARAM_KEY_TYPE, &type_len);
    if(!p_type || type_len > 1)
    {
      log_error(_logger, "Variable type is is too long; it should be only one character long.");
      return res;
    }

    // Act accordingly with the type.
    switch(*p_type)
    {
      case CMD_VAR_PARAM_VALUE_TYPE_CHAR:
	res = sdi12_gen_replace_char_var_in_cmd_string(  ppu8_buffer, buffer_size, *pps_cmd,
						         p_value,     value_len);
	break;

      case CMD_VAR_PARAM_VALUE_TYPE_INT:
	res = sdi12_gen_replace_int_var_in_cmd_string(   ppu8_buffer, buffer_size, *pps_cmd,
						         p_value,     value_len);
	break;

      case CMD_VAR_PARAM_VALUE_TYPE_STRING:
	res = sdi12_gen_replace_string_var_in_cmd_string(ppu8_buffer, buffer_size, *pps_cmd,
							 p_value,     value_len);
	break;

      default:
	log_error(_logger, "Unknown variable type '%c'.", *p_type);
	return false;
    }

    // Move the command string cursor
    for(; *p_c && *p_c != CMD_VAR_PARAMS_END; p_c++) { /* Do nothing */ }
    *pps_cmd = ++p_c;  // Point to next character.

    return res;
  }

  /**
   * Replace a character type variable with it's value in a command string.
   *
   * @param[out] ppu8_buffer  a pointer to the pointer on a buffer. MUST be NOT NULL.
   *                          This double pointer allowed us to move an outside pointer on the buffer.
   * @param[in]  buffer_size  the number of bytes left in the buffer.
   * @param[in]  ps_cmd       part of the command string. MUST be NOT NULL.
   *                          <code>ps_cmd</code> points to the beginning of the variable.
   * @param[in]  ps_value     the value that is set to replace the variable.
   *                          Is NOT '\0' terminated.
   *                          MUST be NOT NULL and CANNOT be EMPTY.
   * @param[in]  value_len   the value's length.
   *                         We need this because <code>ps_value</code> is not '\0' terminated.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdi12_gen_replace_char_var_in_cmd_string(uint8_t **ppu8_buffer,
						       uint16_t  buffer_size,
						       char     *ps_cmd,
						       char     *ps_value,
						       uint16_t  value_len)
  {
    char    *p_range;
    uint16_t range_len;
    bool     res = false;

    // First check the length. A character variable always has a length of 1.
    if(value_len != 1)
    {
      log_error(_logger, "Cannot replace character variable with value whose length is different from 1.");
      return false;
    }

    // See if we are provided a range to compare the value to
    p_range = sdi12_gen_get_cmdstr_var_param_value_using_key(ps_cmd, CMD_VAR_PARAM_KEY_RANGE, &range_len);
    if(p_range)
    {
      res = sdi12_gen_char_value_is_in_range(*ps_value, p_range, range_len);
    }
    else
    {
      // No range set, so just accept the value
      res = true;
    }
    if(!res)
    {
      log_error(_logger, "Cannot set character variable to '%c'; value is not in range.", *ps_value);
      return res;
    }

    // Write the value to the buffer
    if(buffer_size < 1)
    {
      log_error(_logger, "Cannot write character variable '%c' to output buffer, buffer is too small.", *ps_value);
      return false;
    }
    **ppu8_buffer = *ps_value;
    (*ppu8_buffer)++;

    return res;
  }

  /**
   * Indicate if a char value is in a given range or not.
   *
   * @param[in] value     the value to test.
   * @param[in] p_range   the range string to use. MUST be NOT NULL.
   * @param[in] range_len the length of the range string,
   *                      so that we can work even if the range string is not '\0' terminated.
   *
   * @return true  if the value is in range.
   * @return false otherwise.
   */
  bool sdi12_gen_char_value_is_in_range(char value, char *p_range, uint16_t range_len)
  {
    char last_char, range_start;

    // Check that if a range begin character is found then we have a range end character as well.
    if(*p_range == CMD_VAR_PARAM_RANGE_BEGIN)
    {
      if(p_range[range_len - 1] == CMD_VAR_PARAM_RANGE_END)
      {
	p_range  ++;
	range_len--;
      }
      else
      {
	// Invalid range notation
	log_error(_logger, "Invalid range specification.");
	return false;
      }
    }

    // We have a range, check the value against it
    for(last_char = 0, range_start = 0; range_len; p_range++, range_len--)
    {
      if(*p_range == CMD_VAR_PARAM_RANGE_SEP)
      {
	range_start = last_char;
      }
      else
      {
	if(range_start)
	{
	  // Check against a range of values
	  if(range_start > *p_range)
	  {
	    log_error(_logger, "Invalid character variable range specification. Range lower bound '%c' is greater than range higher bound '%c'.", range_start, *p_range);
	    return false;
	  }
	  if(value >= range_start && value <= *p_range)
	  {
	    // Value is in range, so we can use it.
	    return true;
	  }
	  // Value is not in range.
	  last_char = range_start = 0;
	}
	else
	{
	  // Check against a single value
	  if(value == *p_range)
	  {
	    // Value is valid, so we can use it.
	    return true;
	  }
	  // We cannot decide just yet if the value is valid or not.
	  last_char = *p_range;
	}
      }
    }

    return false;
  }

  /**
   * Replace a string type variable with it's value in a command string.
   *
   * @param[out] ppu8_buffer  a pointer to the pointer on a buffer. MUST be NOT NULL.
   *                          This double pointer allowed us to move an outside pointer on the buffer.
   * @param[in]  buffer_size  the number of bytes left in the buffer.
   * @param[in]  ps_cmd       part of the command string. MUST be NOT NULL.
   *                          <code>ps_cmd</code> points to the beginning of the variable.
   * @param[in]  ps_value     the value that is set to replace the variable.
   *                          Is NOT '\0' terminated.
   *                          MUST be NOT NULL and CANNOT be EMPTY.
   * @param[in]  value_len   the value's length.
   *                         We need this because <code>ps_value</code> is not '\0' terminated.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdi12_gen_replace_string_var_in_cmd_string(uint8_t **ppu8_buffer,
							 uint16_t  buffer_size,
							 char     *ps_cmd,
							 char     *ps_value,
							 uint16_t  value_len)
  {
    char    *p_range;
    uint16_t range_len;
    uint8_t *pu8;
    uint8_t  field_length = 0;

    // See if we have a range parameter
    p_range = sdi12_gen_get_cmdstr_var_param_value_using_key(ps_cmd, CMD_VAR_PARAM_KEY_RANGE, &range_len);
    if(p_range)
    {
      if(sdi12_gen_string_value_is_in_range(ps_value, p_range, range_len))
      {
	// We found a match. Copy the value. No padding.
	// The field length is 0 so no padding will be applied.
	// Do nothing.
      }
      else
      {
	log_error(_logger, "String value is not in range.");
	return false;
      }
    }
    else
    {
      // See if we have a field length value
      p_range = sdi12_gen_get_cmdstr_var_param_value_using_key(ps_cmd, CMD_VAR_PARAM_KEY_LENGTH, &range_len);
      if(p_range)
      {
	// We have a field length parameter
	field_length = strn_string_to_uint(p_range, range_len, NULL);
	if(!field_length)
	{
	  log_error(_logger, "Failed to convert variable length to unsigned integer.");
	  return false;
	}

	// Check the the value is not too long
	if(value_len > field_length)
	{
	  log_error(_logger, "String variable value length %u is greater than the field length %u.", value_len, field_length);
	  return false;
	}
	field_length -= value_len;
      }
    }

    // Copy value
    for(pu8 = *ppu8_buffer ; value_len && buffer_size; value_len--, buffer_size--)
    {
      *pu8++ = *ps_value++;
    }
    if(value_len)
    {
      log_error(_logger, "Cannot set variable's value; buffer is too small.");
      return false;
    }

    // Add padding if needed.
    for(; field_length && buffer_size; field_length--, buffer_size--)
    {
      *pu8++ = CMD_VALUES_STRING_PADDING;
    }
    if(field_length)
    {
      log_error(_logger, "Cannot set variable's value; buffer is too small.");
      return false;
    }

    *ppu8_buffer = pu8;
    return true;
  }

  /**
   * Indicate if a string value is in a given range.
   *
   * @param[in] ps_value  the string value to test. MUST be NOT NULL.
   * @param[in] p_range   the range string to use. MUST be NOT NULL.
   * @param[in] range_len the length of the range string,
   *                      so that we can work even if the range string is not '\0' terminated.
   *
   * @return true  if the value is in range.
   * @return false otherwise.
   */
  bool sdi12_gen_string_value_is_in_range(char *ps_value, char *p_range, uint16_t range_len)
  {
    char *p_s, *p_e;

    // Check that if a range begin character is found then we have a range end character as well.
    if(*p_range == CMD_VAR_PARAM_RANGE_BEGIN)
    {
      if(p_range[range_len - 1] == CMD_VAR_PARAM_RANGE_END)
      {
	p_range  ++;
	range_len--;
      }
      else
      {
	// Invalid range notation
	log_error(_logger, "Invalid range specification.");
	return false;
      }
    }

    // Well, check the value against the range
    // We'll extract each range value one by one until we find a matching one
    for( ; *p_range && range_len; p_range++)
    {
      p_s = p_range;

      // Find the end of the item
      for(p_e = p_range ; *p_range && range_len; p_range++, range_len--)
      {
	if(*p_range == CMD_VAR_PARAM_RANGE_ITEM_SEP || *p_range == CMD_VAR_PARAM_RANGE_END) break;
      }
      p_e = p_range - 1;
      if(p_e < p_s)
      {
	log_error(_logger, "Invalid string range item.");
	return false;
      }

      // Compare with the value
      p_e++; // To simplify length and boundaries computations.
      if(strncmp(ps_value, p_s, (uint32_t)(p_e - p_s)) == 0)
      {
	// Match found
	return true;
      }
    }

    return false;
  }

  /**
     * Replace an integer type variable with it's value in a command string.
     *
     * @param[out] ppu8_buffer  a pointer to the pointer on a buffer. MUST be NOT NULL.
     *                          This double pointer allowed us to move an outside pointer on the buffer.
     * @param[in]  buffer_size  the number of bytes left in the buffer.
     * @param[in]  ps_cmd       part of the command string. MUST be NOT NULL.
     *                          <code>ps_cmd</code> points to the beginning of the variable.
     * @param[in]  ps_value     the value that is set to replace the variable.
     *                          Is NOT '\0' terminated.
     *                          MUST be NOT NULL and CANNOT be EMPTY.
     * @param[in]  value_len   the value's length.
     *                         We need this because <code>ps_value</code> is not '\0' terminated.
     *
     * @return true  on success.
     * @return false otherwise.
     */
  static bool sdi12_gen_replace_int_var_in_cmd_string(uint8_t **ppu8_buffer,
						      uint16_t  buffer_size,
						      char     *ps_cmd,
						      char     *ps_value,
						      uint16_t  value_len)
  {
    bool     ok;
    char    *p_range;
    uint16_t range_len;
    uint16_t field_length;
    int32_t  value;
    uint8_t *pu8;
    bool     add_plus_sign = false;

    // Check that the value is an integer
    value = strn_string_to_int(ps_value, value_len, &ok);
    if(!ok)
    {
      log_error(_logger, "Cannot convert integer variable value to integer.");
      return false;
    }

    // See if we have a range parameter
    p_range = sdi12_gen_get_cmdstr_var_param_value_using_key(ps_cmd, CMD_VAR_PARAM_KEY_RANGE, &range_len);
    if(p_range)
    {
      if(!sdi12_gen_int_value_is_in_range(value, p_range, range_len))
      {
	log_error(_logger, "Integer value is not in range.");
	return false;
      }
    }

    // See if we have to use '+' character for positive number.
    // If a single range positive value has the + sign then use it for the value
    if(value >= 0 && *ps_value != '+')
    {
      for( ; range_len; range_len--)
      {
	if(*p_range++ == '+')
	{
	  add_plus_sign = true;
	  value_len++;
	  break;
	}
      }
    }

    // See if we have a field length value
    field_length = 0;
    p_range = sdi12_gen_get_cmdstr_var_param_value_using_key(ps_cmd, CMD_VAR_PARAM_KEY_LENGTH, &range_len);
    if(p_range)
    {
      // We have a field length parameter
      field_length = strn_string_to_uint(p_range, range_len, NULL);
      if(!field_length)
      {
	log_error(_logger, "Failed to convert variable length to unsigned integer.");
	return false;
      }

      // Check the the value is not too long
      if(value_len > field_length)
      {
	log_error(_logger, "Integer variable value length %u is greater than the field length %u.", value_len, field_length);
	return false;
      }
      field_length -= value_len;
    }

    // Write the value
    // Write the sign first
    pu8 = *ppu8_buffer;
    if(buffer_size < 1)
    {
      log_error(_logger, "Cannot set variable's value; buffer is too small.");
      return false;
    }
    if(*ps_value == '+' || *ps_value == '-') { *pu8++ = '-'; value_len--; buffer_size--; ps_value++; }
    else if(add_plus_sign)                   { *pu8++ = '+'; value_len--; buffer_size--;             }

    // Pad the value if needed
    for( ; field_length && buffer_size; field_length--, buffer_size--) { *pu8++ = '0'; }
    if(field_length)
    {
      log_error(_logger, "Cannot set variable's value; buffer is too small.");
      return false;
    }

    // Copy the value
    for(; value_len && buffer_size; value_len--, buffer_size--)
    {
      *pu8++ = *ps_value++;
    }
    if(value_len)
    {
      log_error(_logger, "Cannot set variable's value; buffer is too small.");
      return false;
    }

    *ppu8_buffer = pu8;
    return true;
  }

  /**
     * Indicate if an integer value is in a given range.
     *
     * @param[in] value     the integer value to test. MUST be NOT NULL.
     * @param[in] p_range   the range string to use. MUST be NOT NULL.
     * @param[in] range_len the length of the range string,
     *                      so that we can work even if the range string is not '\0' terminated.
     *
     * @return true  if the value is in range.
     * @return false otherwise.
     */
  bool sdi12_gen_int_value_is_in_range(int32_t value, char *p_range, uint16_t range_len)
  {
    char    *p_s, *p_e;
    bool    compare_to_range, ok;
    int32_t current_value, last_value;

    // Check that if a range begin character is found then we have a range end character as well.
    if(*p_range == CMD_VAR_PARAM_RANGE_BEGIN)
    {
      if(p_range[range_len - 1] == CMD_VAR_PARAM_RANGE_END)
      {
	p_range  ++;
	range_len--;
      }
      else
      {
	// Invalid range notation
	log_error(_logger, "Invalid range specification.");
	return false;
      }
    }

    // Well, check the value against the range
    // We'll extract each range value one by one until we find a matching one
    for(compare_to_range = false ; *p_range && range_len; p_range++)
    {
      if(*p_range == CMD_VAR_PARAM_RANGE_SEP) continue;
      p_s = p_range;

      // Find the end of the item
      for(p_e = p_range ; *p_range && range_len; p_range++, range_len--)
      {
	if( *p_range == CMD_VAR_PARAM_RANGE_ITEM_SEP ||
	    *p_range == CMD_VAR_PARAM_RANGE_SEP      ||
	    *p_range == CMD_VAR_PARAM_RANGE_END) break;
      }
      p_e = p_range - 1;
      if(p_e < p_s)
      {
	log_error(_logger, "Invalid string range item.");
	return false;
      }

      // Convert string to integer value
      current_value = strn_string_to_int(p_s, (uint32_t)(p_e - p_s + 1), &ok);
      if(!ok)
      {
	log_error(_logger, "Failed to convert range value to integer");
	return false;
      }

      // Compare
      if(compare_to_range) { if(value >= last_value && value <= current_value) { return true; }}
      else                 { if(value == current_value)                        { return true; }}

      last_value = current_value;
      compare_to_range = (*p_range == CMD_VAR_PARAM_RANGE_SEP);
    }

    return false;
  }

  /**
   * Get a command string variable's parameter value using the parameter's key.
   *
   * @param[in]  ps_var_str the variable's parameters string.
   * @param[in]  key        the key of the parameter we are looking for.
   *                        Must be in [a-zA-Z].
   * @param[out] pu16_len   where the length of the parameter's value is written to.
   *                        MUST be NOT NULL.
   *
   * @return a pointer the the first character of the value.
   *         <code>*pu16_len</code> is set with the value's character count.
   * @return NULL if we could not find a value. <code>*pu16_len</code> is set to 0.
   */
  static char *sdi12_gen_get_cmdstr_var_param_value_using_key(char     *ps_var_str,
							      char      key,
							      uint16_t *pu16_len)
  {
    char     *pc, *p_value;
    uint16_t len;

    // Check parameters
    if(!ps_var_str || !ps_var_str[0] || !char_is_alpha(key))
    {
      log_error(_logger, "Cannot get variable's parameter value from string '%s' and key '%c'.", ps_var_str ? ps_var_str : "NULL", key);
      *pu16_len = 0;
      return NULL;
    }

    // Look for the key
    for(pc = ps_var_str; *pc; pc++)
    {
      if(*pc == key && *(pc + 1) == CMD_VAR_PARAM_KEY_VALUE_SEP)
      {
	pc += 2;
	break;
      }
    }

    // Get the value
    p_value = pc;
    for(len = 0; *pc && *pc != CMD_VAR_PARAMS_SEP && *pc != CMD_VAR_PARAMS_END; pc++, len++) { /* Do nothing */ }
    if( len == 0)
    {
      // The value cannot be empty
      log_error(_logger, "Cannot get variable's parameter value from string '%s' and key '%c'.", ps_var_str, key);
      *pu16_len = len;
      return NULL;
    }

    *pu16_len = len;
    return p_value;
  }

  /**
   * Get a value from a variable parameters string using a key string.
   *
   * @param[in]  p_params   the variable parameters's string. MUST be NOT NULL.
   *                        Do not need to be '\0' terminated.
   * @param[in]  params_len the length of the <code>p_params</code> string.
   * @param[in]  key        the key.
   * @param[out] pu16_len   where the value's length will be written to. MUST be NOT NULL.
   *                        I set to 0 if the function fails.
   *
   * @return the value. Is not '\0' terminated. It's length is in <code>*pu16_len</code>
   * @return NULL if no value has been found.
   */
  static const char *sdi12_gen_get_value_from_var_params_using_key(const char *p_params,
								   uint16_t    params_len,
								   char        key,
								   uint16_t   *pu16_len)
  {
    const char *pc, *p_end, *p_value;
    uint16_t len;

    // Look for the key
    p_end  = p_params + params_len - 2; // -2 because a minimum key-value pair is 3 bytes long: "k:v".
    for(pc = p_params; pc < p_end; pc++)
    {
      if(*pc == key && *(pc + 1) == CMD_VAR_PARAM_KEY_VALUE_SEP)
      {
	// We found the key
	break;
      }
    }
    if(pc >= p_end)
    {
      // key not found
      *pu16_len = 0;
      return NULL;
    }

    // Get the value
    pc      += 2; // Go the the next character after the colon.
    p_value = pc;
    for(len = 0, p_end = p_params + params_len; pc < p_end && *pc != CMD_VAR_PARAMS_SEP; pc++, len++)
    {
      /* Do nothing */
    }
    if(len == 0)
    {
      // The value cannot be empty
      log_error(_logger, "Empty parameter values are not allowed. Key '%c'.", key);
      *pu16_len = len;
      return NULL;
    }

    *pu16_len = len;
    return p_value;
  }

  /**
   * Get a variable's field length from it's parameters.
   *
   * @note If the length is not specified by a range, but by a fixed value,
   *       then <code>*pu8_min</code> and <code>*pu8_max</code> are set to the same value,
   *       the fixed field's length.
   *
   * @param[in]  p_params   the variable parameters's string. MUST be NOT NULL.
   *                        Do not need to be '\0' terminated.
   * @param[in]  params_len the length of the <code>p_params</code> string.
   * @param[out] pu16_min   where the length's minimum value is written to. MUST be NOT NULL.
   * @param[out] pu16_max   where the length's maximum value is written to. MUST be not NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdi12_gen_get_field_length_from_var_params(const char *p_params,
							 uint16_t    params_len,
							 uint16_t   *pu16_min,
							 uint16_t   *pu16_max)
  {
    bool        ok;
    const char *p_v, *p_s;
    uint16_t    len;
    uint32_t    ui;

    // Get the string value
    p_v  = sdi12_gen_get_value_from_var_params_using_key(p_params,
							 params_len,
							 CMD_VAR_PARAM_KEY_LENGTH,
							 &len);
    if(!p_v) { goto return_error; }

    // See if it is a range or not
    if(*p_v == CMD_VAR_PARAM_RANGE_BEGIN)
    {
      if(p_v[len - 1] != CMD_VAR_PARAM_RANGE_END)
      {
	log_error(_logger, "Field length range syntax is invalid.");
	goto return_error;
      }

      // Extract the minimum boundary
      p_v++; len -= 2; // To not take into account CMD_VAR_PARAM_RANGE_BEGIN and CMD_VAR_PARAM_RANGE_END
      for(p_s = p_v; len && *p_v != CMD_VAR_PARAM_RANGE_SEP; p_v++, len--) { /* Do nothing */ }
      if(len)
      {
	ui = strn_string_to_uint(p_s, (uint32_t)(p_v - p_s), &ok);
	if(!ok || ui > 255u) { goto return_error; }
	*pu16_min = ui;
      }
      else { goto return_error; }

      // Extract the maximum boundary
      p_v++; len--;
      if(!len || *p_v != CMD_VAR_PARAM_RANGE_SEP) { goto return_error; }
      p_v++;
      for(p_s = p_v; *p_v != CMD_VAR_PARAM_RANGE_END; p_v++) { /* Do nothing */ }
      ui = strn_string_to_uint(p_s, (uint32_t)(p_v - p_s), &ok);
      if(!ok || ui > 255u || ui < *pu16_min) { goto return_error; }
      *pu16_max = ui;
      return true;
    }
    else
    {
      // A fixed value is set
      ui = strn_string_to_uint(p_v, len, &ok);
      if(ui > 255u) { ui = 0; }
      *pu16_min = *pu16_max = ui;
      return ok;
    }

    return_error:
    *pu16_min = *pu16_max = 0;
    return false;
  }

  /**
   * Get a value from a key-value list using the key.
   *
   * @param[in]  ps_var_values the list of key-pair values. MUST be NOT NULL.
   * @param[in]  ps_key        the key to look for. MUST be NOT NULL.
   * @param[in]  key_len       the key's length.
   *                           So that we can use it even if it is not '\0' terminated.
   * @param[out] pu16_len      the length of the value found. MUST be NOT NULL.
   *
   * @return a pointer to the first character of the value found.
   *         <code>pu16_len</code> is set to the value's character count.
   * @return NULL if no value has been found.
   */
#define _restart_plus_one() \
    p_c = p_start;        \
    p_start++;            \
    p_k = (char *)ps_key
#define _restart_from_current_plus_one() \
    p_start = p_c + 1; \
    p_k = (char *)ps_key
  static char *sdi12_gen_get_value_from_values_list(const char *ps_var_values,
						    const char *ps_key,
						    uint16_t    key_len,
						    uint16_t   *pu16_len)
  {
    char *p_c, *p_k;
    char *p_start = (char *)ps_var_values;
    char *p_kend  = (char *)(ps_key + key_len);

    // Look for the key with the given name in the list
    for(p_c = p_start, p_k = (char *)ps_key; *p_c; p_c++)
    {
      if(*p_c == SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_KEY_VALUE_PAIR_SEP)
      {
	if(p_k != p_kend) { _restart_from_current_plus_one(); }

	// Compare
	if(strncmp(ps_key, p_start, (uint16_t)(p_c - p_start)) == 0)
	{
	  // Key found
	  p_start = ++p_c;
	  break;
	}
	// Key not found
	_restart_from_current_plus_one();
      }
      else if(*p_c == *p_k)
      {
	if(p_k == p_kend) { _restart_plus_one(); }
	else              { p_k++;               }
      }
      else { _restart_plus_one(); }
    }
    if(!*p_c)
    {
      // No key found
      *pu16_len = 0;
      return NULL;
    }

    // We have the key and p_c points to the first character of the value.
    // Check that the value is not empty
    if(!*p_start || *p_start == SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_PAIRS_SEP)
    {
      // There is no value.
      *pu16_len = 0;
      return NULL;
    }
    // Get the value
    for( ; *p_c && *p_c != SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_PAIRS_SEP; p_c++) { /* Do nothing */ }

    *pu16_len = (uint16_t)(p_c - p_start);
    return p_start;
  }
#undef _restart_plus_one
#undef _restart_from_current_plus_one


  /**
   * Return the next variable string in a given command string.
   *
   * @param[in,out] pps_cmdstr      points to the command string in which to look for the variable.
   *                                MUST be NOT NULL.
   *                                If the function is successful then the pointer is moved so that
   *                                <code>*pps_cmdstr</code> point to the next character
   *                                after the variable definition in the command string.
   * @param[in]     only_get_params only get the parameters (without CMD_VAR_BEGIN_CHAR,
   *                                CMD_VAR_PARAMS_BEGIN and CMD_VAR_PARAMS_END),
   *                                or get the whole variable string?
   * @param[out]    pu16_len        where will be written the variable string's length. MUST be NOT NULL.
   *                                Set to 0 in case of error.
   * @param[out]    pu16_prefix_len where the prefix's length is written to.
   *                                Can be NULL if we are not interested in this information.
   *                                The prefix are the characters in the command string that are before
   *                                the variable definition.
   *                                The prefix can also be seen as the numbers of characters we have
   *                                moved over in the command description string before finding the
   *                                variable definition.
   *                                Is not set to 0 if we do not find a variable.
   *                                In this case it contains the number of characters we've looked at.
   *
   * @return the beginning of the variable definition.
   *         Is not '\0' terminated; use <code>*pu16_len</code> to get the string's length.
   * @return NULL if no variable is found.
   */
  static const char *sdi12_gen_get_cmdstr_next_var(const char **pps_cmdstr,
						   bool         only_get_params,
						   uint16_t    *pu16_len,
						   uint16_t    *pu16_prefix_len)
  {
    uint16_t    prefix_len;
    const char *p_s;
    const char *ps_cmdstr = *pps_cmdstr;

    // Look for the beginning of a variable
    for(p_s = NULL, prefix_len = 0; *ps_cmdstr; ps_cmdstr++, prefix_len++)
    {
      if(*ps_cmdstr == CMD_VAR_BEGIN_CHAR && *(ps_cmdstr + 1) == CMD_VAR_PARAMS_BEGIN)
      {
	p_s = ps_cmdstr;
	break;
      }
    }
    if(pu16_prefix_len) { *pu16_prefix_len = prefix_len; }
    if(!p_s)
    {
      // No variable found
      *pu16_len = 0;
      return NULL;
    }

    // Look for the end of the variable
    for( ; *ps_cmdstr && *ps_cmdstr != CMD_VAR_PARAMS_END; ps_cmdstr++) { /* Do nothing */ }
    if(*ps_cmdstr == CMD_VAR_PARAMS_END)
    {
      // End of variable found
      *pps_cmdstr = ps_cmdstr + 1;
      if(only_get_params)
      {
	*pu16_len = (uint16_t)(ps_cmdstr - p_s + 1) - 3; // -3 to not take into account CMD_VAR_BEGIN_CHAR, CMD_VAR_PARAMS_BEGIN and CMD_VAR_PARAMS_END
	return p_s + 2;                                  // +2 to jump over CMD_VAR_BEGIN_CHAR and CMD_VAR_PARAMS_BEGIN.
      }
      else
      {
	*pu16_len = (uint16_t)(ps_cmdstr - p_s + 1);
	return p_s;
      }
    }

    *pu16_len = 0;
    return NULL;
  }

  /**
   * Get a variable's parameter list, as a string, for a given variable's name.
   *
   * @param[in]  ps_cmdstr   the command string in which to look for the variable. MUST be NOT NULL.
   * @param[in]  ps_var_name the variable's name. MUST be NOT NULL.
   *                         If empty then this function will fail.
   * @param[out] pu16_len    where will be written the parameters string's length. MUST be NOT NULL.
   *                         Set to 0 in case of error.
   *
   * @return the variable's parameters string. Is not '\0' terminated;
   *         use <code>*pu16_len</code> to know it's length.
   * @return NULL if the function failed.
   */
/*  static const char *sdi12_gen_get_cmdstr_var_params_using_var_name(const char *ps_cmdstr,
								    const char *ps_var_name,
								    uint16_t   *pu16_len)
  {
    const char *p_v, *p_v_sav;
    uint16_t    len,  len_sav;

    while((p_v = sdi12_gen_get_cmdstr_next_var(&ps_cmdstr, true, &len, NULL)))
    {
      p_v_sav = p_v;
      len_sav = len;

      for( ; len > 3; p_v++, len--)
      {
	if(*p_v == CMD_VAR_PARAM_KEY_NAME && *(p_v + 1) == CMD_VAR_PARAM_KEY_VALUE_SEP)
	{
	  if(strncmp(p_v + 2, ps_var_name, len - 2) == 0)
	  {
	    // Found match
	    *pu16_len = len_sav;
	    return p_v_sav;
	  }
	}
      }
    }

    *pu16_len = 0;
    return NULL;
  }*/



#ifdef __cplusplus
}
#endif
