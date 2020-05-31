/*
 * Base class for sensor using the extension port USART.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef SENSORS_SENSOR_UART_HPP_
#define SENSORS_SENSOR_UART_HPP_

#include "sensor.hpp"
#include "board.h"
#include "usart.h"


class SensorUART : public Sensor
{

public:
  SensorUART(Power       power,
	     Power       powerSleep,
	     uint32_t    baudrate,
	     USARTParams uartParams,
	     Features    features = FEATURE_BASE);
  virtual ~SensorUART();


protected:
  bool     uartRead(uint8_t *pu8Buffer,
		    uint16_t size,
		    uint32_t timeoutMs);
  bool     uartReadContinuous(uint8_t *pu8Buffer,
			      uint16_t size);
  bool     uartReadFrom(uint8_t *pu8Buffer,
			uint16_t len,
			uint8_t  from,
			bool     copyFromByte,
			uint32_t timeoutMs);
  bool     uartReadFromContinuous(uint8_t *pu8Buffer,
				  uint16_t len,
				  uint8_t  from,
				  bool     copyFromByte);
  uint16_t uartReadTo(uint8_t *pu8Buffer,
		      uint16_t size,
		      uint8_t  to,
		      bool     copyToByte,
		      uint32_t timeoutMs);
  uint16_t uartReadToContinuous(uint8_t *pu8Buffer,
				uint16_t size,
				uint8_t  to,
				bool     copyToByte);
  uint16_t uartReadFromTo(uint8_t *pu8Buffer,
			  uint16_t size,
			  uint8_t  from,
			  uint8_t  to,
			  bool     copyBoundaries,
			  uint32_t timeoutMs);
  uint16_t uartReadFromToContinuous(uint8_t *pu8Buffer,
				    uint16_t size,
				    uint8_t  from,
				    uint8_t  to,
				    bool     copyBoundaries);
  void uartStopContinuousRead();


  bool uartWrite(      const uint8_t *pu8Data, uint16_t size, uint32_t timeoutMs);
  bool uartWriteString(const char    *ps,      uint32_t timeoutMs);

  virtual void receivedData(USART *pv_usart, uint8_t *pu8_data, uint32_t size);
  virtual bool openSpecific();
  virtual void closeSpecific();

private:

  bool waitEndOfItRead(uint32_t timeoutMs);

  static void rxCallback(USART *pv_usart, uint8_t *pu8_data, uint32_t size, void *pv_args);


private:
  USART      *_pvUSART;         ///< The UART object.
  uint32_t    _baudrate;        ///< The baudrate to use.
  USARTParams _params;          ///< The UART parameters.
  bool        _itReceivedData;  ///< Indicate if we have received data using interruption read.
};



#endif /* SENSORS_SENSOR_UART_HPP_ */
