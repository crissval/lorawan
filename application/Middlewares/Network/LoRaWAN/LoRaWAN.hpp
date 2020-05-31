
#pragma once

#include "network.hpp"
#include "timeServer.h"
#include "LoRaMac.h"


#define LORAWAN_DEVEUI_SIZE  8
#define LORAWAN_APPEUI_SIZE  8
#define LORAWAN_APPKEY_SIZE  16


class ClassLoRaWAN : public ClassNetwork
{
public:
  /**
   * Define QoS flags specific to LoRaWAN interface.
   */
  typedef enum QoSFlagSpecific
  {
    QOS_DR = QOS_SPECIFIC_START
  }
  QoSFlagSpecific;

  /**
   * Extend base class QoSValues structure to add LoRaWAN specific values.
   */
  typedef struct QoSValuesSpecific
  {
    QoSValues base;
    int8_t    dr;    ///< Datarate
  }
  QoSValuesSpecific;

  /**
   * List the LoRaWAN datarates.
   */
  typedef enum Datarate
  {
    DR_0,  DR_1,  DR_2,  DR_3,  DR_4,  DR_5, DR_6, DR_7, DR_8, DR_9,
    DR_10, DR_11, DR_12, DR_13, DR_14, DR_15
  }
  Datarate;

  /**
   * Defines the LoRaWAN MAC events' flag.
   */
  typedef enum MACEventFlag
  {
    MAC_EVT_NONE            = 0,
    MAC_EVT_JOINED          = 1u << 0,
    MAC_EVT_JOIN_FAILED     = 1u << 1,
    MAC_EVT_ACK_RECEIVED    = 1u << 2,
    MAC_EVT_NO_ACK_RECEIVED = 1u << 3
  }
  MACEventFlag;
  typedef uint8_t MACEvents;  ///< A ORed combination of MACEventFlag values.

  static const char *TYPE_NAME;  ///< The  network type's name.


public:
  ClassLoRaWAN();

  const char *typeName() const;
  const char *deviceIdAsString();

  bool     setConfiguration(const JsonObject &json);

  bool     process();
  void     addMacEvent(MACEventFlag evt) { this->_macEvents |= evt; }

  void     cancel();
  bool     openSpecific();
  void     closeSpecific();
  void     sleepSpecific();
  void     wakeupSpecific();
  uint32_t maxPayloadSize();
  uint32_t sendAckResponseTimeMaxMs();
  bool     joinSpecific();
  bool     sendSpecific(const uint8_t *pu8_data,
			uint16_t       size,
			SendOptions    options);

  bool  getQoSValues(QoSValues *pvValues, QoSFlags valuesToGet);
  char *QoSValuesToString(char *psBuffer, uint16_t size, const QoSValues *pvValues);

  void     setDatarate(Datarate dr);
  Datarate datarate();
  void     setDatarateRange(int8_t drMin, int8_t drMax);
  void     enableAdapatativeDatarate(bool enable = true);


private:
  bool requestClass(DeviceClass_t newClass);

  static void    mcpsConfirm(   McpsConfirm_t    *pvMcpsConfirm);
  static void    mcpsIndication(McpsIndication_t *pvMcpsIndication);
  static void    mlmeConfirm(   MlmeConfirm_t    *pvMlmeConfirm);
  static uint8_t batteryLevel();


private:
  char          _psDevEUI[LORAWAN_DEVEUI_SIZE * 2 + 1]; ///< The DevEUI as an hexa string.
  int8_t        _datarateMin;           ///< The minimum Tx datarate that we can use.
  int8_t        _datarateMax;           ///< The maximum Tx datarate that we can use.
  int8_t        _defaultDatarate;       ///< The default datarate to use.
  uint8_t       _failedSendCount;       ///< Count the number of failed send with the current join credentials.
  int8_t        _joinDatarate;          ///< The datarate to use for the join.
  uint8_t       _joinNbTrials;          ///< The number of tries to join the network.
  uint8_t       _sendAckNbTrials;       ///< The number of tries to get an acknowledge of data sent.
  bool          _enablePublicNetwork;   ///< Use public (true) or private (false) network?
  bool          _enableADR;             ///< Enable adaptative datarate?
  MACEvents     _macEvents;             ///< MAC events awaiting to be processed.

  LoRaMacPrimitives_t _loramacPrimitives;  ///< Store callback functions for LoRaWAN MAC layer.
  LoRaMacCallback_t   _loramacCallbacks;   ///< Other callback functions for LoRaWAN MAC layer.

  uint8_t _devEUI[LORAWAN_DEVEUI_SIZE];  ///< LoRaWAN devEUI.
  uint8_t _appEUI[LORAWAN_APPEUI_SIZE];  ///< LoRaWAN appEUI.
  uint8_t _appKey[LORAWAN_APPKEY_SIZE];  ///< LoRaWAN appKey.

  uint8_t _dataBuffer[256];  ///< The LoRaWAN data buffer.

  static ClassLoRaWAN *pvInstance;  ///< The unique instance of this class.

  static const char *loramacStatusToString[];
  static const char *loramacRegionIdToString[];
};

