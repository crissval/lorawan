/**
 * Defines the common interface to all network interfaces.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#pragma once

#include "periodic.hpp"
#include "json.hpp"



class ClassNetwork : public ClassPeriodic
{
public:
  class EventClient;

  /**
   * Define the flags used to identifier the quality of service values
   */
  typedef enum QoSFlag
  {
    QOS_FLAG_NONE      = 0x00,
    QOS_RSSI_DBM       = (1u << 0),
    QOS_SNR_DB         = (1u << 1),
    QOS_SPECIFIC_START = (1u << 8),  ///< First value to use for network specific QoS values.
    QOS_FLAG_ALL       = 0xFF
  }
  QoSFlag;
  typedef uint32_t QoSFlags;  ///< The type used to store QoS flags into a single variable.

  /**
   * The type used to store quality of service values.
   */
  typedef struct QoSValues
  {
    QoSFlags flags; ///< A ORed combination of QoSFlag that indicate which values are set.

    int16_t  rssi;  ///< The RSSI.
    int8_t   snr;   ///< The SNR.
  }
  QoSValues;
#define network_qos_values_contains(pvValues, flags) ((pvValues)->flags & (flags))

  /**
   * Define the interface join states.
   */
  typedef enum JoinState
  {
    JOIN_STATUS_NONE,     ///< The interface has joined no network.
    JOIN_STATUS_JOINING,  ///< In the process of joining a network.
    JOIN_STATUS_JOINED,   ///< The interface has joined a network.
    JOIN_STATUS_TIMEOUT,  ///< Join timeout.
    JOIN_STATUS_FAILED,   ///< The last join try failed.
    JOIN_STATUS_LEFT      ///< We have left the network.
  }
  JoinState;

  /**
   * Define the send states.
   */
  typedef enum SendState
  {
    SEND_STATE_NONE,
    SEND_STATE_SENDING,
    SEND_STATE_SENT,
    SEND_STATE_ACKNOWLEDGED,
    SEND_STATE_ACK_FAILED,
    SEND_STATE_TIMEOUT,
    SEND_STATE_FAILED
  }
  SendState;

  /**
   * Defines the send options available.
   */
  typedef enum SendOptionFlag
  {
    SEND_OPTION_NONE            = 0,
    SEND_OPTION_DEFAULT         = 1u << 0, ///< Use the default (current) options.
    SEND_OPTION_REQUEST_ACK     = 1u << 1, ///< Request acknowledge for data sent.
    SEND_OPTION_NO_ACK          = SEND_OPTION_NONE,
    SEND_OPTION_SLEEP_WHEN_DONE = 1u << 2  ///< Go to sleep when done tying to send data.
  }
  SendOptionFlag;
  typedef uint8_t SendOptions;  ///< A ORed combination of SendOptionFlag values.


protected:
  static const char *QOS_VALUES_STRING_SEP_STR;  ///< The separator string used between QoS values in a QoS values string.

  /*
   * Defines the type of sending periods that can be used.
   */
  typedef enum PeriodType
  {
    PERIOD_DEFAULT,       ///< The default (normal) period.
    PERIOD_SENSOR_ALARM   ///< The period defined when a sensor is in alarm.
  }
  PeriodType;


public :
  ClassNetwork();
  virtual ~ClassNetwork();

  virtual const char *typeName() const = 0;
  const char *        name();

  void useSensorInAlarmPeriod(bool use = true);


  /**
   * Return the identifier of the device in the network as a string.
   *
   * @return the identifier.
   * @return an empty string if there is no such thing as a device identifier for this type
   *         of network.
   */
  virtual const char *deviceIdAsString();

  virtual bool setConfiguration(const JsonObject &json);
  void         setJoinTimeoutSec(uint32_t secs) { this->_joinTimeoutMs = secs * 1000; }
  void         setSendTimeoutSec(uint32_t secs) { this->_sendTimeoutMs = secs * 1000; }
  void         setDefaultSendOptions(SendOptions options) { this->_defaultSendOptions = options; }
  void         setQoSValuesToPrintOnFrameRx(QoSFlags values);

  /**
   * Function called to process tasks awaiting to be done, if there are any.
   *
   * @return true  if there are more processing left to do.
   * @return false if all the tasks have been done.
   */
  virtual bool process();

  bool isBusy();

  virtual void join(     bool sleepWhenDone);
  virtual void leave(    bool sleepWhenDone);
  virtual void rejoin(   bool sleepWhenDone);
  JoinState    joinState(void) const { return this->_joinState; }
  SendState    sendState(void) const { return this->_sendState; }

  bool  open();
  void  close();
  bool  isOpened() { return this->_isOpened; }
  void  sleep();
  void  wakeup();
  bool  send(const uint8_t *pu8_data, uint16_t size, SendOptions options);

  virtual uint32_t maxPayloadSize() = 0;

  /**
   * Get quality of service values.
   *
   * @param[out] pvValues    where the values are written to. MUST be NOT NULL.
   * @param[in]  valuesToGet a ORed combination of QoSFlag indicating the values we are interested in.
   *
   * @return true  if at least one value has been retrieved.
   * @return false otherwise.
   */
  virtual bool getQoSValues(QoSValues *pvValues, QoSFlags valuesToGet = QOS_FLAG_ALL);

  virtual char *QoSValuesToString(char *psBuffer, uint16_t size, const QoSValues *pvValues);

  void registerEventClient(EventClient *pvClient);


protected:
  void setJoinState( JoinState state);
  void setSendState( SendState state);
  void receivedFrame();
  void receivedSendAck(bool received = true);

  virtual void  cancel();
  virtual bool  openSpecific()   = 0;
  virtual void  closeSpecific()  = 0;
  virtual void  sleepSpecific()  = 0;
  virtual void  wakeupSpecific() = 0;
  virtual bool  joinSpecific();
  virtual void  leaveSpecific();
  virtual bool  sendSpecific(const uint8_t *pu8_data,
			     uint16_t       size,
			     SendOptions    options) = 0;


private:
  void resetState();
  void setUsePeriod(PeriodType type);


private:
  bool        _isOpened;                   ///< Is the interface opened or not?
  bool        _isAsleep;                   ///< Is the interface asleep or not?
  bool        _sleepWhenDone;              ///< Go to sleep when current operation is done?
  JoinState   _joinState;                  ///< The interface's join state.
  uint32_t    _joinTimeoutMs;              ///< Maximum time, in milliseconds, to wait for join. 0 to disable timeout.
  uint32_t    _rejoinOnSendFailedCount;    ///< The number of send data error that triggers a rejoin. 0 deactivate this functionality.
  SendState   _sendState;                  ///< The send state.
  SendOptions _defaultSendOptions;         ///< The default send options to use.
  uint32_t    _sendTimeoutMs;              ///< The maximum time, in milliseconds, to wait for a successful send. 0 to disable timeout.
  uint32_t    _sendTryCount;               ///< The number of times we have tried to send data.
  uint32_t    _sendFailedCount;            ///< The number of send data errors.
  QoSFlags    _qosValuesToPrintOnFrameRx;  ///< QoS values to print in logs at each frame reception.
  EventClient *_pvEventClients;            ///< Head of the event clients list.
  PeriodType _periodType;                  ///< The type of send period to use.
  uint32_t   _periodDefaultSec;            ///< The default period, in seconds.
  uint32_t   _periodSensorInAlarmSec;      ///< Period, in seconds, to use when a sensor is in alarm.


  /**
   * Base class for network events clients.
   */
public:
  class EventClient
  {
  public:
    virtual void joinStateChanged(ClassNetwork *pvInterface, JoinState state);
    virtual void sendStateChanged(ClassNetwork *pvInterface, SendState state);

  public:
    EventClient *_pvNext;  ///< Next event client in the list.
  };
};



