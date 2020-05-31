/**
 * @file  sdi12command.hpp
 * @brief Header for the base class for all SDI-12 commands.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __SDI_12_COMMANDS_SDI12COMMAND_H__
#define __SDI_12_COMMANDS_SDI12COMMAND_H__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SDI12_ADDRESS_LEN  1  // 1 byte

#define SDI12_COMMAND_END_OF_COMMAND_PATTERN   "!"
#define SDI12_COMMAND_END_OF_COMMAND_LEN       1
#define SDI12_COMMAND_END_OF_RESPONSE_PATTERN  "\r\n"
#define SDI12_COMMAND_END_OF_RESPONSE_LEN      2
#define SDI12_COMMAND_BUFFER_EXTRA_LEN         2  // So that we can always add a '\0' ending character to the command and to the response. So that they can be used as strings.

#define SDI12_COMMAND_VALUE_SIGN_LEN       1
#define SDI12_COMMAND_VALUE_POINT_LEN      1
#define SDI12_COMMAND_VALUE_NB_DIGITS_MIN  1
#define SDI12_COMMAND_VALUE_NB_DIGITS_MAX  7
#define SDI12_COMMAND_VALUE_LEN_MIN        (SDI12_COMMAND_VALUE_SIGN_LEN + SDI12_COMMAND_VALUE_NB_DIGITS_MIN)
#define SDI12_COMMAND_VALUE_LEN_MAX        (\
    SDI12_COMMAND_VALUE_SIGN_LEN  + \
    SDI12_COMMAND_VALUE_POINT_LEN + \
    SDI12_COMMAND_VALUE_NB_DIGITS_MAX)

#define SDI12_CMD_VALUES_LEN_MAX  75

#define SDI12_COMMAND_CRC_LEN  3


#define sdi12_command_signal_success(p_cmd) \
  do \
  { \
    (p_cmd)->error = SDI12_COMMAND_ERROR_NONE;  \
    if((p_cmd)->signal_success) \
    { \
      (p_cmd)->signal_success(p_cmd, (p_cmd)->pv_signal_success_args); \
    } \
  } \
  while(0)
#define sdi12_command_signal_failed(p_cmd, err) \
  do \
  { \
    (p_cmd)->error = (err);  \
    if((p_cmd)->signal_failed) \
    { \
      (p_cmd)->signal_failed(p_cmd, (p_cmd)->pv_signal_failed_args); \
    } \
  } \
  while(0)


  struct SDI12Command;
  typedef struct SDI12Command SDI12Command;

  typedef bool (*sdi12_command_resp_cb_t)(  SDI12Command *p_command);
  typedef void (*sdi12_command_result_cb_t)(SDI12Command *p_command, void *pv_args);


  /**
   * Defines the different error values for the command.
   */
  typedef enum SDI12CommandError
  {
    SDI12_COMMAND_ERROR_UNDEFINED,                 ///< Undefined because the command has not been sent yet.
    SDI12_COMMAND_ERROR_NONE,                      ///< No error.
    SDI12_COMMAND_ERROR_CMD_INITIALISATION_FAILED, ///< Could not initialise the command.
    SDI12_COMMAND_ERROR_BUSY,                      ///< The bus is busy.
    SDI12_COMMAND_ERROR_TIMEOUT,                   ///< The command has timed out.
    SDI12_COMMAND_ERROR_INVALID_RESPONSE,          ///< The response is invalid.
    SDI12_COMMAND_ERROR_BUFFER_TOO_SMALL,          ///< The buffer is too small.
    SDI12_COMMAND_ERROR_INVALID_ADDRESS,           ///< The address is an invalid one.

    SDI12_COMMAND_ERROR_SPECIFIC = 50     ///< The first value that command specific error codes can use.
  }
  SDI12CommandError;

  /**
   * Defines the command's configuration flags.
   */
  typedef enum SDI12CommandConfigFlags
  {
    SDI12_CMD_CONFIG_NONE                            = 0x00,
    SDI12_CMD_CONFIG_SAME_ADDRESSES                  = 0x01, ///< Command and response addresses must be the same.
    SDI12_CMD_CONFIG_ADDRESSES_CAN_BE_DIFFERENT      = 0x00, ///< Command and response addresses can be different.
    SDI12_CMD_CONFIG_SEND_BREAK                      = 0x02, ///< Send a break on the bus before the command.
    SDI12_CMD_CONFIG_DO_NOT_SEND_BREAK               = 0x00, ///< Do not send a break on the bus before the command.
    SDI12_CMD_CONFIG_RESPONSE_HAS_CRC                = 0x04, ///< The command's response include a CRC.
    SDI12_CMD_CONFIG_RESPONSE_DOES_NOT_HAVE_CRC      = 0x00, ///< The command's response does not include a CRC.
    SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST    = 0x08, ///< Following this command a service request can be emitted.
    SDI12_CMD_CONFIG_CANNOT_GENERATE_SERVICE_REQUEST = 0x00, ///< No service request can result from this command.
    SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN           = 0x10  ///< Communication follow the Send Data command pattern.
  }
  SDI12CommandConfigFlags;
#define sdi12_command_cfg_same_addresses(              p_cmd) ((p_cmd)->config & SDI12_CMD_CONFIG_SAME_ADDRESSES)
#define sdi12_command_cfg_send_break(                  p_cmd) ((p_cmd)->config & SDI12_CMD_CONFIG_SEND_BREAK)
#define sdi12_command_cfg_response_has_crc(            p_cmd) ((p_cmd)->config & SDI12_CMD_CONFIG_RESPONSE_HAS_CRC)
#define sdi12_command_cfg_can_generate_service_request(p_cmd) ((p_cmd)->config & SDI12_CMD_CONFIG_CAN_GENERATE_SERVICE_REQUEST)
#define sdi12_command_cfg_use_send_data_pattern(       p_cmd) ((p_cmd)->config & SDI12_CMD_CONFIG_USE_SEND_DATA_PATTERN)


  /**
   * Represent an SDI-12 command.
   * Cannot be used directly, needs to be "extended".
   */
  struct SDI12Command
  {
    char     address;        ///< The sensor's address
    uint8_t *pu8_buffer;     ///< The buffer used to send and receive data. MUST be big enough.
    uint8_t  buffer_size;    ///< The buffer's size.
    uint8_t *pu8_cmd;        ///< The command to send/that has been sent. Is '\0' terminated so that we can use it as a string.
    uint16_t data_len;       ///< The number of data bytes in the buffer.
    uint8_t  error;          ///< The error code.
    uint8_t  config;         ///< The command configuration. A ORed combination of <code>SDI12CommandConfigFlags</code> values.

    /**
     * A context specific argument that makes only sense to the context using the object.
     * Not mandatory, Can be NULL.
     */
    void *pv_arg;

    /**
     * Function used to process a response to a command.
     * This function pointer can be NULL.
     *
     * @param[in] p_command the command to process. MUST be NOT NULL.
     *
     * @return true  if the response has been processed.
     * @return false if an error has been found.
     */
    sdi12_command_resp_cb_t process_response;

    /**
     * Function used to signal that the command has been successfully executed.
     */
    sdi12_command_result_cb_t  signal_success;
    void                      *pv_signal_success_args;

    /**
     * Function used to signal that the command has failed.
     */
    sdi12_command_result_cb_t  signal_failed;
    void                      *pv_signal_failed_args;
  };
#define SDI12_COMMAND_PTR(po) ((SDI12Command *)(po))


  /**
   * The type used to get an iterator on values in a command response.
   */
  typedef struct SDI12CmdValuesIterator
  {
    uint8_t *pu8_start; ///< Iterator begin position.
    uint8_t *pu8_end;   ///< Iterator end position; byte after last valid byte.
    uint8_t *pu8_it;    ///< Iterator position >= pu8_start and <= pu8_end.
    uint8_t  nb_values; ///< The number of values to iterate over.
  }
  SDI12CmdValuesIterator;


  /**
   * List the value types.
   */
  typedef enum SDI12CmdValueType
  {
    SDI12_CMD_VALUE_TYPE_UNKNOWN,
    SDI12_CMD_VALUE_TYPE_INTEGER,
    SDI12_CMD_VALUE_TYPE_FLOAT
  }
  SDI12CmdValueType;

  /**
   * Used to store an SDI-12 value
   */
  typedef struct SDI12CmdValue
  {
    SDI12CmdValueType type;  ///< The value's type
    union
    {
      int32_t integer_value; ///< Where an integer value is stored.
      float   float_value;   ///< Where a floating point value is stored.
    }
    value;
  }
  SDI12CmdValue;


  extern void sdi12_command_init(SDI12Command             *p_command,
				 uint8_t                  *pu8_buffer,
				 uint8_t                   buffer_size,
				 uint8_t                   config,
				 sdi12_command_resp_cb_t   process_response_cb,
				 sdi12_command_result_cb_t success_cb,
				 void                     *pv_success_cbargs,
				 sdi12_command_result_cb_t failed_cb,
				 void                     *pv_failed_cbargs);


  extern SDI12Command *sdi12_command_set_command_data(SDI12Command *p_command,
						      char          addr,
						      char         *ps_cmd);

  extern bool sdi12_command_check_crc(const SDI12Command *p_command);

  extern bool     sdi12_cmd_get_iterator_on_values(           const SDI12Command     *p_cmd,
						              SDI12CmdValuesIterator *p_it);
  extern bool     sdi12_cmd_get_iterator_on_values_using_data(uint8_t                *pu8_buffer,
							      uint16_t                data_len,
							      bool                    has_crc,
  						              SDI12CmdValuesIterator *p_it);
  extern bool     sdi12_cmd_iterator_get_next_value(          SDI12CmdValuesIterator *p_it,
						              SDI12CmdValue          *p_value);
  extern uint8_t *sdi12_cmd_iterator_get_values_data_buffer(  SDI12CmdValuesIterator *p_it,
							      uint16_t               *p_len);


  /**
   * Get the address of the sensor the command has been sent to.
   *
   * @param[in] p_command the command. MUST be NOT NULL.
   *
   * @return the address.
   */
#define sdi12_command_get_address(p_command) \
    (SDI12_COMMAND_PTR(p_command)->address)

  /**
   * Indicate if a command contains a response or not.
   *
   * @param[in] p_command the command. MUST be NOT NULL.
   *
   * @return true  if the command contains a response.
   * @return false otherwise.
   */
#define sdi12_command_contains_response(p_command) \
    (SDI12_COMMAND_PTR(p_command)->error != SDI12_COMMAND_ERROR_UNDEFINED)

extern uint8_t *sdi12_command_response_data(const SDI12Command *pv_cmd, uint8_t *pu8_size);

  /**
   * Check that the addresses in the command and in the response are the same.
   *
   * @pre the response must have been received.
   *
   * @param[in] p_command the command and it's response. MUST be NOT NULL.
   */
#define sdi12_command_cmd_and_response_addresses_are_the_same(p_command) \
    (SDI12_COMMAND_PTR(p_command)->address == (char)SDI12_COMMAND_PTR(p_command)->pu8_buffer[0])

  extern bool sdi12_command_process_response_common(SDI12Command *p_command);


#ifdef __cplusplus
}
#endif
#endif /* __SDI_12_COMMANDS_SDI12COMMAND_H__ */
