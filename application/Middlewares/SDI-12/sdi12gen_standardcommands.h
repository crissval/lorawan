/**
 * @file  sdi12_standardcommands.h
 * @brief 
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __SDI_12_SDI12_STANDARDCOMMANDS_H__
#define __SDI_12_SDI12_STANDARDCOMMANDS_H__

#include "sdi12gen.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SDI12_CMD_STD_ACK_ACTIVE                       "ackActive"
#define SDI12_CMD_STD_QUERY_ADDRESS                    "queryAddr"
#define SDI12_CMD_STD_SEND_ID                          "sendId"
#define SDI12_CMD_STD_CHANGE_ADDRESS                   "changeAddr"
#define SDI12_CMD_STD_START_MEASUREMENT                "startM"
#define SDI12_CMD_STD_START_MEASUREMENT_WITH_CRC       "startMWithCRC"
#define SDI12_CMD_STD_SEND_DATA                        "sendData"
#define SDI12_CMD_STD_SEND_DATA_WITH_CRC               "sendDataWithCRC"
#define SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS          "additionalM"
#define SDI12_CMD_STD_ADDITIONAL_MEASUREMENTS_WITH_CRC "additionalMWithCRC"
#define SDI12_CMD_STD_START_VERIFICATION               "startVerification"


  extern const SDI12GenSensorCommands *sdi12_gen_standard_commands_get_descriptions();

  extern const char *sdi12_gen_stdcmds_get_additionalm_varvalues_using_id(uint8_t id);

#ifdef __cplusplus
}
#endif
#endif /* __SDI_12_SDI12_STANDARDCOMMANDS_H__ */
