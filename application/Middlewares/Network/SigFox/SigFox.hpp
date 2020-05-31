/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Minimal LoRaWAN 1.0.x Class A Definition ***************/
/**********************************************************/
#pragma once

#include "network.hpp"
#include "board.h"


#define __SIGFOX_RCC_GPIOx_ENABLE()		__HAL_RCC_GPIOF_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();
#define __SIGFOX_RCC_USARTx_ENABLE()	__HAL_RCC_USART1_CLK_ENABLE()
// USART
#define SIGFOX_USARTx					USART1
#define SIGFOX_GPIO_AF_USARTx			GPIO_AF7_USART1
// Communication
#define SIGFOX_TXRX_GPIOx				GPIOB
#define SIGFOX_RX_GPIO_PIN				GPIO_PIN_7
#define SIGFOX_TX_GPIO_PIN				GPIO_PIN_6
// PWR
#define SIGFOX_PWR_GPIOx				GPIOF
#define SIGFOX_PWR_GPIO_PIN				GPIO_PIN_11
// SigFox Timeout
#define SIGFOX_TIMEOUT					10000 //ms


class ClassSigFox : public ClassNetwork
{
public:
  static const char *TYPE_NAME;  ///< The  network type's name.


public :
  ClassSigFox();

  const char *typeName() const;
  const char *deviceIdAsString();
  uint32_t    maxPayloadSize();


protected:
  bool openSpecific();
  void closeSpecific();
  void sleepSpecific();
  void wakeupSpecific();
  bool sendSpecific(const uint8_t *pu8_data,
		     uint16_t       size,
		     SendOptions    options);

private :
  UART_HandleTypeDef huart;
};
