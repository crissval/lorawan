/**
 * @file  sdi12_standardcommands.c
 * @brief 
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <sdi12gen_standardcommands.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Describe the standard commands.
   * The commands supported by all sensors.
   */
static const SDI12GenSensorCommands sdi12_gen_standard_commands_descriptions =
{
    NULL,  // Because supported by all sensors
    {
	{
	  SDI12_CMD_STD_ACK_ACTIVE,
	  "Acknowledge Active",
	  "a!",
	  "a<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_QUERY_ADDRESS,
	  "Address Query",
	  "?!",
	  "a<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_SEND_ID,
	  "Send Identification",
	  "aI!",
	  "a${n:sdi12_version,t:i,l:2}${n:vendor_id,t:s,l:8}${n:sensor_model,t:s,l:6}"
	  "${n:sensor_version,t:s,l:3}${n:optional,t:s,l:[0..13]}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_CHANGE_ADDRESS,
	  "Change Address",
	  "aA${n:new_address,t:c,r:[0..9a..zA..Z]}!",
	  "b<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_START_MEASUREMENT,
	  "Start Measurement",
	  "aM!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  SDI12_CMD_STD_SEND_DATA,
	  NULL
	},
	{
	  SDI12_CMD_STD_START_MEASUREMENT_WITH_CRC,
	  "Start Measurement with CRC",
	  "aMC!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  SDI12_CMD_STD_SEND_DATA_WITH_CRC,
	  NULL
	},
	{
	  SDI12_CMD_STD_SEND_DATA,
	  "Send Data",
	  "aD${n:inc,t:i,l:1,r:[0..9]}!",
	  "a<values><CR><LF>",
	  SDI12_CMD_CONFIG_DO_NOT_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_SEND_DATA_WITH_CRC,
	  "Send Data With CRC",
	  "aD${n:inc,t:i,l:1,r:[0..9]}!",
	  "a<values><CRC><CR><LF>",
	  SDI12_CMD_CONFIG_DO_NOT_SEND_BREAK,
	  NULL,
	  NULL,
	  NULL
	},
	{
	  SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS,
	  "Additional Measurements",
	  "aM${n:num,t:i,l:1,r:[1..9]}!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  SDI12_CMD_STD_SEND_DATA,
	  NULL
	},
	{
	  SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS_WITH_CRC,
	  "Additional Measurements with CRC",
	  "aMC${n:num,t:i,l:1,r:[1..9]}!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  SDI12_CMD_STD_SEND_DATA_WITH_CRC,
	  NULL
	},
	{
	  SDI12_CMD_STD_START_VERIFICATION,
	  "Start Verification",
	  "aV!",
	  "a${n:time_sec,t:i,l:3}${n:nb_values,t:i,l:1}<CR><LF>",
	  SDI12_CMD_CONFIG_SEND_BREAK                   |
	  SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST |
	  SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN,
	  NULL,
	  SDI12_CMD_STD_SEND_DATA,
	  NULL
	},
	// End of command list indication
	{ NULL, NULL, NULL, NULL, SDI12_CMD_CONFIG_NONE, NULL, NULL, NULL }
    }
};

/**
 * The variable values for the Additional Start Measurement commands, CRC or not.
 *
 * Use the additional command id as index to this table.
 */
static const char *_sdi12_gen_stdcmds_additionalm_varvalues[10] =
{
    NULL, "num:1", "num:2", "num:3", "num:4", "num:5", "num:6", "num:7", "num:8", "num:9"
};


/**
 * Return the standard command descriptions.
 *
 * @return the descriptions.
 */
const SDI12GenSensorCommands *sdi12_gen_standard_commands_get_descriptions()
{
  return &sdi12_gen_standard_commands_descriptions;
}

/**
 * Return the variables values string for the additional commands, with or without CRC,
 * using the additional command's identifier.
 *
 * @param[in] id the additionnal's command identifier, in range [1..9].
 *
 * @return the variable values string.
 * @return NULL if id is 0.
 * @return NULL if id is not valid.
 */
const char *sdi12_gen_stdcmds_get_additionalm_varvalues_using_id(uint8_t id)
{
  return id < 10 ? _sdi12_gen_stdcmds_additionalm_varvalues[id] : NULL;
}


#ifdef __cplusplus
}
#endif
