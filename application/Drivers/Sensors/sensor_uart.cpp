/*
 * Base class for sensor using the extension port USART.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include <string.h>
#include "sensor_uart.hpp"
#include "connecsens.hpp"
#include "powerandclocks.h"


SensorUART::SensorUART(Power       power,
		       Power       powerSleep,
		       uint32_t    baudrate,
		       USARTParams uartParams,
		       Features    features) :
		       Sensor(power, powerSleep, features)
{
  this->_pvUSART        = usart_get_by_id(USART_ID_EXT);
  this->_baudrate       = baudrate;
  this->_params         = uartParams;
  this->_itReceivedData = false;
}

SensorUART::~SensorUART()
{
  // Do nothing
}


bool SensorUART::openSpecific()
{
  if(periodType() == PERIOD_TYPE_AT_SENSOR_S_FLOW)
  {
    this->_params |= USART_PARAM_RX_WHEN_ASLEEP;
  }

  return usart_open(this->_pvUSART, name(), this->_baudrate, this->_params);
}

void SensorUART::closeSpecific()
{
  usart_close(this->_pvUSART);
}


/**
 * Wait the end of a USART interruption driven read.
 *
 * Wait stop when we received something or in case of timeout.
 *
 * @param[in] timeoutMs The wait timeout time, in milliseconds.
 *
 * @return true  If we received something.
 * @return false Otherwise (timeout).
 */
bool SensorUART::waitEndOfItRead(uint32_t timeoutMs)
{
  uint32_t refMs, leftMs, ellapsedMs;
  bool     res = false;

  refMs  = board_ms_now();
  leftMs = timeoutMs;
  while(1)
  {
    pwrclk_sleep_ms_max(leftMs);
    if(this->_itReceivedData) { res = true; goto exit; }

    ellapsedMs = board_ms_diff(refMs, board_ms_now());
    if(ellapsedMs >= timeoutMs)
    {
      usart_stop_it_read(this->_pvUSART);
      goto exit;
    }
    leftMs = timeoutMs - ellapsedMs;
  }

  exit:
  return res;
}

/**
 * Stop the current continuous read if there is any.
 */
void SensorUART::uartStopContinuousRead()
{
  usart_stop_it_read(this->_pvUSART);
}

/**
 * Read a given number of bytes from the UART.
 *
 * @param[out] pu8Buffer  the buffer where to write the data read from the UART. MUST be NOT NULL.
 * @param[in]  size       the number of bytes to read from the UART.
 * @param[in]  timeoutMs  the maximum time, in milliseconds, allowed to get all the data.
 *                        If 0 then no timeout, exit this function while the UART read
 *                        is still running.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartRead(uint8_t *pu8Buffer, uint16_t size, uint32_t timeoutMs)
{
  usart_stop_it_read(this->_pvUSART);
  this->_itReceivedData = false;
  usart_read_it(this->_pvUSART, pu8Buffer, size, false, &SensorUART::rxCallback, this);

  return timeoutMs ? waitEndOfItRead(timeoutMs): true;
}

/**
 * Start a continuous read to get a given number of bytes from the UART.
 *
 * This function returns immediately but the read operation goes on forever or
 * until #SensorUART::uartStopContinuousRead() is called.
 *
 * Function #SensorUART::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
 * is called each time the expected amount of data has been received.
 *
 * @attention The buffer must be valid as long as the continuous read is running.
 *
 * @param[out] pu8Buffer the buffer where to write the data read from the UART. MUST be NOT NULL.
 * @param[in]  size      the number of bytes to read from the UART.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartReadContinuous(uint8_t *pu8Buffer, uint16_t size)
{
  uartStopContinuousRead();
  this->_itReceivedData = false;
  usart_read_it(this->_pvUSART, pu8Buffer, size, true, &SensorUART::rxCallback, this);

  return true;
}

/**
 * Look for a start byte then copy a given number of bytes from the UART to a buffer.
 *
 * @param[out] pu8Buffer    the buffer where the data are written to. MUST be NOT NULL.
 *                          The buffer MUST be big enough to receive all the bytes read from the UART,
 *                          plus the start byte if we are asked to copy it.
 * @param[in]  len          the number of bytes to read and copy from the UART to the buffer.
 *                          Does not include the start byte, only the following bytes.
 * @param[in]  from         the start byte to look for. The copy to the buffer will start as soon as
 *                          this start byte has been read from the UART.
 * @param[in]  copyFromByte copy the from (start) byte to the buffer or not?
 * @param[in]  timeoutMs    the maximum time, in milliseconds, allowed to get all the data.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartReadFrom(uint8_t *pu8Buffer,
			      uint16_t len,
			      uint8_t  from,
			      bool     copyFromByte,
			      uint32_t timeoutMs)
{
  usart_stop_it_read(this->_pvUSART);
  this->_itReceivedData = false;
  usart_read_from_it(this->_pvUSART,
		     pu8Buffer,               len,
		     from,
		     copyFromByte,            false,
		     &SensorUART::rxCallback, this);

  return timeoutMs ? waitEndOfItRead(timeoutMs): true;
}

/**
 * Start a continuous look for a start byte then copy a given number of bytes
 * from the UART to a buffer.
 *
 * This function returns immediately but the read operation goes on forever or
 * until #SensorUART::uartStopContinuousRead() is called.
 *
 * Function #SensorUART::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
 * is called each time matching data have been received.
 *
 * @attention The buffer must be valid as long as the continuous read is running.
 *
 * @param[out] pu8Buffer    the buffer where the data are written to. MUST be NOT NULL.
 *                          The buffer MUST be big enough to receive all the bytes read from the UART,
 *                          plus the start byte if we are asked to copy it.
 * @param[in]  len          the number of bytes to read and copy from the UART to the buffer.
 *                          Does not include the start byte, only the following bytes.
 * @param[in]  from         the start byte to look for. The copy to the buffer will start as soon as
 *                          this start byte has been read from the UART.
 * @param[in]  copyFromByte copy the from (start) byte to the buffer or not?
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartReadFromContinuous(uint8_t *pu8Buffer,
					uint16_t len,
					uint8_t  from,
					bool     copyFromByte)
{
  uartStopContinuousRead();
  this->_itReceivedData = false;
  usart_read_from_it(this->_pvUSART,
		     pu8Buffer,               len,
		     from,
		     copyFromByte,            true,
		     &SensorUART::rxCallback, this);

  return true;
}

/**
 * Read data from the UART and copy them to a buffer until we read a given stop byte.
 *
 * @param[out] pu8Buffer  the buffer where the data are written to. MUST be NOT NULL.
 * @param[in]  size       the buffer's size.
 * @param[in]  to         the stop byte to look for.
 * @param[in]  copyToByte copy the to (stop) byte to the buffer or not?
 * @param[in]  timeoutMs  the maximum time, in milliseconds, allowed to get all the data.
 *
 * @return the number of bytes written to the buffer.
 * @return 0 in case of error or timeout.
 */
uint16_t SensorUART::uartReadTo(uint8_t *pu8Buffer,
				uint16_t size,
				uint8_t  to,
				bool     copyToByte,
				uint32_t timeoutMs)
{
  usart_stop_it_read(this->_pvUSART);
  this->_itReceivedData = false;
  usart_read_to_it(  this->_pvUSART,
		     pu8Buffer,               size,
		     to,
		     copyToByte,              false,
		     &SensorUART::rxCallback, this);

  return timeoutMs ? waitEndOfItRead(timeoutMs): true;
}

/**
 * Start a continuous read where data are copied to a buffer until a stop byte is read,
 * or until the buffer is full.
 *
 * This function returns immediately but the read operation goes on forever or
 * until #SensorUART::uartStopContinuousRead() is called.
 *
 * Function #SensorUART::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
 * is called each time matching data have been received.
 *
 * @attention The buffer must be valid as long as the continuous read is running.
 *
 * @param[out] pu8Buffer  the buffer where the data are written to. MUST be NOT NULL.
 * @param[in]  size       the buffer's size.
 * @param[in]  to         the stop byte to look for.
 * @param[in]  copyToByte copy the to (stop) byte to the buffer or not?
 *
 * @return the number of bytes written to the buffer.
 * @return 0 in case of error or timeout.
 */
uint16_t SensorUART::uartReadToContinuous(uint8_t *pu8Buffer,
					  uint16_t size,
					  uint8_t  to,
					  bool     copyToByte)
{
  uartStopContinuousRead();
  this->_itReceivedData = false;
  usart_read_to_it(  this->_pvUSART,
		     pu8Buffer,               size,
		     to,
		     copyToByte,              true,
		     &SensorUART::rxCallback, this);

  return true;
}

/**
 * Read from the UART and copy the data bytes that are between a start (from) and a stop (to) bytes.
 *
 * @param[out] pu8Buffer      the buffer where the data are written to. MUST be NOT NULL.
 * @param[in]  size           the buffer's size.
 * @param[in]  from           the start byte to look for.
 * @param[in]  to             the end byte to look for.
 * @param[in]  copyBoundaries copy the start and stop bytes to the buffer?
 * @param[in]  timeoutMs      the maximum time, in milliseconds, allowed to get all the data.
 *
 * @return the number of bytes written to the buffer.
 * @return 0 in case of error or timeout.
 */
uint16_t SensorUART::uartReadFromTo(uint8_t *pu8Buffer,
				    uint16_t size,
				    uint8_t  from,
				    uint8_t  to,
				    bool     copyBoundaries,
				    uint32_t timeoutMs)
{
  usart_stop_it_read(   this->_pvUSART);
  this->_itReceivedData = false;
  usart_read_from_to_it(this->_pvUSART,
			pu8Buffer,               size,
			from,                    to,
			copyBoundaries,          false,
			&SensorUART::rxCallback, this);

  return timeoutMs ? waitEndOfItRead(timeoutMs): true;
}

/**
 * Start a continuous read to look for data between a start byte and a stop byte,
 * or until the reception buffer is full.
 *
 * This function returns immediately but the read operation goes on forever or
 * until #SensorUART::uartStopContinuousRead() is called.
 *
 * Function #SensorUART::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
 * is called each time matching data have been received.
 *
 * @attention The buffer must be valid as long as the continuous read is running.
 *
 * @param[out] pu8Buffer      the buffer where the data are written to. MUST be NOT NULL.
 * @param[in]  size           the buffer's size.
 * @param[in]  from           the start byte to look for.
 * @param[in]  to             the end byte to look for.
 * @param[in]  copyBoundaries copy the start and stop bytes to the buffer?
 *
 * @return the number of bytes written to the buffer.
 * @return 0 in case of error or timeout.
 */
uint16_t SensorUART::uartReadFromToContinuous(uint8_t *pu8Buffer,
					      uint16_t size,
					      uint8_t  from,
					      uint8_t  to,
					      bool     copyBoundaries)
{
  uartStopContinuousRead();
  this->_itReceivedData = false;
  usart_read_from_to_it(this->_pvUSART,
			pu8Buffer,               size,
			from,                    to,
			copyBoundaries,          true,
			&SensorUART::rxCallback, this);

  return true;
}

/**
 * Write data to the UART.
 *
 * @param[in] pu8Data   the data to write. MUST be NOT NULL.
 * @param[in] size      the number of data bytes to write.
 * @param[in] timeoutMs the maximum time, in milliseconds, allowed to send all the data.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartWrite(const uint8_t *pu8Data,
			   uint16_t       size,
			   uint32_t       timeoutMs)
{
  return usart_write(this->_pvUSART, pu8Data, size, timeoutMs);
}


/**
 * Write a string to the UART.
 *
 * @param[in] pu8Data   the data to write. MUST be NOT NULL. MUST be null terminated.
 * @param[in] timeoutMs the maximum time, in milliseconds, allowed to send all the data.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool SensorUART::uartWriteString(const char *ps, uint32_t timeoutMs)
{
  return usart_write_string(this->_pvUSART, ps, timeoutMs);
}

/**
 * Function to call to indicate that we have received data from the USART.
 *
 * Does nothing. Overwrite it in case something need to be done in a child class.
 *
 * @param[in] pv_usart Pointer to the USART instance that has received the data.
 *                     Is not NULL.
 * @param[in] pu8_data Pointer to the received data. Is NOT NULL.
 * @param[in] size     The number of data bytes received.
 */
void SensorUART::receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size)
{
  UNUSED(pv_usart);
  UNUSED(pu8_data);
  UNUSED(size);
}

/**
 * USART Rx interruption callback function,
 * called when the USART has received data in interruption mode.
 *
 * @param[in] pv_usart Pointer to the USART instance that has received the data.
 *                     Is not NULL.
 * @param[in] pu8_data Pointer to the received data. Is NOT NULL.
 * @param[in] size     The number of data bytes received.
 * @param[in] pv_args  In our cases this is a pointer to the #SensorUART instance
 *                     that has requested the read.
 */
void SensorUART::rxCallback(USART   *pv_usart,
			    uint8_t *pu8_data,
			    uint32_t size,
			    void    *pv_args)
{
  ((SensorUART *)pv_args)->_itReceivedData = true;
  ((SensorUART *)pv_args)->receivedData(pv_usart, pu8_data, size);
}


