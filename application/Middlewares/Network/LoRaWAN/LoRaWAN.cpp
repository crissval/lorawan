
#include <string.h>
#include "LoRaWAN.hpp"
#include "region/Region.h"
#include "logger.h"
#include "binhex.hpp"
#include "loraradio.h"
#include "sx1272.h"
#include "bq2589x.h"
#include "connecsens.hpp"


#ifndef LORAWAN_PUBLIC_NETWORK
#define LORAWAN_PUBLIC_NETWORK  false
#endif

#ifndef LORAWAN_DEFAULT_DATA_RATE
#define LORAWAN_DEFAULT_DATA_RATE  DR_5
#endif

#ifndef LORAWAN_JOINREQ_NB_TRIALS
#define LORAWAN_JOINREQ_NB_TRIALS  8
#endif

#ifndef LORAWAN_SENDACK_NB_TRIALS
#define LORAWAN_SENDACK_NB_TRIALS  2
#endif

#ifndef LORAWAN_ENABLE_ADR
#define LORAWAN_ENABLE_ADR         true
#endif


#define DEFAULT_MAX_PAYLOAD_SIZE  230

#define LORA_RADIO_POWER_ON_SETTLE_TIME_MS  200
#define LORA_RADIO_DIO_IRQ_PRIORITY         0
#define LORA_RADIO_SPI_TIMEOUT_MS           1000

#define LORAWAN_APP_PORT       2
#define LORAWAN_DEFAULT_CLASS  CLASS_A


  CREATE_LOGGER(lorawan);
#undef  logger
#define logger  lorawan


  const char *ClassLoRaWAN::loramacStatusToString[] =
  {
      "",                                // LORAMAC_STATUS_OK
      "busy",                            // LORAMAC_STATUS_BUSY
      "unknown service",                 // LORAMAC_STATUS_SERVICE_UNKNOWN
      "invalid parameter",               // LORAMAC_STATUS_PARAMETER_INVALID
      "invalid frequency",               // LORAMAC_STATUS_FREQUENCY_INVALID
      "invalid datarate",                // LORAMAC_STATUS_DATARATE_INVALID
      "invalid frequency and datarate",  // LORAMAC_STATUS_FREQ_AND_DR_INVALID
      "has not joined network",          // LORAMAC_STATUS_NO_NETWORK_JOINED
      "payload length error",            // LORAMAC_STATUS_LENGTH_ERROR
      "device is switched off",          // LORAMAC_STATUS_DEVICE_OFF
      "region not supported"             // LORAMAC_STATUS_REGION_NOT_SUPPORTED
  };

  const char *ClassLoRaWAN::loramacRegionIdToString[] =
  {
      "AS923", "AU915", "CN470", "CN779", "EU433", "EU868",
      "KR920", "IN865", "US915", "US915_HYBRID"
  };


static SPI_HandleTypeDef _lorawan_spi; ///< The SPI object to use.

static void     lorawan_SX1272IoInit();
static void     lorawan_SX1272IoDeInit();
static void     lorawan_SX1272IoIrqInit(       DioIrqHandler **irqHandlers);
static void     lorawan_SX1272SetXO(           uint8_t         state);
static uint32_t lorawan_SX1272GetWakeTime();
static uint8_t  lorawan_SX1272GetPaSelect(     uint32_t channel);
static void     lorawan_SX1272SetAntSwLowPower(bool     status);
static void     lorawan_SX1272SetRfTxPower(    int8_t   power);
static void     lorawan_SX1272SetAntSw(        uint8_t  opMode);
static bool     lorawan_SX1272CheckRfFrequency(uint32_t frequency);


static LoRaBoardCallback_t LoRaBoardCallbacks =
{
    lorawan_SX1272SetXO,
    lorawan_SX1272GetWakeTime,
    lorawan_SX1272IoIrqInit,
    lorawan_SX1272SetRfTxPower,
    lorawan_SX1272SetAntSwLowPower,
    lorawan_SX1272SetAntSw
};

static void     lorawan_sx1272_com_reset(bool reset);
static void     lorawan_sx1272_com_start();
static void     lorawan_sx1272_com_stop();
static uint8_t *lorawan_sx1272_com_read( uint8_t       *pu8_buffer, uint16_t size);
static bool     lorawan_sx1272_com_write(const uint8_t *pu8_data,   uint16_t size);

static LoRaRadioCom lorawan_sx1272_com =
{
    lorawan_sx1272_com_reset,
    lorawan_sx1272_com_start,
    lorawan_sx1272_com_stop,
    lorawan_sx1272_com_read,
    lorawan_sx1272_com_write
};

/**
 * The LoRa radio object
 */
const struct LoRaRadio lora_radio =
{
    lorawan_SX1272IoInit,
    lorawan_SX1272IoDeInit,
    SX1272Init,
    SX1272ReInit,
    SX1272GetStatus,
    SX1272SetModem,
    SX1272SetChannel,
    SX1272IsChannelFree,
    SX1272Random,
    SX1272SetRxConfig,
    SX1272SetTxConfig,
    lorawan_SX1272CheckRfFrequency,
    SX1272GetTimeOnAir,
    SX1272Send,
    SX1272SetSleep,
    SX1272SetStby,
    SX1272SetRx,
    SX1272StartCad,
    SX1272SetTxContinuousWave,
    SX1272ReadRssi,
    SX1272Write,
    SX1272Read,
    SX1272WriteBuffer,
    SX1272ReadBuffer,
    SX1272SetMaxPayloadLength,
    SX1272SetPublicNetwork,
    SX1272GetRadioWakeUpTime
};


ClassLoRaWAN *ClassLoRaWAN::pvInstance = NULL;
const char *  ClassLoRaWAN::TYPE_NAME  = "LoRaWAN";


ClassLoRaWAN::ClassLoRaWAN(): ClassNetwork()
{
  this->_psDevEUI[0]           = '\0';
  this->_datarateMin           = DR_0;
  this->_datarateMax           = DR_7;
  this->_defaultDatarate       = LORAWAN_DEFAULT_DATA_RATE;
  this->_joinDatarate          = LORAWAN_DEFAULT_DATA_RATE;
  this->_joinNbTrials          = LORAWAN_JOINREQ_NB_TRIALS;
  this->_sendAckNbTrials       = LORAWAN_SENDACK_NB_TRIALS;
  this->_enablePublicNetwork   = LORAWAN_PUBLIC_NETWORK;
  this->_enableADR             = LORAWAN_ENABLE_ADR;
  this->_macEvents             = MAC_EVT_NONE;

  memset(this->_devEUI, 0, sizeof(this->_devEUI));
  memset(this->_appEUI, 0, sizeof(this->_appEUI));
  memset(this->_appKey, 0, sizeof(this->_appKey));

  // Iinitialisations for MAC layer
  _loramacPrimitives.MacMcpsConfirm    = &mcpsConfirm;
  _loramacPrimitives.MacMcpsIndication = &mcpsIndication;
  _loramacPrimitives.MacMlmeConfirm    = &mlmeConfirm;
  _loramacCallbacks.GetBatteryLevel    = &batteryLevel;

  // Configure QoS log printing
  setQoSValuesToPrintOnFrameRx(ClassNetwork::QOS_RSSI_DBM |
			       ClassNetwork::QOS_SNR_DB   |
			       ClassLoRaWAN::QOS_DR);


  // Set Radio board callbacks
  SX1272BoardInit(&LoRaBoardCallbacks, &lorawan_sx1272_com);

  pvInstance = this;
}


const char *ClassLoRaWAN::typeName() const
{
  return TYPE_NAME;
}

/**
 * Return the DevEUI, as a string, as device identifier in the network.
 *
 * @pre the DevEUI MUST have been set before calling this function.
 *
 * @return the DevEUI as a string.
 */
const char *ClassLoRaWAN::deviceIdAsString()
{
  return this->_psDevEUI;
}

void ClassLoRaWAN::cancel()
{
  LoRaMacCancel();

  ClassNetwork::cancel();
}

bool ClassLoRaWAN::openSpecific()
{
  MibRequestConfirm_t mibReq;

  // Init LoRa radio
  lora_radio.IoInit();

  // Init MAC layer
  LoRaMacInitialization(&this->_loramacPrimitives,
			&this->_loramacCallbacks,
			PASTER2(LORAMAC_REGION_, LORAMAC_REGION));
  log_debug(logger, "Configure LoRaWAN MAC layer for region %d (%s) operation.",
	    PASTER2(LORAMAC_REGION_, LORAMAC_REGION),
	    loramacRegionIdToString[PASTER2(LORAMAC_REGION_, LORAMAC_REGION)]);

  // Set up some default communication parameters
  mibReq.Type            = MIB_ADR;
  mibReq.Param.AdrEnable = _enableADR;
  LoRaMacMibSetRequestConfirm(&mibReq);

  mibReq.Type                   = MIB_CHANNELS_DATARATE;
  mibReq.Param.ChannelsDatarate = this->_defaultDatarate;
  LoRaMacMibSetRequestConfirm(&mibReq);

  mibReq.Type                      = MIB_PUBLIC_NETWORK;
  mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
  LoRaMacMibSetRequestConfirm(&mibReq);

  mibReq.Type        = MIB_DEVICE_CLASS;
  mibReq.Param.Class = CLASS_A;
  LoRaMacMibSetRequestConfirm(&mibReq);

  return true;
}

void ClassLoRaWAN::closeSpecific()
{
  LoRaMacDeInit();
  lora_radio.IoDeInit();
}

void ClassLoRaWAN::sleepSpecific()
{
  lora_radio.IoDeInit();
}

void ClassLoRaWAN::wakeupSpecific()
{
  lora_radio.IoInit();
  lora_radio.ReInit();
}


bool ClassLoRaWAN::setConfiguration(const JsonObject &json)
{
  int8_t      i8;
  int32_t     i32;
  const char *psValue;
  bool        res = ClassNetwork::setConfiguration(json);

  // Get devEUI
  if( json["devEUI"].success()                      &&
      (psValue = json["devEUI"].as<const char *>()) &&
      is_hex_string_of_len(psValue, LORAWAN_DEVEUI_SIZE * 2))
  {
    n_hex2bin(psValue, (char *)this->_devEUI, LORAWAN_DEVEUI_SIZE);
    strlcpy(this->_psDevEUI, psValue, sizeof(this->_psDevEUI));
  }
  else
  {
    log_error(logger,"Configuration parameter 'devEUI' must be set with an hexadecimal value 16 characters long.");
    res = false;
  }

  // Get appEUI
  if( json["appEUI"].success()                      &&
      (psValue = json["appEUI"].as<const char *>()) &&
      is_hex_string_of_len(psValue, LORAWAN_APPEUI_SIZE * 2))
  {
    n_hex2bin(psValue, (char *)this->_appEUI, LORAWAN_APPEUI_SIZE);
  }
  else
  {
    log_error(logger,"Configuration parameter 'appEUI' must be set with an hexadecimal value 16 characters long.");
    res = false;
  }

  // Get appKey
  if( json["appKey"].success()                      &&
      (psValue = json["appKey"].as<const char *>()) &&
      is_hex_string_of_len(psValue, LORAWAN_APPKEY_SIZE * 2))
  {
    n_hex2bin(psValue, (char *)this->_appKey, LORAWAN_APPKEY_SIZE);
  }
  else
  {
    log_error(logger, "Configuration parameter 'appKey' must be set with an hexadecimal value 32 characters long.");
    res = false;
  }

  // Data Rate
  if(json["dataRate"].success())
  {
    i8 = json["dataRate"].as<int8_t>();
    if(i8 < 0 || i8 > 5)
    {
      log_error(logger, "'dataRate' parameter's value must in range [0..5].");
      res = false;
    }
    else { setDatarate((Datarate)i8); }
  }

  // ADR
  if(json["useAdapativeDataRate"].success())
  {
    enableAdapatativeDatarate(json["useAdapativeDataRate"].as<bool>());
  }

  // Public network
  if(json["publicNetwork"].success())
  {
    this->_enablePublicNetwork = json["publicNetwork"].as<bool>();
  }

  // Join and send nb trials
  if(json["joinNbTrials"].success())
  {
    i32 = json["joinNbTrials"].as<int32_t>();
    if(i32 <= 0 || i32 > 8)
    {
      log_warn(logger, "Value for configuration parameter 'joinNbTrials' must be in range [1..8]; using default value: %d.", this->_joinNbTrials);
    }
    else { this->_joinNbTrials = (uint8_t)i32; }
  }
  if(json["sendAckNbTrials"].success())
  {
    i32 = json["sendAckNbTrials"].as<int32_t>();
    if(i32 <= 0 || i32 > 8)
    {
      log_warn(logger, "Value for configuration parameter 'sendAckNbTrials' must be in range [1..8]; using default value: %d.", this->_sendAckNbTrials);
    }
    else { this->_sendAckNbTrials = (uint8_t)i32; }
  }

  return res;
}


void ClassLoRaWAN::setDatarate(Datarate dr)
{
  MibRequestConfirm_t mibReq;

  if(dr >= this->_datarateMin && dr <= this->_datarateMax)
  {
    this->_joinDatarate = this->_defaultDatarate = (int8_t)dr;
    if(isOpened())
    {
      mibReq.Type                   = MIB_CHANNELS_DATARATE;
      mibReq.Param.ChannelsDatarate = dr;
      LoRaMacMibSetRequestConfirm(&mibReq);
    }
  }
}

ClassLoRaWAN::Datarate ClassLoRaWAN::datarate()
{
  MibRequestConfirm_t mib;

  mib.Type = MIB_CHANNELS_DATARATE;
  LoRaMacMibGetRequestConfirm(&mib);

  return (Datarate)mib.Param.ChannelsDatarate;
}

void ClassLoRaWAN::setDatarateRange(int8_t drMin, int8_t drMax)
{
  if(drMin <= drMax)
  {
    this->_datarateMin = drMin;
    this->_datarateMax = drMax;
  }
}

void ClassLoRaWAN::enableAdapatativeDatarate(bool enable)
{
  this->_enableADR = enable;
}


uint32_t ClassLoRaWAN::maxPayloadSize()
{
  LoRaMacTxInfo_t txInfos;

  LoRaMacQueryTxPossible(0, &txInfos);

  return txInfos.MaxPossiblePayload;
}


/**
 * Process awaiting MAC events.
 *
 * @return true  if there is more processing left to do.
 * @return false otherwise.
 */
bool ClassLoRaWAN::process()
{
  bool      res;
  MACEvents evts = this->_macEvents;

  // Clear events so that we can receive new ones even while we are processing previous ones.
  this->_macEvents = MAC_EVT_NONE;

  res = ClassNetwork::process();

  if(     evts & MAC_EVT_ACK_RECEIVED)    { receivedSendAck(true);            }
  else if(evts & MAC_EVT_NO_ACK_RECEIVED) { receivedSendAck(false);           }

  if(     evts & MAC_EVT_JOINED)          { setJoinState(JOIN_STATUS_JOINED); }
  else if(evts & MAC_EVT_JOIN_FAILED)     { setJoinState(JOIN_STATUS_FAILED); }




  if(!LoRaMacIsBusy())
  {
    if(this->joinState() == JOIN_STATUS_JOINING) { setJoinState(JOIN_STATUS_FAILED); }
    if(this->sendState() == SEND_STATE_SENDING)  { setSendState(SEND_STATE_FAILED);  }
  }

  return res;
}


/**
 * LoRaWAN join specific implementation.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ClassLoRaWAN::joinSpecific()
{
  MlmeReq_t       mlmeReq;
  LoRaMacStatus_t status;

  // Over the air activation
  mlmeReq.Type              = MLME_JOIN;
  mlmeReq.Req.Join.DevEui   = this->_devEUI;
  mlmeReq.Req.Join.AppEui   = this->_appEUI;
  mlmeReq.Req.Join.AppKey   = this->_appKey;
  mlmeReq.Req.Join.NbTrials = this->_joinNbTrials;
  mlmeReq.Req.Join.DataRate = this->_joinDatarate;
  status = LoRaMacMlmeRequest(&mlmeReq);
  if(status != LORAMAC_STATUS_OK)
  {
    log_error(logger, "Join error: %d (%s).", status, loramacStatusToString[status]);
  }

 return status == LORAMAC_STATUS_OK;
}

bool ClassLoRaWAN::sendSpecific(const uint8_t *pu8_data,
				uint16_t       size,
				SendOptions    options)
{
  McpsReq_t       mcpsReq;
  LoRaMacTxInfo_t txInfo;
  LoRaMacStatus_t status;
  const char     *psMsgType;

  // Copy data to data buffer
  memcpy(this->_dataBuffer, pu8_data, size);

  // Prepare send request.
  if(LoRaMacQueryTxPossible(size, &txInfo) != LORAMAC_STATUS_OK)
  {
      // Send empty frame in order to flush MAC commands
    psMsgType                           = "Unconfirmed";
    mcpsReq.Type                        = MCPS_UNCONFIRMED;
    mcpsReq.Req.Unconfirmed.fBuffer     = NULL;
    mcpsReq.Req.Unconfirmed.fBufferSize = 0;
    mcpsReq.Req.Unconfirmed.Datarate    = this->_defaultDatarate;
  }
  else
  {
    if(options & SEND_OPTION_REQUEST_ACK)
    {
      psMsgType                         = "Confirmed";
      mcpsReq.Type                      = MCPS_CONFIRMED;
      mcpsReq.Req.Confirmed.fPort       = LORAWAN_APP_PORT;
      mcpsReq.Req.Confirmed.fBuffer     = this->_dataBuffer;
      mcpsReq.Req.Confirmed.fBufferSize = size;
      mcpsReq.Req.Confirmed.NbTrials    = this->_sendAckNbTrials;
      mcpsReq.Req.Confirmed.Datarate    = this->_defaultDatarate;
    }
    else
    {
      psMsgType                           = "Unconfirmed";
      mcpsReq.Type                        = MCPS_UNCONFIRMED;
      mcpsReq.Req.Unconfirmed.fPort       = LORAWAN_APP_PORT;
      mcpsReq.Req.Unconfirmed.fBuffer     = this->_dataBuffer;
      mcpsReq.Req.Unconfirmed.fBufferSize = size;
      mcpsReq.Req.Unconfirmed.Datarate    = this->_defaultDatarate;
    }
  }

  // Send a send request.
  status = LoRaMacMcpsRequest(&mcpsReq);
  log_debug(logger, "Send data rate is: DR%d.", datarate());
  if(status != LORAMAC_STATUS_OK)
  {
    log_error(logger, "Send error: %d (%s).", status, loramacStatusToString[status]);
  }
  else
  {
    log_info(logger, "%s message sent.", psMsgType);
  }

  return status == LORAMAC_STATUS_OK;
}



bool ClassLoRaWAN::getQoSValues(QoSValues *pvValues, QoSFlags valuesToGet)
{
  pvValues->flags = QOS_FLAG_NONE;

  if(valuesToGet & QOS_RSSI_DBM)
  {
    pvValues->rssi   = SX1272.Settings.LoRaPacketHandler.RssiValue;
    pvValues->flags |= QOS_RSSI_DBM;
  }
  if(valuesToGet & QOS_SNR_DB)
  {
    pvValues->snr    = SX1272.Settings.LoRaPacketHandler.SnrValue;
    pvValues->flags |= QOS_SNR_DB;
  }

  return pvValues->flags != QOS_FLAG_NONE;
}


char *ClassLoRaWAN::QoSValuesToString(char    *psBuffer,
				      uint16_t size,
				      const    QoSValues *pvValues)
{
  uint16_t len, l;

  ClassNetwork::QoSValuesToString(psBuffer, size, pvValues);
  len = strlen(psBuffer);
  if(len >= size) { goto exit; }
  size -= len;

  if(pvValues->flags & QOS_DR)
  {
    if(len)
    {
      l = strlcpy(psBuffer + len, QOS_VALUES_STRING_SEP_STR, size);
      if(l >= size) { goto exit; }
      len += l; size -= l;
    }
    l = snprintf( psBuffer + len, size, "DR%i", ((QoSValuesSpecific *)pvValues)->dr);
    if(l >= size) { goto exit; }
    len += l; size -= l;
  }

  exit:
  return psBuffer;
}


/**
 * MCPS-Confirm event function.
 *
 * @param[in] pvMcpsConfirm pointer to the confirm object. MUST be NOT NULL.
 */
void ClassLoRaWAN::mcpsConfirm(McpsConfirm_t *pvMcpsConfirm)
{
  if(!pvInstance) { return; }

  if(pvMcpsConfirm->McpsRequest == MCPS_CONFIRMED)
  {
    pvInstance->addMacEvent(
	pvMcpsConfirm->AckReceived ?
	    MAC_EVT_ACK_RECEIVED :
	    MAC_EVT_NO_ACK_RECEIVED);
  }
}

/**
 * MCPS-Indication event function.
 *
 * @param[in] pvMcpsIndication the indication object. MUST be NOT NULL.
 */
void ClassLoRaWAN::mcpsIndication(McpsIndication_t *pvMcpsIndication)
{
  if(!pvInstance || pvMcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK) { return; }

  if(pvMcpsIndication->RxData)
  {
    // Here we get the data received from the gateway
    // pvMcpsIndication->Buffer contains the data.
    // pvMcpsIndication->BufferSize contains the data count.
    switch(pvMcpsIndication->Port)
    {
      case 3:
	// This port switches the class
	if(pvMcpsIndication->BufferSize == 1)
	{
	  switch(pvMcpsIndication->Buffer[0])
	  {
	    case 0:  pvInstance->requestClass(CLASS_A); break;
	    case 1:  pvInstance->requestClass(CLASS_B); break;
	    case 2:  pvInstance->requestClass(CLASS_C); break;
	    default: break;
	  }
	}
	break;

      case LORAWAN_APP_PORT:
      default:
	// Do nothing
	break;
    }
  }
}

/**
 * MLME-Confirm event function
 *
 * @param[in] pvMlmeConfirm the confirm object. MUST be NOT NULL.
 */
void ClassLoRaWAN::mlmeConfirm(MlmeConfirm_t *pvMlmeConfirm)
{
  if(!pvInstance) { return; }

  switch(pvMlmeConfirm->MlmeRequest)
  {
    case MLME_JOIN:
      pvInstance->addMacEvent(
	  (pvMlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK) ?
	      MAC_EVT_JOINED :
	      MAC_EVT_JOIN_FAILED);
      break;

    case MLME_LINK_CHECK:
    default:
      // Do nothing
      break;
  }
}


/**
 * Function to call to change when a class change has been received from the network.
 */
bool ClassLoRaWAN::requestClass(DeviceClass_t newClass)
{
  UNUSED(newClass);
  // We do not use any other class than A so this function is not implemented.
  return false;
}


/**
 * Return the battery level in the format expected by the MAC layer.
 *
 * @return the level value in range [1..254].
 * @return 255 if the information is not available.
 */
uint8_t ClassLoRaWAN::batteryLevel()
{
  return bq2589x_vbat_level(1, 254, 255);
}



static void lorawan_SX1272IoInit()
{
  GPIO_InitTypeDef init;

  gpio_use_gpios_with_ids(LORA_RESET_GPIO, LORA_PWR_GPIO,
			  LORA_MOSI_GPIO,  LORA_MISO_GPIO,
			  LORA_SCK_GPIO,   LORA_NSS_GPIO,
			  LORA_DIO0_GPIO,  LORA_DIO1_GPIO,
			  LORA_DIO2_GPIO,  LORA_DIO3_GPIO
			  );

  // Make sure to not activate the radio reset PIN
  init.Alternate = 0;
  init.Mode      = GPIO_MODE_ANALOG;
  init.Pull      = GPIO_NOPULL;
  init.Speed     = GPIO_SPEED_LOW;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_RESET_GPIO, &init.Pin), &init);

  // Configure power GPIO.
  init.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_PWR_GPIO,   &init.Pin), &init);

  // Configure SPI interface
  // First SPI module
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _CLK_ENABLE)();
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _FORCE_RESET)();
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _RELEASE_RESET)();
  _lorawan_spi.Instance               = PASTER2(SPI, LORA_SPI_ID);
  _lorawan_spi.Init.BaudRatePrescaler = SPI_CR1_BR_1;
  _lorawan_spi.Init.Direction         = SPI_DIRECTION_2LINES;
  _lorawan_spi.Init.Mode              = SPI_MODE_MASTER;
  _lorawan_spi.Init.CLKPolarity       = SPI_POLARITY_LOW;
  _lorawan_spi.Init.CLKPhase          = SPI_PHASE_1EDGE;
  _lorawan_spi.Init.DataSize          = SPI_DATASIZE_8BIT;
  _lorawan_spi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  _lorawan_spi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  _lorawan_spi.Init.NSS               = SPI_NSS_SOFT;
  _lorawan_spi.Init.TIMode            = SPI_TIMODE_DISABLE;
  HAL_SPI_Init(&_lorawan_spi);

  // Then the GPIOs
  init.Mode      = GPIO_MODE_AF_PP;
  init.Pull      = GPIO_NOPULL;
  init.Speed     = GPIO_SPEED_HIGH;
  init.Alternate = LORA_MOSI_AF;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_MOSI_GPIO,  &init.Pin), &init);
  init.Alternate = LORA_MOSI_AF;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_MISO_GPIO,  &init.Pin), &init);
  init.Alternate = LORA_SCK_AF;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_SCK_GPIO,   &init.Pin), &init);
  init.Mode      = GPIO_MODE_OUTPUT_PP;
  init.Alternate = 0;
  HAL_GPIO_Init(    gpio_hal_port_and_pin_from_id(LORA_NSS_GPIO, &init.Pin), &init);
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_NSS_GPIO, &init.Pin), init.Pin, GPIO_PIN_SET);

  // Turn radio power ON
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_PWR_GPIO, &init.Pin),
		    init.Pin,
		    (GPIO_PinState)LORA_PWR_LEVEL_FOR_ON);
#if defined LORA_RADIO_POWER_ON_SETTLE_TIME_MS && LORA_RADIO_POWER_ON_SETTLE_TIME_MS != 0
  HAL_Delay(LORA_RADIO_POWER_ON_SETTLE_TIME_MS);  // Wait for power to settle down.
#endif

  // Set up interruption GPIOs
  init.Mode  = GPIO_MODE_IT_RISING;
  init.Pull  = GPIO_PULLDOWN;
  init.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_DIO0_GPIO, &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_DIO1_GPIO, &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_DIO2_GPIO, &init.Pin), &init);
#ifdef LORAWAN_USE_DIO3
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_DIO3_GPIO, &init.Pin), &init);
#endif
}

static void lorawan_SX1272IoDeInit()
{
  GPIO_InitTypeDef init;

  // Remove interruption handlers
  gpio_remove_irq_handler(LORA_DIO0_GPIO);
  gpio_remove_irq_handler(LORA_DIO1_GPIO);
  gpio_remove_irq_handler(LORA_DIO2_GPIO);
  gpio_remove_irq_handler(LORA_DIO3_GPIO);

  // Turn radio power OFF
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_PWR_GPIO,   &init.Pin),
		    init.Pin,
		    (GPIO_PinState)!LORA_PWR_LEVEL_FOR_ON);

  // Free GPIOs
  gpio_free_gpios_with_ids(LORA_RESET_GPIO, LORA_PWR_GPIO,
			   LORA_MOSI_GPIO,  LORA_MISO_GPIO,
			   LORA_SCK_GPIO,   LORA_NSS_GPIO,
			   LORA_DIO0_GPIO,  LORA_DIO1_GPIO,
			   LORA_DIO2_GPIO,  LORA_DIO3_GPIO);

  // Disable SPI module
  HAL_SPI_DeInit(&_lorawan_spi);
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _FORCE_RESET)();
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _RELEASE_RESET)();
  PASTER3(__HAL_RCC_SPI, LORA_SPI_ID, _CLK_DISABLE)();
}

#define IRQ_HIGH_PRIORITY 0
static void lorawan_SX1272IoIrqInit(DioIrqHandler **irqHandlers)
{
  gpio_set_irq_handler(LORA_DIO0_GPIO, LORA_RADIO_DIO_IRQ_PRIORITY, irqHandlers[0]);
  gpio_set_irq_handler(LORA_DIO1_GPIO, LORA_RADIO_DIO_IRQ_PRIORITY, irqHandlers[1]);
  gpio_set_irq_handler(LORA_DIO2_GPIO, LORA_RADIO_DIO_IRQ_PRIORITY, irqHandlers[2]);
  gpio_set_irq_handler(LORA_DIO3_GPIO, LORA_RADIO_DIO_IRQ_PRIORITY, irqHandlers[3]);
}

/**
 * Apply, or not the reset signal.
 *
 * @param[in] reset apply reset signal?
 */
static void lorawan_sx1272_com_reset(bool reset)
{
  GPIO_InitTypeDef init;

  init.Pull  = GPIO_NOPULL;
  init.Speed = GPIO_SPEED_HIGH;

  if(reset)
  {
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(    gpio_hal_port_and_pin_from_id(LORA_RESET_GPIO, &init.Pin), &init);
    HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_RESET_GPIO, &init.Pin),
		      init.Pin,
		      GPIO_PIN_SET);
  }
  else
  {
    init.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(LORA_RESET_GPIO, &init.Pin), &init);
  }
}

/**
 * Enable CHIP for SPI communication.
 */
static void lorawan_sx1272_com_start()
{
  uint32_t pin;

  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_NSS_GPIO, &pin), pin, GPIO_PIN_RESET);
}

/**
 * Disable CHIP for SPI communication.
 */
static void lorawan_sx1272_com_stop()
{
  uint32_t pin;

  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(LORA_NSS_GPIO, &pin), pin, GPIO_PIN_SET);
}


/**
 * Read bytes from radio over SPI.
 *
 * @pre the communication MUST have been started (NSS).
 *
 * @param[in] pu8_buffer the buffer where to write the data read from the radio. MUST be NOT NULL.
 * @param[in] size       the number of bytes to read.
 *
 * @return pu8_buffer.
 * @return NULL in case of error.
 */
static uint8_t *lorawan_sx1272_com_read(uint8_t *pu8_buffer, uint16_t size)
{
  uint8_t *pu8;
  uint8_t  dummy = 0;

  // Read
  for(pu8 = pu8_buffer; size; size--, pu8++)
  {
    if(HAL_SPI_TransmitReceive(&_lorawan_spi, &dummy, pu8, 1, LORA_RADIO_SPI_TIMEOUT_MS) != HAL_OK)
    {
      pu8_buffer = NULL;
      goto exit;
    }
  }

  exit:
  return pu8_buffer;
}

/**
 * Write bytes to the radio over SPI.
 *
 * @pre the communication MUST have been started (NSS).
 *
 * @param[in] pu8_data the data to write. MUST be NOT NULL.
 * @param[in] size the number of data bytes to write.
 *
 * @return true  on success.
 * @return false otherwise.
 */
static bool lorawan_sx1272_com_write(const uint8_t *pu8_data, uint16_t size)
{
  uint8_t dummy = 0;
  bool    res   = true;

  // Write
  for( ; size; size--, pu8_data++)
  {
    if(HAL_SPI_TransmitReceive(&_lorawan_spi,
			       (uint8_t *)pu8_data,
			       &dummy,
			       1,
			       LORA_RADIO_SPI_TIMEOUT_MS) != HAL_OK)
    {
      res = false;
      goto exit;
    }
  }

  exit:
  return res;
}


static uint32_t lorawan_SX1272GetWakeTime()
{
  return  0;  // No wakeup time
}

static void lorawan_SX1272SetXO(uint8_t state)
{
  UNUSED(state);
  // Do nothing
}

static void lorawan_SX1272SetRfTxPower(int8_t power)
{
    uint8_t paConfig = 0;
    uint8_t paDac    = 0;

    paConfig = SX1272Read(REG_PACONFIG);
    paDac    = SX1272Read(REG_PADAC);

    paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK) |
	lorawan_SX1272GetPaSelect(SX1272.Settings.Channel);

    if((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST)
    {
      paDac  = paDac & RF_PADAC_20DBM_MASK;
      paDac |= (power > 17) ? RF_PADAC_20DBM_ON: RF_PADAC_20DBM_OFF;

      paConfig = paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK;
      if((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON)
      {
	if(power < 5)  { power = 5;  }
	if(power > 20) { power = 20; }
	paConfig |= (uint8_t)((uint16_t)(power - 5) & 0x0F);
      }
      else
      {
	if(power < 2)  { power = 2;  }
	if(power > 17) { power = 17; }
	paConfig |= (uint8_t)((uint16_t)(power - 2) & 0x0F);
      }
    }
    else
    {
      if(power < -1) { power = -1; }
      if(power > 14) { power = 14; }
      paConfig = (paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power + 1) & 0x0F);
    }
    SX1272Write(REG_PACONFIG, paConfig);
    SX1272Write(REG_PADAC,    paDac);
}

static uint8_t lorawan_SX1272GetPaSelect(uint32_t channel)
{
  UNUSED(channel);
  return RF_PACONFIG_PASELECT_RFO;
}

static void lorawan_SX1272SetAntSwLowPower(bool status)
{
  UNUSED(status);
  // Do nothing: ant Switch Controlled by SX1272 IC
}

static void lorawan_SX1272SetAntSw(uint8_t opMode)
{
  switch(opMode)
  {
    case RFLR_OPMODE_TRANSMITTER:
      SX1272.RxTx = 1;
      break;

    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
    default:
      SX1272.RxTx = 0;
      break;
  }
}

static bool lorawan_SX1272CheckRfFrequency(uint32_t frequency)
{
  UNUSED(frequency);
  // Implement check. Currently all frequencies are supported
  return true;
}

