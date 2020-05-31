/**
 * @file  sdi12_generic.h
 * @brief Header file for the generic SDI-12 implementation.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __SDI_12_SDI12_GENERIC_H__
#define __SDI_12_SDI12_GENERIC_H__

#include <sdi12command.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_KEY_VALUE_PAIR_SEP ':'
#define SDI12_GEN_CMD_DESC_VAR_VALUES_LIST_PAIRS_SEP          ','

#define SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_TIMEOUT_SEC "time_sec"
#define SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_NB_VALUES   "nb_values"
#define SDI12_GEN_CMD_DESC_VAR_NAME_SEND_DATA_INCREMENT   "inc"


  /**
   * The structure used to describe a SDI-12 command.
   *
   * In the comments present in this structure the &lt;variable&gt; value
   * represents a command variable.
   * It is represented in the command description string by the pattern ${...}
   * where the "..." are the variable parameters.
   * Some parameters are mandatory, others are optional.
   * The parameters are specified using a key-value pair,
   * where the key is a single character separated from it's value by a colon.
   * Parameters are separated by commas.
   * There MUST be only the REQUIRED CHARACTERS,
   * NO SPACING CHARACTERS that have no meaning for the command.
   *
   * Here is a list of the parameters and their meanings.
   * <ul>
   * 	<li>
   *	  The variable's name, key 'n', mandatory.<br/>
   *	  Example: <code>n:a_name</code>
   * 	</li>
   * 	<li>
   * 	  The variable's type, key 't', mandatory.<br/>
   * 	  Possible values are:
   * 	    <ul>
   * 	      <li>'c' for a character.</li>
   * 	      <li>'i' for a signed integer.</li>
   * 	      <li>'s' for a string.</li>
   * 	    </ul>
   * 	</li>
   * 	<li>
   * 	  The variable's length, key 'l', optional.<br/>
   * 	  It is highly recommended to use it though,
   * 	  and it is mandatory for response descriptions,
   * 	  so that we can know how to decode the response.<br/>
   * 	  It is useless for the 'c' type, because it's length always is 1 character.
   * 	  It is also not used with strings when a range of values is specified.<br/>
   * 	  The length can be specified using two methods:
   * 	  <ul>
   * 	    <li>
   * 	      If the length is constant then just indicate its value.<br/>
   * 	      Example: <code>l:10</code> for a constant 10 characters length.
   * 	    </li>
   * 	    <li>
   * 	      If the length can change the specify the range using the format [min..max].
   * 	      A variable length can only be used for the last variable.<br/>
   * 	      Example: <code>l:[3..7]</code> for a length between 3 and 7 characters.
   * 	    </li>
   * 	  </ul>
   * 	  If the variable's value is too short, then it will be padded to meet the length specified.
   * 	  Numbers are 0 padded on the left, while the strings are space padded at the end.
   * 	</li>
   * 	<li>
   * 	  The variable's value range, key 'r', optional.<br/>
   * 	  The syntax changes depending on the variable's type.
   * 	  <ul>
   * 	    <li>
   * 	      For the characters it is: <code>r:[c_min..c_max]</code>.
   * 	      Eventually, several ranges can be specified this way:
   * 	      <code>r:[c_min1..c_max2c_min2..cmax2c_min3..c_max3]</code>, and so on...<br/>
   * 	      You can also specify single values using the syntax:
   * 	      <code>[fjg]</code>, which means that the character values can only be
   * 	      'f', 'j' or 'g' and nothing else.<br/>
   * 	      You can also mix both notations: <code>[fjg0..9B..K]</code>,
   * 	      which means that the value can be 'f', 'j', 'g', any character between '0' and '9',
   * 	      or any character between 'B' and 'K'.
   * 	    </li>
   * 	    <li>
   * 	      For integers the syntax is: <code>r:[min..max]</code>.
   * 	      Or, if only a finite number of values are possible,
   * 	      then it is possible to list these values by using a range made of a '|'
   * 	      separated list of values; the syntax is: <code>[v1|v2|v3]</code>.<br/>
   * 	      If the positive values start with a '+' character then all positive values
   * 	      in the resulting command also will have the '+' sign.
   * 	      Here again you can also mix notation, for example:
   * 	      <code>[v1|v2|min1..max2|v3|min2..max2]</code>.
   * 	    </li>
   * 	    <li>
   * 	      For strings, the range specify a limited number of possible values.
   * 	      The syntax is: <code>r:[value1,value2,value3]</code>.
   * 	      String values CANNOT contain a comma character.<br/>
   * 	    </li>
   * 	  </ul>
   * 	</li>
   * </ul>
   *
   * The command and responses also can contain some variables or fields with
   * with predefined meaning:
   * <ul>
   * 	<li>
   * 	  &lt;CRC&gt; can only be present in a response description and indicate
   * 	  that the response includes a CRC value.
   * 	</li>
   * 	<li>
   * 	  &lt;CR&gt; can only be present in a response description and represents
   * 	  a carriage return character.
   * 	</li>
   * 	<li>
   * 	  &lt;LF&gt; can only be present in a response description and represents
   * 	  a line feed character.
   * 	</li>
   * 	<li>
   * 	  &lt;values&gt; can only be present in a response description and represents
   * 	  a varying number of SDI-12 command values.
   * 	</li>
   * </ul>
   */
  typedef struct SDI12GenCommandDescription
  {
    /**
     * The command's name.
     * MUST be NOT NULL and NOT EMPTY.
     */
    const char *ps_cmd_name;

    /**
     * The command's full name.
     * Can be the same as <code>ps_cmd_name</code>.
     * MUST be NOT NULL and NOT EMPTY.
     */
    const char *ps_full_name;

    /**
     * The string representing the command to send.
     * MUST be NOT NULL and NOT EMPTY.
     * The format is like this:
     *   "(a|\?)([0-9a-zA-Z]*<variable>*)*!"
     */
    const char *ps_cmd;

    /**
     * The string representing the response to the command.
     * MUST be NOT NUUL and NOT empty.
     * The format is like this:
     *   "(a|b)([0-9a-zA-Z]*<variable>*<values>*)*(<CRC>)?<CR><LF>"
     */
    const char *ps_response;

    /**
     * The command's configuration.
     * A ORed combination of SDI12CommandConfigFlags values.
     */
    uint8_t config;

    /**
     * Callback function to process the response from the sensor.
     *
     * Can be NULL if there is no need for specialised processing of the response.
     *
     * @return true  if the response seems correct.
     * @return false in case of error.
     */
    sdi12_command_resp_cb_t process_response;

    /**
     * The name of the command to call if this command is successful.
     * Can be NULL if there is no command to call next.
     */
    const char *ps_next_cmd_name;

    /**
     * The name of the sensor type the next command to call belongs to.
     * Can be NULL if the next command is a standard command.
     */
    const char *ps_next_command_sensor_type_name;
  }
  SDI12GenCommandDescription;

  /**
   * Structure used to describe all the commands for a sensor.
   */
  struct SDI12GenSensorCommands;
  typedef struct SDI12GenSensorCommands SDI12GenSensorCommands;
  struct SDI12GenSensorCommands
  {
    /**
     * The sensor's name.
     * MUST be NOT EMPTY.
     * Can only be NULL if the commands are supported by all sensors.
     */
    const char *ps_sensor_name;

    /**
     * The list of commands.
     *
     * The list is terminated by a command whose name is set to NULL.
     */
    const SDI12GenCommandDescription commands[];
  };


  /**
   * Defines the variable types
   */
  typedef enum SDI12GenCmdDescVarType
  {
    SDI12_GEN_CMDDESC_VAR_TYPE_UNDEFINED = 0,
    SDI12_GEN_CMDDESC_VAR_TYPE_CHAR      = 'c',
    SDI12_GEN_CMDDESC_VAR_TYPE_INT       = 'i',
    SDI12_GEN_CMDDESC_VAR_TYPE_STRING    = 's'
  }
  SDI12GenCmdDescVarType;


  extern bool sdi12_gen_command_init_from_description(SDI12Command                      *p_cmd,
						      const SDI12GenCommandDescription  *p_desc,
						      char                               addr,
						      const char                        *ps_var_values,
						      sdi12_command_result_cb_t          success_cb,
						      void                              *pv_success_cbargs,
						      sdi12_command_result_cb_t          failed_cb,
						      void                              *pv_failed_cbargs);

  extern const char *sdi12_gen_get_cmd_response_value(     const SDI12Command     *p_cmd,
						           const char             *ps_var_name,
						           uint16_t               *pu16_value_len,
						           SDI12GenCmdDescVarType *p_type);
  extern char sdi12_gen_get_cmd_response_value_as_char(    const SDI12Command     *p_cmd,
						           const char             *ps_var_name,
						           char                    default_value,
						           bool                   *pb_ok);
  extern int32_t sdi12_gen_get_cmd_response_value_as_int(  const SDI12Command     *p_cmd,
  						           const char             *ps_var_name,
  						           int32_t                 default_value,
  						           bool                   *pb_ok);
  extern uint32_t sdi12_gen_get_cmd_response_value_as_uint(const SDI12Command     *p_cmd,
    						           const char             *ps_var_name,
    						           uint32_t                default_value,
    						           bool                   *pb_ok);
  extern char *sdi12_gen_get_cmd_response_value_as_string( const SDI12Command     *p_cmd,
							   const char             *ps_var_name,
							   char                   *ps_buffer,
							   uint16_t                buffer_size,
							   const char             *default_value,
							   bool                   *pb_ok);

  extern bool sdi12_gen_char_value_is_in_range(  char    value,    char *p_range, uint16_t range_len);
  extern bool sdi12_gen_string_value_is_in_range(char   *ps_value, char *p_range, uint16_t range_len);
  extern bool sdi12_gen_int_value_is_in_range(   int32_t value,    char *p_range, uint16_t range_len);


#ifdef __cplusplus
}
#endif
#endif /* __SDI_12_SDI12_GENERIC_H__ */
