/*
 * Implements USART communication.
 *
 * @author: Jérôme FUCHET
 * @date:   2018
 */

#ifndef PERIPHERALS_USART_H_
#define PERIPHERALS_USART_H_

#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Define the USART identifiers.
   */
  typedef enum USARTId
  {
    USART_ID_EXT,        ///< The USART from the extension port.
    USART_ID_SIGFOX,     ///< The USART for the Sigfox interface.
#ifdef A2135_USART_ID
    USART_ID_GPS_A2135,  ///< The USART used to communicate with A2135 GPS module.
#endif
    USART_ID_COUNT       ///< Not an actual identifier; used to count them.
  }
  USARTId;


#define USART_PARAM_WORD_LEN_POS   0
#define USART_PARAM_STOP_BITS_POS  2
#define USART_PARAM_PARITY_POS     5
#define USART_PARAM_MODE_POS       7
#define USART_PARAM_ASLEEP_POS     9
  /**
   * Define the UART configuration parameters.
   */
  typedef enum USARTParam
  {
    USART_PARAM_NONE             = 0,
    USART_PARAM_WORD_LEN_7BITS   = 1u << USART_PARAM_WORD_LEN_POS,
    USART_PARAM_WORD_LEN_8BITS   = 2u << USART_PARAM_WORD_LEN_POS,
    USART_PARAM_WORD_LEN_9BITS   = 3u << USART_PARAM_WORD_LEN_POS,
    USART_PARAM_STOP_BITS_0_5    = 1u << USART_PARAM_STOP_BITS_POS,
    USART_PARAM_STOP_BITS_1      = 2u << USART_PARAM_STOP_BITS_POS,
    USART_PARAM_STOP_BITS_1_5    = 3u << USART_PARAM_STOP_BITS_POS,
    USART_PARAM_STOP_BITS_2      = 4u << USART_PARAM_STOP_BITS_POS,
    USART_PARAM_PARITY_NONE      = 0,
    USART_PARAM_PARITY_EVEN      = 1u << USART_PARAM_PARITY_POS,
    USART_PARAM_PARITY_ODD       = 2u << USART_PARAM_PARITY_POS,
    USART_PARAM_MODE_RX          = 1u << USART_PARAM_MODE_POS,
    USART_PARAM_MODE_TX          = 2u << USART_PARAM_MODE_POS,
    USART_PARAM_MODE_RX_TX       = USART_PARAM_MODE_RX | USART_PARAM_MODE_TX,
    USART_PARAM_RX_WHEN_ASLEEP   = 1u << USART_PARAM_ASLEEP_POS
  }
  USARTParam;
  typedef uint16_t USARTParams;
#define USART_PARAM_WORD_LEN_MASK   (3u << USART_PARAM_WORD_LEN_POS)
#define USART_PARAM_STOP_BITS_MASK  (7u << USART_PARAM_STOP_BITS_POS)
#define USART_PARAM_PARITY_MASK     (3u << USART_PARAM_PARITY_POS)
#define USART_PARAM_MODE_MASK       (3u << USART_PARAM_MODE_POS)


  struct USART;
  typedef struct USART USART;


  /**
   * The type of the callback functions used in interruption reception.
   *
   * @param[out] pv_usart the USART that received the data. Is NOT NULL.
   * @param[out] pu8_data the data received by the USART. Is NOT NULL.
   * @param[out] size     the amount of data in pu8_data, in bytes. Including ending null character if there is one.
   * @param[out] pv_args  argument set at callback's registration. Can be NULL.
   */
  typedef void (*USARTItRxCallback)(USART *pv_usart, uint8_t *pu8_data, uint32_t size, void *pv_args);


  extern USART *usart_get_by_id(  USARTId     id);
  extern USART *usart_get_by_name(const char *ps_name);

  extern bool usart_open( USART      *pv_usart,
			  const char *ps_user_name,
			  uint32_t    baudrate,
			  USARTParams params);
  extern void usart_close(USART *pv_usart);

  extern void usart_sleep( USART *pv_usart);
  extern void usart_wakeup(USART *pv_usart);

  extern const char *usart_name(     USART *pv_usart);
  extern bool        usart_is_opened(USART *pv_usart);

  extern void usart_stop_it_read(USART *pv_usart);

  extern bool usart_read(   USART   *pv_usart,
			    uint8_t *pu8_buffer,
			    uint16_t size,
			    uint32_t timeout_ms);
  extern void usart_read_it(USART            *pv_usart,
			    uint8_t          *pu8_buffer,
			    uint32_t          size,
			    bool              continuous,
			    USARTItRxCallback pf_cb,
			    void             *pv_cb_args);

  extern bool usart_read_from(   USART   *pv_usart,
				 uint8_t *pu8_buffer,
				 uint16_t len,
				 uint8_t  from,
				 bool     copy_from_byte,
				 uint32_t timeout_ms);
  extern void usart_read_from_it(USART            *pv_usart,
				 uint8_t          *pu8_buffer,
				 uint32_t          len,
				 uint8_t           from,
				 bool              copy_from_byte,
				 bool              continuous,
				 USARTItRxCallback pf_cb,
				 void             *pv_cb_args);

  extern uint16_t usart_read_to(   USART   *pv_usart,
				   uint8_t *pu8_buffer,
				   uint16_t size,
				   uint8_t  to,
				   bool     copy_to_byte,
				   uint32_t timeout_ms);
  extern void     usart_read_to_it(USART            *pv_usart,
				   uint8_t          *pu8_buffer,
				   uint32_t          size,
				   uint8_t           to,
				   bool              copy_to_byte,
				   bool              continuous,
				   USARTItRxCallback pf_cb,
				   void             *pv_cb_args);

  extern uint16_t usart_read_from_to(   USART   *pv_usart,
					uint8_t *pu8_buffer,
					uint16_t size,
					uint8_t  from,
					uint8_t  to,
					bool     copy_boundaries,
					uint32_t timeout_ms);
  extern void     usart_read_from_to_it(USART            *pv_usart,
					uint8_t          *pu8_buffer,
					uint16_t          size,
					uint8_t           from,
					uint8_t           to,
					bool              copy_boundaries,
					bool              continuous,
					USARTItRxCallback pf_cb,
					void             *pv_cb_args);

  extern bool usart_write(       USART         *pv_usart,
				 const uint8_t *pu8_data,
				 uint16_t       size,
				 uint32_t       timeout_ms);
  extern bool usart_write_string(USART *pv_usart, const char *ps, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
#endif /* PERIPHERALS_USART_H_ */
