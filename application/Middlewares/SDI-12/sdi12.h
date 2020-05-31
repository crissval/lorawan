/**
 * @file  sdi12.h
 * @brief Header file for the SDI-12 communication protocol
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __SDI12_H__
#define __SDI12_H__

#include <sdi12command.h>


#ifdef __cplusplus
extern "C" {
#endif



/**
 * The SDI-12 statuses
 */
typedef enum SDI12InterfaceStatus
{
  SDI12_STATUS_OK   = 0,  ///< Everything is OK
  SDI12_STATUS_BUSY = 1   ///< The SDI-12 interface is busy; a command is being executed.
}
SDI12InterfaceStatus;

/**
 * The SDI-12 interface states.
 */
typedef enum SDI12InterfaceState
{
  SDI12_STATE_UNDEFINED = 0,                ///< Not an actual state. Can be used to check if the interface has been initialised or not.
  SDI12_STATE_IDLE,                         ///< The interface is free to send a command.
  SDI12_STATE_SENDING_BREAK,                ///< The interface is sending a break.
  SDI12_STATE_CMD_BREAK_END,                ///< The command break end has been reached.
  SDI12_STATE_SENDING_COMMAND,              ///< The interface is sending a command payload.
  SDI12_STATE_COMMAND_SENT,                 ///< The command has been sent.
  SDI12_STATE_WAITING_FOR_RESPONSE,         ///< We are waiting for a response to our command.
  SDI12_STATE_GOT_RESPONSE,                 ///< We have received a response to our command.
  SDI12_STATE_RX_BUFFER_TOO_SMALL,          ///< The command buffer is too small for the response.
  SDI12_STATE_RESPONSE_TIMEOUT,             ///< The time allowed to get the response has elapsed.
  SDI12_STATE_WAITING_FOR_SERVICE_REQUEST,  ///< The interface is waiting for a service request from a sensor.
  SDI12_STATE_GOT_SERVICE_REQUEST,          ///< We have received a service request.
  SDI12_STATE_SRVREQ_RX_BUFFER_TOO_SMALL,   ///< The service request reception buffer is too small.
  SDI12_STATE_SRVREQ_TIMEOUT,               ///< The time allowed to get a service request has elapsed.
  SDI12_STATE_ABORTING_MEASUREMENT,         ///< Sending a break on the bus to abort the current measurement.
  SDI12_STATE_ABORTM_BREAK_END              ///< Reached the end of the break used to abort the current measurement.
}
SDI12InterfaceState;


/**
 * The context used to filter response data when red from the UART.
 */
typedef struct SDI12RxFilterCtx
{
  uint8_t last_byte;  ///< Memorises the last byte.
}
SDI12RxFilterCtx;


/**
 * Defines the software interface used to communicate with the SDI PHY interface (an UART).
 */
struct SDI12PHY;
typedef struct SDI12PHY SDI12PHY;
struct SDI12PHY
{
  /**
   * Initialises the PHY interface.
   *
   * UART has to be configured with these parameters:
   * - 1 start bit
   * - 7 data bits, least significant bit transmitted first
   * - 1 parity bit, even parity
   * - 1 stop bit
   *
   * @param[in] p_phy the physical interface to initalise. MUST be NOT NULL.
   */
  void (* init)(SDI12PHY *p_phy);

  /**
   * Animate the PHY interface's state machine, if there is one.
   *
   * This function pointer can be NULL if there is nothing to animate.
   *
   * @param[in] p_phy the physical interface. MUST be NOT NULL.
   *
   * @return true  if there is more processing to do.
   * @return false otherwise.
   */
  bool (* process)(SDI12PHY *p_phy);

  /**
   * Set the TX line to the break level.
   *
   * @param[in] p_phy     the physical interface. MUST be NOT NULL.
   * @param[in] cb        the function to call at the end of the break.  Can be NULL.
   * @param[in] pv_cbargs the argument to pass to the callback function. Can be NULL.
   */
  void (* set_tx_to_break_level)(SDI12PHY *p_phy, void (*cb)(void *pv_args), void *pv_cbargs);

  /**
   * Send data.
   *
   * @note Before sending the data, the PHY must switch to TX mode.
   * @post The PHY remains in TX mode.
   *
   * @param[in] p_phy     the physical interface. MUST be NOT NULL.
   * @param[in] pu8_data  the data buffer. CANNOT be NULL.
   * @param[in] len       the data buffer length.
   * @param[in] cb        the function to call when all the data have been sent. CANNOT be NULL.
   * @param[in] pv_cbargs the value to pass to the callback function. Can be NULL.
   */
  void (* send_data)(SDI12PHY *p_phy,
                     uint8_t  *pu8_data,        uint8_t len,
		     void (*cb)(void *pv_args), void   *pv_cbargs);

  /**
   * Receive the data from the PHY interface byte after byte.
   *
   * @note Switch the PHY to RX mode before starting to receive.
   * @post The PHY is still in RX mode.
   *
   * @param[in] p_phy      the physical interface. MUST be NOT NULL.
   * @param[in] b          the data byte received.
   * @param[in] cb         the function to call each time a byte is received. CANNOT be NULL.
   *                       Continue receiving data if the function returns true and
   *                       stop receiving if it returns false.
   * @param[in] pv_cbargs  the value passed to the callback function. Can be NULL.
   * @param[in] timeout_ms the maximum time, in seconds, to get all the data we need.
   *                       A value of 0 does not disable the timeout.
   *
   * @return true  on success.
   * @return false on timeout.
   * @return false on error.
   */
  bool (* receive_byte_by_byte)(
      SDI12PHY *p_phy,
      bool    (*cb)(uint8_t b, void *pv_args),
      void     *pv_cbargs,
      uint32_t  timeout_ms);

  /**
   * Stop receiving.
   *
   * @post The PHY interface is configured in RX mode.
   *
   * @param[in] p_phy the physical interface. MUST be NOT NULL.
   */
  void (* stop_receiving)(SDI12PHY *p_phy);
};


struct SDI12Interface;
typedef struct SDI12Interface SDI12Interface;

#define SDI12_SERVICE_REQUEST_LEN  SDI12_ADDRESS_LEN + SDI12_COMMAND_END_OF_RESPONSE_LEN

/**
 * Type for a generic SDI-12 callback.
 */
typedef void (*SDI12Callback)(SDI12Interface *p_i, void *pv_cbargs);

/**
 * The type of the function to call after a service request has been received.
 */
typedef void (*SD12SrvReqCallback)(SDI12Interface *p_i, char addr, void *pv_cbargs);

/**
 * Used to hold service request data.
 */
  typedef struct SDI12ServiceRequest
  {
    char    address;                               ///< The address of the sensor we are expecting.
    uint8_t pu8_buffer[SDI12_SERVICE_REQUEST_LEN]; ///< The reception buffer.
    uint8_t data_len;                              ///< The amount of data received.

    SD12SrvReqCallback callback;  ///< The function to call at reception or at timeout. Can be NULL.
    void              *pv_cbargs; ///< The argument passed to the callback function. Can be NULL.
  }
  SDI12ServiceRequest;
#define sdi12_set_no_service_request(p_sr)  (p_sr)->pu8_buffer[0] = '\0'
#define sdi12_is_a_service_request(  p_sr)  sdi12_is_address_valid((p_sr)->pu8_buffer[0])

#include "board.h"

/**
 * Describes an SDI interface
 */
struct SDI12Interface
{
  SDI12PHY           *p_phy;             ///< The PHY (the UART used).
  uint8_t             state;             ///< The interface's state.
  uint32_t            timeout_ms;        ///< The current timeout value, in milliseconds.
  uint32_t            ts_ms;             ///< The current timestamp value, in milliseconds.
  SDI12Command       *p_current_command; ///< The current command. NULL if no command is currently being executed.
  SDI12RxFilterCtx    rx_filter_ctx;     ///< The UART RX filter context.

  SDI12ServiceRequest service_request; ///< Store service request.

  SDI12Callback       callback;  ///< A generic, multi-purpose, SDI-12 interface callback. Can be NULL.
  void               *pv_cbargs; ///< The arguments to pass to the generic callback. Can be NULL.

#if defined SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS && SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS > 0
  uint32_t            last_command_timestamp;  ///< Timestamp of end of execution of the last command.
#endif
};
#define SDI12_INTERFACE_PTR(po)  ((SDI12Interface *)(po))


extern bool sdi12_is_address_valid(char addr);

extern void sdi12_init(   SDI12Interface *p_interface, SDI12PHY *p_phy);
extern bool sdi12_process(SDI12Interface *p_interface);
extern bool sdi12_is_busy(SDI12Interface *p_interface);

extern bool sdi12_send_command(SDI12Interface *p_interface,
			       SDI12Command   *p_command,
			       uint32_t        timeout_ms);

extern bool sdi12_wait_for_service_request(SDI12Interface    *p_interface,
					   char               address,
					   uint32_t           timeout_ms,
					   SD12SrvReqCallback cb,
					   void               *pv_cbargs);

extern void sdi12_abort_measurement(SDI12Interface *p_interface,
				    SDI12Callback   cb,
				    void           *pv_cbargs);


#ifdef __cplusplus
}
#endif
#endif /* __SDI12_H__ */
