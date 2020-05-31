/**
 * @file  sdi12.c
 * @brief Implementation of the SDI-12 communication protocol
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "sdi12.h"

#ifdef __cplusplus
extern "C" {
#endif


#if SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS < 0
#error SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS cannot be < 0
#elif SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS > 5000
#warning Are you sure you want SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS to be greater than 5000 ms?
#endif


  static void sdi12_go_to_idle(                    SDI12Interface *p_interface);
  static void sdi12_reset_cmd_context(             SDI12Interface *p_interface);
  static bool sdi12_retry_wait_for_service_request(SDI12Interface *p_interface);

  static void sdi12_cmd_break_end(                 void   *p_i);
  static void sdi12_cmd_data_sent(                 void   *p_i);
  static bool sdi12_cmdresponse_reception_callback(uint8_t b, void *p_i);

  static bool sdi12_srvreq_reception_callback(uint8_t b, void *p_i);

  static void sdi12_abort_break_end(void *p_i);


  /**
   * Indicate if an address is valid or not.
   *
   * @param[in] addr the address to check.
   *
   * @return true  if it is valid.
   * @return false otherwise.
   */
  bool sdi12_is_address_valid(char addr)
  {
    return ('0' <= addr && addr <= '9') ||
	   ('A' <= addr && addr <= 'Z') ||
	   ('a' <= addr && addr <= 'z') ||
	   addr == '?';
  }


  /**
   * Initialises an SDI interface.
   *
   * @param[in] p_interface the SDI interface to use. MUST be NOT NULL.
   * @param[in] p_phy       the PHY interface (the UART used by the SDI-12 interface). MUST be NOT NULL.
   */
  void sdi12_init(SDI12Interface *p_interface, SDI12PHY *p_phy)
  {
    // Initialise the PHY
    p_phy->init(p_phy);
    p_interface->p_phy = p_phy;

    // Go to idle state
    sdi12_go_to_idle(p_interface);
  }

  /**
   * Resets the command context.
   *
   * @param[in] p_interface the interface. MUST be NOT NULL.
   */
  static void sdi12_reset_cmd_context(SDI12Interface *p_interface)
  {
    p_interface->p_current_command        = NULL;
    p_interface->timeout_ms               = 0;
    p_interface->ts_ms                    = 0;
    p_interface->rx_filter_ctx.last_byte  = 0;
    p_interface->service_request.data_len = 0;
    p_interface->callback                 = NULL;
    p_interface->pv_cbargs                = NULL;
    sdi12_set_no_service_request(&p_interface->service_request);
  }


  /**
   * Return to the idle state.
   *
   * @param[in] p_interface the interface. MUST be NOT NULL.
   */
  static void sdi12_go_to_idle(SDI12Interface *p_interface)
  {
    // Stop receiving.
    p_interface->p_phy->stop_receiving(p_interface->p_phy);

    // We are ready to process another command
    p_interface->state = SDI12_STATE_IDLE;

#if defined SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS && SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS > 0
    p_interface->last_command_timestamp = HAL_GetTick();
#endif
  }

  /**
   * Indicate if a SDI-12 interface is busy or not.
   *
   * @param[in] p_interface the interface to check. MUST be NOT NULL.
   *
   * @return true  if the interface is busy.
   * @return false otherwise.
   */
  bool sdi12_is_busy(SDI12Interface *p_interface)
  {
    return p_interface->state != SDI12_STATE_IDLE;
  }


  /**
   * Run the SDI-12 operations that are pending; run the protocol automaton.
   *
   * @param[in] p_interface the interface. MUST be NOT NULL.
   *
   * @return true  if something has been done.
   * @return false if there was nothing to do.
   */
  bool sdi12_process(SDI12Interface *p_interface)
  {
    uint16_t len;
    bool res = false;

    if(p_interface->p_phy->process) { res |= p_interface->p_phy->process(p_interface->p_phy); }

    switch(p_interface->state)
    {
      case SDI12_STATE_IDLE:
      case SDI12_STATE_SENDING_BREAK:
      case SDI12_STATE_SENDING_COMMAND:
      case SDI12_STATE_WAITING_FOR_RESPONSE:
      case SDI12_STATE_WAITING_FOR_SERVICE_REQUEST:
      case SDI12_STATE_ABORTING_MEASUREMENT:
	// Nothing to do.
	break;

      case SDI12_STATE_CMD_BREAK_END:
	// The break has been sent, it's time to send the actual command
	p_interface->state = SDI12_STATE_SENDING_COMMAND;
	p_interface->p_phy->send_data(p_interface->p_phy,
				      p_interface->p_current_command->pu8_buffer,
				      p_interface->p_current_command->data_len,
				      sdi12_cmd_data_sent,
				      p_interface);
	res = true;
	break;

      case SDI12_STATE_COMMAND_SENT:
	// Switch to reception to get the response.
	p_interface->state = SDI12_STATE_WAITING_FOR_RESPONSE;

	// Set up the reception buffer so that to keep the command that has been sent.
	// We have to move the pointer of data_len + 1;
	// the +1 is to keep the '\0' character added at the end of the command sent.
	len                                          = p_interface->p_current_command->data_len + 1;
	p_interface->p_current_command->pu8_buffer  += len;
	p_interface->p_current_command->buffer_size -= len;
	p_interface->p_current_command->data_len     = 0;

	// Start getting response data bytes.
	p_interface->rx_filter_ctx.last_byte     = 0;
	if(!p_interface->p_phy->receive_byte_by_byte(p_interface->p_phy,
						     sdi12_cmdresponse_reception_callback, p_interface,
						     p_interface->timeout_ms))
	{
	  p_interface->state = SDI12_STATE_RESPONSE_TIMEOUT;
	}

	res = true;
	break;

      case SDI12_STATE_GOT_RESPONSE:
	// We have our response.
	// Add a '\0' ending character so that the response also is a valid string.
	// but do not increase the data length.
	p_interface->p_current_command->pu8_buffer[p_interface->p_current_command->data_len] = '\0';

	// Go to IDLE state
	sdi12_go_to_idle(p_interface);

	// Process response
	if(sdi12_command_process_response_common(p_interface->p_current_command) &&
	    (!p_interface->p_current_command->process_response ||
	      p_interface->p_current_command->process_response(p_interface->p_current_command)))
	{
	  // Response has been successfully processed.
	  sdi12_command_signal_success(p_interface->p_current_command);
	}
	else
	{
	  // Something went wrong while processing the response
	  sdi12_command_signal_failed(  p_interface->p_current_command,
				      ((p_interface->p_current_command->error !=
					  SDI12_COMMAND_ERROR_UNDEFINED) ?
					      p_interface->p_current_command->error :
					      SDI12_COMMAND_ERROR_INVALID_RESPONSE));
	}

	res = true;
	break;

      case SDI12_STATE_RX_BUFFER_TOO_SMALL:
	// We were receiving the response but the reception buffer is too small.
	// Go to IDLE state
	sdi12_go_to_idle(p_interface);

	// Command has failed
	sdi12_command_signal_failed(p_interface->p_current_command,
				    SDI12_COMMAND_ERROR_BUFFER_TOO_SMALL);

	res = true;
	break;

      case SDI12_STATE_RESPONSE_TIMEOUT:
	// The reception has timed out.
	// Go to idle state
	sdi12_go_to_idle(p_interface);

	// Command has failed
	sdi12_command_signal_failed(p_interface->p_current_command,
				    SDI12_COMMAND_ERROR_TIMEOUT);

	res = true;
	break;

      case SDI12_STATE_GOT_SERVICE_REQUEST:
	// We have received a service request
	// Check that it is coming the expected address.
	if(p_interface->service_request.address == (char)p_interface->service_request.pu8_buffer[0])
	{
	  // Add a '\0' ending character so that the service request also is a valid string.
	  p_interface->service_request.pu8_buffer[SDI12_SERVICE_REQUEST_LEN - 1] = '\0';

	  // Go to IDLE state
	  sdi12_go_to_idle(p_interface);

	  // Call callback function if there is one
	  if(p_interface->service_request.callback)
	  {
	    p_interface->service_request.callback(p_interface,
						  p_interface->service_request.address,
						  p_interface->service_request.pv_cbargs);
	  }
	}
	else
	{
	  // Not the sensor we were expecting.
	  // Discard this service request and wait for the next one
	  sdi12_retry_wait_for_service_request(p_interface);
	}
	res = true;
	break;

      case SDI12_STATE_SRVREQ_RX_BUFFER_TOO_SMALL:
	// We were waiting for a service request but the reception buffer is too small.
	// Discard this service request and wait for the next one
	sdi12_retry_wait_for_service_request(p_interface);
	res = true;
	break;

      case SDI12_STATE_SRVREQ_TIMEOUT:
	// We were waiting for a service request but the time allowed to receive it has elapsed.
	// Go to IDLE state
	sdi12_go_to_idle(p_interface);

	// Signal error if a callback is set
	sdi12_set_no_service_request(&p_interface->service_request);
	if(p_interface->service_request.callback)
	{
	  p_interface->service_request.callback(p_interface,
						'\0',
						p_interface->service_request.pv_cbargs);
	}

	res = true;
	break;

      case SDI12_STATE_ABORTM_BREAK_END:
	// We reached the end of the break used to abort current measurement.
	// Go to IDLE state
	sdi12_go_to_idle(p_interface);

	// Call callback if one is set
	if(p_interface->callback) { p_interface->callback(p_interface, p_interface->pv_cbargs); }
	res = true;
	break;

      default:
	/* We should never be here. */
	fatal_error_with_msg("Unknown SDI-12 interface state: %u", p_interface->state);
    }

    return res;
  }


  //=========================== Sending command ========================================

  /**
   * Send a command on the SDI-12 bus.
   *
   * @param[in]  p_interface the SDI interface to use; MUST be NOT NULL.
   * @param[in]  p_command   the command to send. MUST be NOT NULL.
   *                         MUST have been initialised with the command payload.
   * @param[in]  timeout_ms  the maximum time, in milliseconds, allowed for the command.
   *
   * @return true  if the command is being sent on the bus.
   * @return false if the interface is busy.
   *               The command error status is set to SDI12_COMMAND_ERROR_BUSY.
   *               The command failed callback is not called.
   */
  bool sdi12_send_command(SDI12Interface *p_interface,
			  SDI12Command   *p_command,
			  uint32_t        timeout_ms)
  {
#if defined SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS && SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS > 0
    uint32_t ts;
#endif

    // Check that the bus is not busy.
    if(p_interface->state != SDI12_STATE_IDLE)
    {
      p_command->error = SDI12_COMMAND_ERROR_BUSY;
      return false;
    }

#if defined SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS && SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS > 0
    do
    {
      ts = HAL_GetTick();
      ts = (ts >= p_interface->last_command_timestamp ?
	  ts    - p_interface->last_command_timestamp :
	  UINT32_MAX - p_interface->last_command_timestamp + ts);
    }
    while(ts < SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS);
#endif

    // Resets the context
    sdi12_reset_cmd_context(p_interface);

    if(sdi12_command_cfg_send_break(p_command))
    {
      // Before the command can be sent, we must send a break
      p_interface->state = SDI12_STATE_SENDING_BREAK;

      // Set the current command
      p_interface->p_current_command = p_command;
      p_interface->timeout_ms        = timeout_ms;

      // Use the timer to handle break duration.
      // At the end of the break duration, the callback function will be called
      p_interface->p_phy->set_tx_to_break_level(p_interface->p_phy,
						sdi12_cmd_break_end, p_interface);
    }
    else
    {
      // Do not send a break before the command.
      // Directly send the command.
      p_interface->state = SDI12_STATE_CMD_BREAK_END;

      // Set the current command
      p_interface->p_current_command = p_command;
      p_interface->timeout_ms        = timeout_ms;

      // Re-use the code that is found in the process() function. Simulating a break's end.
      sdi12_process(p_interface);
    }

    return true;
  }

  /**
   * Function called when the break duration for a command has been reached.
   *
   * @param[in] p_i the SDI interface to use to process the data. IS NOT NULL.
   */
  static void sdi12_cmd_break_end(void *p_i)
  {
    SDI12Interface *p_interface = SDI12_INTERFACE_PTR(p_i);

    // Change state and process what comes next
    p_interface->state = SDI12_STATE_CMD_BREAK_END;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_CMD_BREAK_END
    sdi12_process(p_interface);
#endif
  }

  /**
   * Function called when the data have been sent by the UART
   *
   * @param[in] p_i the SDI-12 interface using the UART. MUST be NOT NULL.
   */
  static void sdi12_cmd_data_sent(void *p_i)
  {
    SDI12Interface *p_interface = SDI12_INTERFACE_PTR(p_i);

    // Change state and process what comes next
    p_interface->state = SDI12_STATE_COMMAND_SENT;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_CMD_DATA_SENT
    sdi12_process(p_interface);
#endif
  }

  /**
   * Function called when the UART has received data and we are listening for a command response.
   *
   * @param[in] b   the last byte received.
   * @param[in] p_i the SDI interface to use to process the data. IS NOT NULL.
   *
   * @return true  if the  the UART should continue to receive.
   * @return false otherwise.
   */
  static bool sdi12_cmdresponse_reception_callback(uint8_t b, void *p_i)
  {
    SDI12Interface *p_interface = SDI12_INTERFACE_PTR(p_i);
    SDI12Command   *p_command   = p_interface->p_current_command;

    if(p_interface->state != SDI12_STATE_WAITING_FOR_RESPONSE)
    {
      fatal_error_with_msg("SDI-12 received command response data byte when expecting none.");
    }

    // Check that the reception buffer can take in the new byte
    if(p_command->data_len >= p_command->buffer_size)
    {
      // The buffer is full. Process what comes next.
      p_interface->state = SDI12_STATE_RX_BUFFER_TOO_SMALL;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_CMD_RECEPTION
      sdi12_process(p_interface);
#endif
      return false; // Stop receiving
    }

    // Copy the last data to the buffer
    p_command->pu8_buffer[p_command->data_len++] = b;

    // Look for end of response pattern
    if(b == '\n' && p_interface->rx_filter_ctx.last_byte == '\r')
    {
      // End of response found. Process what comes next.
      p_interface->state = SDI12_STATE_GOT_RESPONSE;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_CMD_RECEPTION
      sdi12_process(p_interface);
#endif
      return false; // Stop receiving
    }
    else
    {
      // Not end of response; memorise current data byte for next loop
      p_interface->rx_filter_ctx.last_byte = b;
    }

    return true;
  }


  //=========================== Waiting for service request ========================================

  /**
   * Listen to the SDI-12 bus, waiting for a service request to come.
   *
   * @param[in]  p_interface the SDI interface to use; MUST be NOT NULL.
   * @param[in]  address     the address of the sensor we expect a service request from.
   * @param[in]  timeout_ms  the maximum time, in milliseconds, to wait for the service request to come.
   * @param[in]  cb          the function to call when the service request has been received or on timeout.
   *                         On timeout, the address passed to the callback function is '\0'.
   *                         Can be NULL.
   * @param[in]  pv_cbargs   the argument passed to the callback function. Can be NULL.
   *
   * @return true  if we have received a service request.
   * @return false in case of timeout.
   * @return false in case of error.
   */
  bool sdi12_wait_for_service_request(SDI12Interface    *p_interface,
				      char               address,
				      uint32_t           timeout_ms,
				      SD12SrvReqCallback cb,
				      void              *pv_cbargs)
  {
    bool ok;

    // Check that the bus is not busy.
    if(p_interface->state != SDI12_STATE_IDLE) { return false; }

    // Resets the context
    sdi12_reset_cmd_context(p_interface);

    // Set the state
    p_interface->state = SDI12_STATE_WAITING_FOR_SERVICE_REQUEST;

    // Initialise the service request object
    sdi12_set_no_service_request(&p_interface->service_request);
    p_interface->service_request.address    = address;
    p_interface->service_request.data_len   = 0;
    p_interface->service_request.callback   = cb;
    p_interface->service_request.pv_cbargs  = pv_cbargs;
    p_interface->rx_filter_ctx.last_byte    = 0;

    // Start receiving
    p_interface->timeout_ms = timeout_ms;
    p_interface->ts_ms      = board_ms_now();
    if(!(ok = p_interface->p_phy->receive_byte_by_byte(p_interface->p_phy,
						       sdi12_srvreq_reception_callback, p_interface,
						       timeout_ms)))
    {
      p_interface->state = SDI12_STATE_SRVREQ_TIMEOUT;
    }

    return ok;
  }

  /**
   * Retry waiting for a service request.
   *
   * We call this function if we were previously already waiting for a service request,
   * one came, but was not the one we were expecting.
   * This function reset the reception variables, but keep the timeout timer running.
   *
   * @param[in]  p_interface the SDI interface to use; MUST be NOT NULL.
   *
   * @return true  if we have received a service request.
   * @return false in case of timeout.
   */
  static bool sdi12_retry_wait_for_service_request(SDI12Interface *p_interface)
  {
    bool ok;

    // Stop receiving while we reset our variables.
    p_interface->p_phy->stop_receiving(p_interface->p_phy);

    // Reset the variables for next service request to come. Keep timeout timer running.
    p_interface->state                    = SDI12_STATE_WAITING_FOR_SERVICE_REQUEST;
    sdi12_set_no_service_request(&p_interface->service_request);
    p_interface->service_request.data_len = 0;
    p_interface->rx_filter_ctx.last_byte  = 0;

    // Start receiving
    p_interface->timeout_ms -= board_ms_diff(p_interface->ts_ms, board_ms_now());
    if(!(ok = p_interface->p_phy->receive_byte_by_byte(p_interface->p_phy,
						       sdi12_srvreq_reception_callback, p_interface,
						       p_interface->timeout_ms)))
    {
      p_interface->state = SDI12_STATE_SRVREQ_TIMEOUT;
    }

    return ok;
  }


  /**
   * Function called when the UART has received data and we are listening for a service request.
   *
   * @param[in] b   the last byte received.
   * @param[in] p_i the SDI interface to use to process the data. IS NOT NULL.
   *
   * @return true  if the  the UART should continue to receive.
   * @return false otherwise.
   */
  static bool sdi12_srvreq_reception_callback(uint8_t b, void *p_i)
  {
    SDI12Interface      *p_interface       = SDI12_INTERFACE_PTR(p_i);
    SDI12ServiceRequest *p_service_request = &p_interface->service_request;

    if(p_interface->state != SDI12_STATE_WAITING_FOR_SERVICE_REQUEST)
    {
      fatal_error_with_msg("SDI-12 received service request data byte when expecting none.");
    }

    // Check that the reception buffer can take in the new byte
    if(p_service_request->data_len >= SDI12_SERVICE_REQUEST_LEN)
    {
      // The buffer is full. Process what comes next.
      p_interface->state = SDI12_STATE_SRVREQ_RX_BUFFER_TOO_SMALL;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_SRVREQ_RECEPTION
      sdi12_process(p_interface);
#endif
      return false; // Stop receiving
    }

    // Copy the last data to the buffer
    p_service_request->pu8_buffer[p_service_request->data_len++] = b;

    // Look for end of response pattern
    if(b == '\n' && p_interface->rx_filter_ctx.last_byte == '\r')
    {
      // End of service request found. Process what comes next.
      p_interface->state = SDI12_STATE_GOT_SERVICE_REQUEST;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_SRVREQ_RECEPTION
      sdi12_process(p_interface);
#endif
      return false; // Stop receiving
    }
    else
    {
      // Not end of response; memorise current data byte for next loop
      p_interface->rx_filter_ctx.last_byte = b;
    }

    return true;
  }


  //=========================== Aborting measurement ========================================

  /**
   * Send a break to abort the current measurement.
   *
   * @param[in] p_interface the SDI-12 interface. MUST be NOT NULL.
   * @param[in] cb          the function to call after the break has been sent to the bus. Can be NULL.
   * @param[in] pv_cbargs   the arguments passed to the callback function. Can be NULL.
   */
  void sdi12_abort_measurement(SDI12Interface *p_interface,
			       SDI12Callback   cb,
			       void           *pv_cbargs)
  {
    // Make sure that a measurement is running. That is we are in a service request waiting state
    if(p_interface->state != SDI12_STATE_WAITING_FOR_SERVICE_REQUEST) { return; }

    // Set state
    p_interface->state = SDI12_STATE_ABORTING_MEASUREMENT;

    // Set the callback
    p_interface->callback  = cb;
    p_interface->pv_cbargs = pv_cbargs;

    // Start sending a break
    p_interface->p_phy->set_tx_to_break_level(p_interface->p_phy,
					      sdi12_abort_break_end, p_interface);
  }

  /**
   * Called at the end of break used to abort a measurement.
   *
   * @param[in] p_i the SDI-12 interface. MUQT be NOT NULL.
   */
  static void sdi12_abort_break_end(void *p_i)
  {
    SDI12Interface *p_interface = SDI12_INTERFACE_PTR(p_i);

    // Change state and process what comes next
    p_interface->state = SDI12_STATE_ABORTM_BREAK_END;
#if !defined SDI12_DO_NOT_PROCESS_IN_CALLBACKS && \
    !defined SDI12_DO_NOT_PROCESS_IN_CALLBACK_ABORTM_BREAK_END
    sdi12_process(p_interface);
#endif
  }


#ifdef __cplusplus
}
#endif
