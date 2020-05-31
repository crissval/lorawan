/**
 * @file  sdi12command.c
 * @brief Implementation of the base class for all SDI-12 commands.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <sdi12command.h>
#include "sdi12.h"
#include "utils.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Initialise a command.
   *
   * @param[in] p_command           the command object to initialise.
   * @param[in] pu8_buffer          the buffer used to send data and to store the response.
   *                                MUST be NOT NULL and MUST be big enough for the response.
   * @param[in] buffer_size         the buffer's size. MUST be > 0.
   * @param[in] config              a ORed combination of <code>SDI12CommandConfigFlags</code> values.
   * @param[in] process_response_cb the callback function used to process responses.
   *                                Can be NULL.
   * @param[in] success_cb          the callback function called when the command has successfully been
   *                                executed.
   *                                Can be NULL.
   * @param[in] pv_success_cbargs   the arguments passed to the callback function <code>success_cb</code>.
   *                                Can be NULL.
   * @param[in] failed_cb           the callback function called when the command has failed.
   *                                Can be NULL.
   * @param[in] pv_failed_cbargs    the arguments passed to the callback function <code>failed_cb</code>.
   *                                Can be NULL.
   */
  void sdi12_command_init(SDI12Command             *p_command,
			  uint8_t                  *pu8_buffer,
			  uint8_t                   buffer_size,
			  uint8_t                   config,
			  sdi12_command_resp_cb_t   process_response_cb,
			  sdi12_command_result_cb_t success_cb,
			  void                     *pv_success_cbargs,
			  sdi12_command_result_cb_t failed_cb,
			  void                     *pv_failed_cbargs)
  {
    p_command->address                = '\0';
    p_command->pu8_buffer             = pu8_buffer;
    p_command->buffer_size            = buffer_size;
    p_command->pu8_cmd                = pu8_buffer;
    p_command->data_len               = 0;
    p_command->config                 = config;
    p_command->error                  = SDI12_COMMAND_ERROR_UNDEFINED;
    p_command->process_response       = process_response_cb;
    p_command->pv_arg                 = NULL;
    p_command->signal_success         = success_cb;
    p_command->pv_signal_success_args = pv_success_cbargs;
    p_command->signal_failed          = failed_cb;
    p_command->pv_signal_failed_args  = pv_failed_cbargs;
  }


  /**
   * Get the command data buffer to send.
   *
   * @post p_command->pu8_buffer contains the command data.
   * @post p_command->data_len   contains the number of data bytes.
   *
   * @param[in]  p_command the command object. MUST be NOT NULL.
   *                       MUST have been initialised.
   * @param[in]  addr      the address of the sensor we want to speak to.
   *                       Valid addresses are:
   *                       '0' <= addr <= '9' || 'A' <= addr <= 'Z' || 'a' <= addr <= 'z' || '?'.
   * @param[in]  ps_cmd    the command string, NULL terminated,
   *                       without address and exclamation point.
   *                       Can be NULL or empty (for the acknowledge active command).
   *
   * @return the updated command object.
   * @return NULL if the address is not valid.
   */
  SDI12Command *sdi12_command_set_command_data(SDI12Command *p_command,
					       char          addr,
					       char         *ps_cmd)
  {
    uint8_t  len    = 2;  // Minimum command length
    uint8_t *buffer = p_command->pu8_buffer;

    if(!sdi12_is_address_valid(addr)) { return NULL; }

    p_command->address = addr;
    *buffer++          = addr; // Set the address
    if(ps_cmd)                 // copy the command string, without the ending '\0'
    {
      for(; *ps_cmd; len++) { *buffer++ = *ps_cmd++; }
    }
    *buffer++ = '!';   // Set the command ending exclamation point
    *buffer   = '\0';  // Append '\0' character so that the command can also be used as a string.

    p_command->data_len = len;

    return p_command;
  }


  /**
   * common checks to perform on all responses.
   *
   * @param[in] p_command the command, with its response, to process.
   *
   * @return true  if the passes the common tests.
   * @return false otherwise.
   */
  bool sdi12_command_process_response_common(SDI12Command *p_command)
  {
    return (p_command->data_len >= SDI12_ADDRESS_LEN + SDI12_COMMAND_END_OF_RESPONSE_LEN) &&
	(sdi12_is_address_valid((char)p_command->pu8_buffer[0]))                          &&
	(!sdi12_command_cfg_same_addresses(p_command)   ||
	    sdi12_command_cmd_and_response_addresses_are_the_same(p_command)) &&
	(!sdi12_command_cfg_response_has_crc(p_command) || sdi12_command_check_crc(p_command));
  }


  /**
   * Check the CRC in a command response.
   *
   * @param[in] p_command the command, with its response. MUST be NOT NULL.
   *                      the response must contain a CRC.
   *
   * @return true  if the response and the CRC match.
   * @return false.
   */
  bool sdi12_command_check_crc(const SDI12Command *p_command)
  {
    uint8_t  *pu8, *pu8_crc;
    uint8_t  c1, c2, c3;
    uint8_t  i;
    uint16_t crc = 0;

    // Compute CRC
    pu8     =  p_command->pu8_buffer;
    pu8_crc = &p_command->pu8_buffer[p_command->data_len - SDI12_COMMAND_CRC_LEN - SDI12_COMMAND_END_OF_RESPONSE_LEN];
    for(; pu8 != pu8_crc; pu8++)
    {
      crc ^= *pu8;
      for(i = 1; i <= 8; i++)
      {
	if(crc & 0x0001)
	{
	  crc >>= 1;
	  crc  ^= 0xA001;
	}
	else
	{
	  crc >>= 1;
	}
      }
    }

    // Convert to ASCII
    c1 = 0x40 |  (crc >> 12);
    c2 = 0x40 | ((crc >> 6) & 0x3F);
    c3 = 0x40 |  (crc & 0x3F);

    return c1 == pu8_crc[0] && c2 == pu8_crc[1] && c3 == pu8_crc[2];
  }

  /**
   * Get the response's data.
   *
   * @param[in]  pv_cmd   the command. MUST be NOT NULL.
   * @param[out] pu8_size where the data size is written to.
   *                      the address and the <CR><LR> characters are not taken into account.
   *                      Can be NULL if we are not interested in this information.
   *
   * @return the data.
   * @return NULL if the command has no response.
   */
  uint8_t *sdi12_command_response_data(const SDI12Command *pv_cmd, uint8_t *pu8_size)
  {
    if(!sdi12_command_contains_response(pv_cmd)) { return NULL; }

    if(pu8_size) { *pu8_size = pv_cmd->data_len - SDI12_ADDRESS_LEN - SDI12_COMMAND_END_OF_RESPONSE_LEN; }
    return pv_cmd->pu8_buffer + SDI12_ADDRESS_LEN;
  }


  /**
   * Get an iterator on a command response's values.
   *
   * Values are detected by looking for a '+' or '-' character followed by digits or '.' only.
   * The iterator returned is set up on the first value.
   *
   * @param[in]  p_cmd the command and it's response. MUST be NOT NULL.
   * @param[out] p_it  the iterator to initialise. MUST be NOT NULL.
   *
   * @return true  if a least one value has been found and the iterator has been initialised to get them.
   * @return false otherwise. And all iterator values are set to NULL.
   */
  bool sdi12_cmd_get_iterator_on_values(const SDI12Command *p_cmd, SDI12CmdValuesIterator *p_it)
  {
    return sdi12_cmd_get_iterator_on_values_using_data(p_cmd->pu8_buffer,
						       p_cmd->data_len,
						       sdi12_command_cfg_response_has_crc(p_cmd),
						       p_it);
  }

  /**
   * Get an iterator on a command response's values.
   *
   * The data are directly provided through a buffer and not a command object.
   *
   * Values are detected by looking for a '+' or '-' character followed by digits or '.' only.
   * The iterator returned is set up on the first value.
   *
   * @param[in]  pu8_buffer the buffer with the response. MUST be NOT NULL.
   * @param[in]  data_len   the number of data bytes in the buffer.
   * @param[in]  has_crc    is there a CRC in the data buffer?
   * @param[out] p_it  the iterator to initialise. MUST be NOT NULL.
   *
   * @return true  if a least one value has been found and the iterator has been initialised to get them.
   * @return false otherwise. And all iterator values are set to NULL.
   */
  bool sdi12_cmd_get_iterator_on_values_using_data(uint8_t                *pu8_buffer,
						   uint16_t                data_len,
						   bool                    has_crc,
						   SDI12CmdValuesIterator *p_it)
  {
    uint8_t *pu8, *pu8_end, *pu8_s, *pu8_e;
    uint8_t  nb_values, nb_digits, nb_points;
    uint8_t  values_len   = SDI12_ADDRESS_LEN + SDI12_COMMAND_END_OF_RESPONSE_LEN;

    if(has_crc) { values_len += SDI12_COMMAND_CRC_LEN; }
    values_len =  data_len - values_len;
    pu8        = &pu8_buffer[SDI12_ADDRESS_LEN];
    pu8_end    = &pu8_buffer[values_len + SDI12_ADDRESS_LEN];
    // We are looking for a string beginning with '+' or '-', only made of digits and at most one '.',
    // and long of at most
    for(p_it->pu8_start = pu8_s = pu8_e = NULL, nb_values = nb_digits = nb_points = 0; pu8 != pu8_end; pu8++)
    {
      if(!pu8_s)
      {
	// Look for the value's sign
	if(*pu8 == '+' || *pu8 == '-') { pu8_s = pu8; }
	else                           { continue;    }
      }
      else
      {
	if(char_is_digit(*pu8))
	{
	  nb_digits++;
	  if(nb_digits > SDI12_COMMAND_VALUE_NB_DIGITS_MAX)
	  {
	    // Too many digits
	    if(p_it->pu8_start) { break; }  // We already have at least one value, so stop here.
	    pu8_s     = NULL;
	    nb_digits = nb_points = 0;
	  }
	}
	else if(*pu8 == '.')
	{
	  nb_points++;
	  if(nb_points > 1)
	  {
	    // Too many decimal points
	    if(p_it->pu8_start) { break; } // We already have at least one value, so stop here.
	    pu8_s     = NULL;
	    nb_digits = nb_points = 0;
	  }
	}
	else
	{
	  if(nb_digits                 >= SDI12_COMMAND_VALUE_NB_DIGITS_MIN &&
	      ((uint8_t)(pu8 - pu8_s)) <= SDI12_COMMAND_VALUE_LEN_MAX)
	  {
	    // The value is valid
	    if(!p_it->pu8_start) { p_it->pu8_start = pu8_s; }
	    pu8_e = pu8;
	    nb_values++;
	  }
	  else
	  {
	    if(p_it->pu8_start) { break; } // We already have at least one value, so stop here.
	  }
	  if(*pu8 == '+' || *pu8 == '-')
	  {
	    // There may be another value after the one we just found
	    pu8_s     = NULL;
	    nb_digits = nb_points = 0;
	    pu8--;  // So that the current character is taken into account at the next loop; counter-balance the upcoming pu8++ from the for loop.
	  }
	  else { break; }  // No value is next.
	}
      }
    }
    if(pu8 == pu8_end && pu8_s && nb_digits >= SDI12_COMMAND_VALUE_NB_DIGITS_MIN &&
	((uint8_t)(pu8 - pu8_s))            <= SDI12_COMMAND_VALUE_LEN_MAX)
    {
      // The value is valid
      if(!p_it->pu8_start) { p_it->pu8_start = pu8_s; }
      pu8_e = pu8;
      nb_values++;
    }

    // Set up the iterator
    if(p_it->pu8_start)
    {
      p_it->pu8_end   = pu8_e;
      p_it->pu8_it    = p_it->pu8_start;
      p_it->nb_values = nb_values;
      return true;
    }

    // No value have been found
    p_it->pu8_start = p_it->pu8_end = p_it->pu8_it = NULL;
    p_it->nb_values = 0;
    return false;
  }


  /**
   * Get the next value from a values iterator.
   *
   * @param[in]  p_it    the iterator. MUST be NOT NULL and MUST have been initialised.
   * @param[out] p_value where the next value is written to. MUST be NOT NULL.
   *
   * @return true  if the value has been read and the iterator moved to the next value.
   * @return false if there are no more values to read from the iterator.
   */
  bool sdi12_cmd_iterator_get_next_value(SDI12CmdValuesIterator *p_it, SDI12CmdValue *p_value)
  {
    uint8_t  *pu8;
    int32_t   value = 0;
    uint32_t  div   = 0;

    p_value->type                = SDI12_CMD_VALUE_TYPE_UNKNOWN;
    p_value->value.integer_value = 0;

    if(!p_it->pu8_start || p_it->pu8_it >= p_it->pu8_end)
    {
      // Iterator empty or no more values to read.
      return false;
    }

    // Get the next sign character to find the end of the value
    for(pu8 = p_it->pu8_it + 1; pu8 != p_it->pu8_end && *pu8 != '+' && *pu8 != '-'; pu8++)
    {
      if(*pu8 == '.') { div  = 1; }
      else
      {
	value = value * 10 + (*pu8 - '0');
	div  *= 10;
      }
    }

    // Set up the value
    if(*p_it->pu8_it == '-') { value = -value; }
    if(div)
    {
      p_value->type                = SDI12_CMD_VALUE_TYPE_FLOAT;
      p_value->value.float_value   = ((float)value) / ((float)div);
    }
    else
    {
      p_value->type                = SDI12_CMD_VALUE_TYPE_INTEGER;
      p_value->value.integer_value = value;
    }

    // Update the iterator
    p_it->pu8_it = pu8;

    return true;
  }


  /**
   * Get the data buffer corresponding to the iterator range.
   *
   * @param[in]  p_it  the iterator. MUST be NOT NULL.
   * @param[out] p_len where the buffer's len is written to. MUST be NOT NULL.
   *                   Set to 0 if this function fails.
   *
   * @return the buffer's address.
   * @return NULL if the iterator is not initialised.
   */
  uint8_t *sdi12_cmd_iterator_get_values_data_buffer(SDI12CmdValuesIterator *p_it, uint16_t *p_len)
  {
    // Check that the iterator has been initialised.
    if(!p_it->pu8_start)
    {
      *p_len = 0;
      return NULL;
    }

    // Return the buffer
    *p_len = (uint16_t)(p_it->pu8_end - p_it->pu8_start);
    return p_it->pu8_start;
  }


#ifdef __cplusplus
}
#endif
