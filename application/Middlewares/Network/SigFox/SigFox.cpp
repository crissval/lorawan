
#include <string.h>
#include "SigFox.hpp"
#include "binhex.hpp"



const char *ClassSigFox::TYPE_NAME  = "SigFox";


// Le constructeur appelle le constructeur parent
ClassSigFox::ClassSigFox()
{
  // Do nothing
}

// Initialisation du chip SigFox
bool ClassSigFox::openSpecific()
{
  uint8_t *buffer = (uint8_t *) malloc(5*sizeof(uint8_t));
  if(!buffer) return false;
  buffer[0]='\0';
  __SIGFOX_RCC_GPIOx_ENABLE();
  /* Activation de l'alimentation de l'�tage RF */
  GPIO_InitTypeDef gpioinit;
  gpioinit.Alternate = 	0;
  gpioinit.Mode = 		GPIO_MODE_OUTPUT_PP;
  gpioinit.Pin = 			SIGFOX_PWR_GPIO_PIN;
  gpioinit.Pull = 		GPIO_NOPULL;
  gpioinit.Speed = 		GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SIGFOX_PWR_GPIOx,&gpioinit);
  HAL_GPIO_WritePin(SIGFOX_PWR_GPIOx,SIGFOX_PWR_GPIO_PIN,GPIO_PIN_SET);
  GPIO_InitTypeDef GPIO_InitStruct;
  // Initialisation GPIO - AF - TX & RX
  GPIO_InitStruct.Alternate = SIGFOX_GPIO_AF_USARTx;
  GPIO_InitStruct.Mode 	  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pin		  = SIGFOX_RX_GPIO_PIN | SIGFOX_TX_GPIO_PIN;
  GPIO_InitStruct.Pull 	  = GPIO_PULLUP;
  GPIO_InitStruct.Speed 	  = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SIGFOX_TXRX_GPIOx, &GPIO_InitStruct);
  /* Initialisation de la communication s�rie */
  __SIGFOX_RCC_USARTx_ENABLE();
  this->huart.Instance =                    SIGFOX_USARTx;
  this->huart.Init.BaudRate =               9600;
  this->huart.Init.WordLength =             UART_WORDLENGTH_8B;
  this->huart.Init.StopBits =       		  UART_STOPBITS_1;
  this->huart.Init.Parity =                 UART_PARITY_NONE;
  this->huart.Init.Mode =                   UART_MODE_TX_RX;
  this->huart.Init.HwFlowCtl =              UART_HWCONTROL_NONE;
  this->huart.Init.OverSampling =           UART_OVERSAMPLING_16;
  this->huart.Init.OneBitSampling =         UART_ONE_BIT_SAMPLE_DISABLE;
  this->huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(&(this->huart));
  /* Check communication status */
  HAL_StatusTypeDef status;
  for(int i=0;i<3;i++){
    HAL_UART_Transmit(&(this->huart),(uint8_t *)"AT\r\n",4,1000);
    status = HAL_UART_Receive(&(this->huart),buffer,4,1000);
    if((status == HAL_OK) & (buffer[0] == 'O')){
      free(buffer);
      return true;
    }
    HAL_Delay(100);
  }
  return false;
}

void ClassSigFox::closeSpecific()
{
  HAL_GPIO_WritePin(SIGFOX_PWR_GPIOx,SIGFOX_PWR_GPIO_PIN,GPIO_PIN_RESET);
}

void ClassSigFox::sleepSpecific()
{
  // TODO: implement me
}

void ClassSigFox::wakeupSpecific()
{
  // TODO: implement me
}

const char *ClassSigFox::typeName() const
{
  return TYPE_NAME;
}

const char *ClassSigFox::deviceIdAsString()
{
  // TODO: implement me
  return ClassNetwork::deviceIdAsString();
}

uint32_t ClassSigFox::maxPayloadSize()
{
  return 12;
}


/* cette fonction est appel�e avec un payload de taille < � 12 octets */
bool ClassSigFox::sendSpecific(const uint8_t *pu8_data,
			       uint16_t       size,
			       SendOptions    options)
{
  uint8_t buffer[32]; // 6 header bytes + 24 bytes (12 * 2) data bytes + \r + \n = 32

  UNUSED(options);

  // Check data size
  if(size > 12) return false;

  strcpy((char *)buffer, "AT$SF=");
  n_bin2hex((const char *)pu8_data, (char *)&buffer[6], size);
  strcat((char *)buffer, "\r\n");

  // Envoi de la commande
  HAL_UART_Transmit(&this->huart, buffer, strlen((char *)buffer), 1000);
  HAL_StatusTypeDef status = HAL_UART_Receive(&this->huart, buffer, 4, SIGFOX_TIMEOUT);

  return (status == HAL_OK);
}

