/**
 * Network base class.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "network.hpp"
#include "connecsens.hpp"
#include "logger.h"
#include "board.h"
#include "powerandclocks.h"


  CREATE_LOGGER(network);
#undef  logger
#define logger  network


#ifndef NETWORK_JOIN_TIMEOUT_MS_DEFAULT
#define NETWORK_JOIN_TIMEOUT_MS_DEFAULT  120000  // 120 seconds
#endif

#ifndef NETWORK_SEND_TIMEOUT_MS_DEFAULT
#define NETWORK_SEND_TIMEOUT_MS_DEFAULT  60000  // 60 seconds
#endif

#ifndef NETWORK_REJOIN_ON_SEND_FAILED_COUNT
#define NETWORK_REJOIN_ON_SEND_FAILED_COUNT  24
#endif


  const char *ClassNetwork::QOS_VALUES_STRING_SEP_STR = ", ";


ClassNetwork::ClassNetwork()
{
  this->_joinTimeoutMs             = NETWORK_JOIN_TIMEOUT_MS_DEFAULT;
  this->_sendTimeoutMs             = NETWORK_SEND_TIMEOUT_MS_DEFAULT;
  this->_rejoinOnSendFailedCount   = NETWORK_REJOIN_ON_SEND_FAILED_COUNT;
  this->_defaultSendOptions        = SEND_OPTION_NONE;
  this->_qosValuesToPrintOnFrameRx = QOS_FLAG_NONE;
  this->_pvEventClients            = NULL;
  this->_periodType                = PERIOD_DEFAULT;
  this->_periodDefaultSec          = 0;
  this->_periodSensorInAlarmSec    = 0;

  resetState();
}

ClassNetwork::~ClassNetwork()
{
  close();
}

/**
 * Reset internal state.
 */
void ClassNetwork::resetState()
{
  this->_isOpened        = false;
  this->_isAsleep        = false;
  this->_sleepWhenDone   = false;
  this->_joinState       = JOIN_STATUS_NONE;
  this->_sendState       = SEND_STATE_NONE;
  this->_sendTryCount    = 0;
  this->_sendFailedCount = 0;
}


const char *ClassNetwork::deviceIdAsString() { return ""; }

/**
 * Set the QoS values to print in logs after a frame has been received.
 */
void ClassNetwork::setQoSValuesToPrintOnFrameRx(QoSFlags values)
{
  this->_qosValuesToPrintOnFrameRx = values;
}


/**
 * Set the type of period to use.
 *
 * If the type of period has changed then the next send time is updated.
 *
 * @param[in] type The type of send period to use.
 */
void ClassNetwork::setUsePeriod(PeriodType type)
{
  uint32_t periodSecPrev;

  if(type != this->_periodType)
  {
    periodSecPrev     = periodSec();
    this->_periodType = PERIOD_DEFAULT;
    setPeriodSec(this->_periodDefaultSec);
    switch(type)
    {
      case PERIOD_DEFAULT: break;  // Do nothing

      case PERIOD_SENSOR_ALARM:
	if(this->_periodSensorInAlarmSec)
	{
	  this->_periodType = PERIOD_SENSOR_ALARM;
	  setPeriodSec(this->_periodSensorInAlarmSec);
	}
	break;

      default:
	log_error(logger, "Unknown period type %d, use default period.", type);
	break;
    }
    if(periodSec() != periodSecPrev)
    {
      log_debug(logger, "Send period has been set to %lu seconds.", periodSec());
    }
  }
}


/**
 * Set, or not, the network interface to use the send period defined for
 * when a sensor is in alarm. If such a period is defined.
 *
 * @param[in] use Use the sensor in alarm period?
 */
void ClassNetwork::useSensorInAlarmPeriod(bool use)
{
  if(!this->_periodSensorInAlarmSec) { goto exit; }

  if(      use && this->_periodType != PERIOD_SENSOR_ALARM)
  {
    setUsePeriod(PERIOD_SENSOR_ALARM);
    log_debug(logger, "Use sensor in alarm send period.");
  }
  else if(!use && this->_periodType == PERIOD_SENSOR_ALARM)
  {
    setUsePeriod(PERIOD_DEFAULT);
  }

  exit:
  return;
}


bool ClassNetwork::setConfiguration(const JsonObject &json)
{
  uint32_t u32;
  int32_t  i32;
  bool     ok;
  bool     res = true;

  this->_periodDefaultSec = ConnecSenS::getPeriodSec(json);
  setPeriodSec(this->_periodDefaultSec);

  this->_periodSensorInAlarmSec =
      ConnecSenS::getPeriodSec(json, 0, NULL, "periodWhenAlarm");

  u32 = ConnecSenS::getPeriodSec(json, periodSec() / 4, &ok, "periodSpread");
  if(!ok || u32 != 0) { setPeriodSpreadSec(u32); }

  u32 = ConnecSenS::getPeriodSec(json, NETWORK_JOIN_TIMEOUT_MS_DEFAULT, &ok, "joinTimeout");
  if(ok) { setJoinTimeoutSec(u32); }

  u32 = ConnecSenS::getPeriodSec(json, NETWORK_SEND_TIMEOUT_MS_DEFAULT, &ok, "sendTimeout");
  if(ok) { setSendTimeoutSec(u32); }

  if(json["rejoinOnSendFailsCount"].success())
  {
    i32 = json["rejoinOnSendFailsCount"].as<int32_t>();
    if(i32 < 0)
    {
      log_warn(logger, "Configuration parameter 'rejoinOnSendFailsCount' cannot be negative; use default value: %l.", this->_rejoinOnSendFailedCount);
    }
    else { this->_rejoinOnSendFailedCount = (uint32_t)i32; }
  }

  return res;
}


/**
 * Open the network interface.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool ClassNetwork::open()
{

  if(this->_isOpened) {  goto exit; }
  log_debug(logger, "Opening network interface...");

  resetState();

  this->_isOpened      = openSpecific();
  if(this->_isOpened) { log_info( logger, "Network interface is open.");        }
  else                { log_error(logger, "Failed to open network interface."); }

  exit:
  return this->_isOpened;
}

/**
 * Close the network interface.
 */
void ClassNetwork::close()
{
  if(this->_isOpened)
  {
    log_debug(logger, "Closing network interface...");
    leave(false);
    closeSpecific();
    resetState();
    log_info(logger, "Network interface has been closed.");
  }
}

/**
 * Put interface to sleep
 */
void ClassNetwork::sleep()
{
  if(this->_isOpened && !this->_isAsleep)
  {
    if( this->_joinState == JOIN_STATUS_JOINING ||
	this->_sendState == SEND_STATE_SENDING)
    {
      log_debug(logger, "Cannot go to sleep; the network interface is still active.");
    }
    else
    {
      log_debug(logger, "Network interface is put to sleep.");
      sleepSpecific();
      this->_isAsleep = true;
    }
  }
  else
  {
    log_debug(logger, "Do not go to sleep; network interface is not opened or already is alseep.");
  }
}

/**
 * Wakeup interface from sleep.
 */
void ClassNetwork::wakeup()
{
  // Make sure that the interface is opened
  open();

  if(this->_isAsleep)
  {
    log_debug(logger, "Wake network interface up from sleep.");
    wakeupSpecific();
    this->_isAsleep = false;
  }
}


bool ClassNetwork::process()
{
  // Do nothing
  return false;
}


bool ClassNetwork::isBusy()
{
  return
      this->_joinState != JOIN_STATUS_JOINING &&
      this->_sendState != SEND_STATE_SENDING;
}

/**
 * Cancel the current network operation.
 */
void ClassNetwork::cancel()
{
  if(joinState() == JOIN_STATUS_JOINING)
  {
    setJoinState(JOIN_STATUS_FAILED);
  }

  if(sendState() == SEND_STATE_SENDING)
  {
    setSendState(SEND_STATE_FAILED);
  }
}


/**
 * Try to join a network.
 *
 * @param[in] go to sleep when join try is done?
 */
void ClassNetwork::join(bool sleepWhenDone)
{
  uint32_t ms_ref;

  if(this->_joinState == JOIN_STATUS_JOINING || this->_joinState == JOIN_STATUS_JOINED)
  { goto exit; }

  this->_sendTryCount    = 0;
  this->_sendFailedCount = 0;
  this->_sleepWhenDone   = sleepWhenDone;

  // Make sure that we are awake
  wakeup();

  // Start join
  setJoinState(JOIN_STATUS_JOINING);
  if(!joinSpecific())
  {
    setJoinState(JOIN_STATUS_FAILED);
    goto exit;
  }

  // Wait for join to end?
  if(this->_joinTimeoutMs)
  {
    // Wait
    ms_ref = board_ms_now();
    while(joinState() == JOIN_STATUS_JOINING)
    {
      pwrclk_sleep_ms_max(1000);
      ConnecSenS::yield();  // It includes calling the network process() function.
      if(board_is_timeout(ms_ref, this->_joinTimeoutMs)) break;
    }
    if(joinState() == JOIN_STATUS_JOINING)
    {
      setJoinState(JOIN_STATUS_TIMEOUT);
      cancel(); // Cancel join operation
    }
  }

  exit:
  return;
}

/**
 * Default implementation of the join function.
 *
 * Switch to joined state.
 */
bool ClassNetwork::joinSpecific(void)
{
  setJoinState(JOIN_STATUS_JOINED);
  return true;
}

/**
 * Leave a network.
 *
 * @param[in] go to sleep when join try is done?
 */
void ClassNetwork::leave(bool sleepWhenDone)
{
  if(this->_joinState == JOIN_STATUS_JOINED || this->_joinState == JOIN_STATUS_JOINING)
  {
    this->_sleepWhenDone = sleepWhenDone;
    log_debug(logger, "Leaving network...");
    leaveSpecific();
    setJoinState(JOIN_STATUS_LEFT);
  }
}

/**
 * Default implementation of the leave network function.
 *
 * Switch to no join state.
 */
void ClassNetwork::leaveSpecific(void)
{
  // Do nothing
}

/**
 * Rejoin a network.
 *
 * @param[in] go to sleep when join try is done?
 */
void ClassNetwork::rejoin(bool sleepWhenDone)
{
  leave(false);
  join( sleepWhenDone);
}

/**
 * Set the join state.
 */
void ClassNetwork::setJoinState(JoinState state)
{
  EventClient *pvec;
  JoinState    nextState;

  if(state == this->_joinState) return;

  this->_joinState = nextState = state;
  switch(state)
  {
    case JOIN_STATUS_JOINING:
      log_debug(logger, "Joining network...");
      break;

    case JOIN_STATUS_JOINED:
      log_info(logger, "Joined network.");
      receivedFrame();
      if(this->_sleepWhenDone) { sleep(); }
      break;

    case JOIN_STATUS_TIMEOUT:
      log_error(logger, "Join timeout.");
      nextState = JOIN_STATUS_FAILED;
      break;

    case JOIN_STATUS_FAILED:
      log_error(logger, "Failed to join network.");
      if(this->_sleepWhenDone) { sleep(); }
      break;

    case JOIN_STATUS_LEFT:
      log_info(logger, "We have left the network.");
      if(this->_sleepWhenDone) { sleep(); }
      break;

    case JOIN_STATUS_NONE:
      // Do nothing; but this should not happen
      break;

    default:
      log_fatal(logger, "Unknown join state: %u.", state);
  }

  // Signal event clients
  for(pvec = this->_pvEventClients; pvec; pvec = pvec->_pvNext)
  {
    pvec->joinStateChanged(this, state);
  }

  // Some state changes may still need to be done.
  if(nextState != state) { setJoinState(nextState); }
}


/**
 * Send data.
 *
 * @param[in] pu8_data the data to send. MUST be NOT NULL.
 * @param[in] size     the number of data bytes to send.
 * @param[in] options  the send options to use.
 *                     Use value SEND_OPTION_DEFAULT to use the current default send options.
 *
 * @return true  if the data have been sent. But it does not mean that we have received
 *               an acknowledge for it.
 * @return false otherwise.
 */
bool ClassNetwork::send(const uint8_t *pu8_data, uint16_t size, SendOptions options)
{
  uint32_t  ms_ref;
  bool      sleepWhenDoneBak = this->_sleepWhenDone;

  // Are we already sending data?
  if(this->_sendState == SEND_STATE_SENDING)
  {
    log_error(logger, "Cannot send data; we already are sending data.");
    return false;
  }
  this->_sleepWhenDone = false;

  // Make sure that we are awake
  wakeup();

  // Do we have to trigger a join or a rejoin?
  if( this->_rejoinOnSendFailedCount &&
      this->_sendFailedCount >= this->_rejoinOnSendFailedCount)
  {
    // Even make an open/close to be sure.
    log_info(logger, "At least %d data send failed; rejoin network.", this->_rejoinOnSendFailedCount);
    close();
    open();
  }
  if(this->_joinState != JOIN_STATUS_JOINED)
  {
    join(false);
    sleepWhenDoneBak = true;
  }
  if(this->_joinState == JOIN_STATUS_JOINING)
  {
    log_info(logger, "Cannot send data; join is still in progress.");
    goto error_exit;
  }

  // Send data
  setSendState(SEND_STATE_SENDING);
  if(options == SEND_OPTION_DEFAULT) { options = this->_defaultSendOptions; }
  this->_sleepWhenDone = ((options & SEND_OPTION_SLEEP_WHEN_DONE) != 0);
  if(!sendSpecific(pu8_data, size, options))
  {
    setSendState(SEND_STATE_FAILED);
    goto error_exit;
  }
  this->_sendTryCount++;

  // Wait for acknowledge if we have been asked to.
  if((options & SEND_OPTION_REQUEST_ACK) && this->_sendTimeoutMs)
  {
    ms_ref = board_ms_now();
    while(this->_sendState == SEND_STATE_SENDING)
    {
      pwrclk_sleep_ms_max(1000);
      ConnecSenS::yield();  // Includes call to the network's process() function.
      if(board_is_timeout(ms_ref, this->_sendTimeoutMs)) break;
    }
    if(this->_sendState == SEND_STATE_SENDING)
    {
      setSendState(SEND_STATE_TIMEOUT);
      cancel();  // Cancel send operation.
      goto error_exit;
    }
  }
  else { setSendState(SEND_STATE_SENT); }

  return true;

  error_exit:
  this->_sleepWhenDone = sleepWhenDoneBak;
  return false;
}

/**
 * Call this function to indicate that we received an acknowledge for
 * the last send command or if we cannot expect to receive any more,
 * if all hope is lost.
 *
 * @param[in] received indicate if if we received the acknowledge or not.
 */
void ClassNetwork::receivedSendAck(bool received)
{
  if(this->_sendState == SEND_STATE_SENDING)
  {
    setSendState(received ? SEND_STATE_ACKNOWLEDGED : SEND_STATE_ACK_FAILED);
  }
  else
  {
    log_warn(logger, "Received acknowledge for last data sent; we were not expecting it.");
  }
}

/**
 * Set the current send state
 */
void ClassNetwork::setSendState(SendState state)
{
  EventClient *pvec;
  SendState    nextState;

  if(state == this->_sendState) return;

  this->_sendState = nextState = state;
  switch(state)
  {
    case SEND_STATE_SENDING:
      log_info(logger, "Sending data...");
      break;

    case SEND_STATE_SENT:
      this->_sendFailedCount = 0;
      log_info(logger, "Data have been sent.");
      if(this->_sleepWhenDone) { sleep(); }
      break;

    case SEND_STATE_ACKNOWLEDGED:
      log_debug(logger, "Received acknowledgment");
      receivedFrame();
      nextState = SEND_STATE_SENT;
      break;

    case SEND_STATE_ACK_FAILED:
      log_error(logger, "No acknowledgment has been received.");
      nextState = SEND_STATE_FAILED;
      break;

    case SEND_STATE_TIMEOUT:
      log_error(logger, "Data send timeout.");
      nextState = SEND_STATE_FAILED;
      break;

    case SEND_STATE_FAILED:
      if(this->_sendTryCount)  { this->_sendFailedCount++; }
      if(this->_sleepWhenDone) { sleep(); }
      log_error(logger, "Failed to send data.");
      break;

    case SEND_STATE_NONE:
      // Should not happen
      if(this->_sleepWhenDone) { sleep(); }
      break;

    default:
      log_fatal(logger, "Unknown send state: %u.", state);
      break;
  }

  // Signal event clients
  for(pvec = this->_pvEventClients; pvec; pvec = pvec->_pvNext)
  {
    pvec->sendStateChanged(this, state);
  }

  // Some state changes may still need to be done.
  if(nextState != state) { setSendState(nextState); }
}


/**
 * Function called when a frame has been received, whatever it's type.
 */
void ClassNetwork::receivedFrame()
{
  char      buffer[128];
  QoSValues qosValues;

  // Print QoS values to log
  if(this->_qosValuesToPrintOnFrameRx != QOS_FLAG_NONE)
  {
    if(getQoSValues(&qosValues, this->_qosValuesToPrintOnFrameRx))
    {
      QoSValuesToString(buffer, sizeof(buffer), &qosValues);
      log_info(logger, "Received frame, QoS: %s.", buffer);
    }
  }
}


/**
 * Default implementation of the function.
 *
 * Does nothing.
 *
 * @return false. There are no values to return by default.
 */
bool ClassNetwork::getQoSValues(QoSValues *pvValues, QoSFlags valuesToGet)
{
  UNUSED(valuesToGet);
  pvValues->flags = QOS_FLAG_NONE;
  // Do nothing
  return false;
}

/**
 * Return a string representing QoS values.
 *
 * If the buffer is too small then the string is truncated.
 * This is a default implementation that does pretty much nothing.
 *
 * @param[out] psBuffer the buffer where to write the string. MUST be NOT NULL.
 * @param[in]  size     the buffer's size. MUST be > 0.
 * @param[in]  pvValues the QoS values to print. MUST be NOT NULL.
 *
 * @return the string (psBuffer).
 */
char *ClassNetwork::QoSValuesToString(char            *psBuffer,
				      uint16_t         size,
				      const QoSValues *pvValues)
{
  uint16_t l;
  uint16_t len = 0;

  psBuffer[0] = '\0';

  if(pvValues->flags & QOS_RSSI_DBM)
  {
    len = snprintf(psBuffer, size, "RSSI: %i dBm", pvValues->rssi);
    if(len >= size) { goto exit; }
  }

  if(pvValues->flags & QOS_SNR_DB)
  {
    if(len)
    {
      l = strlcpy(psBuffer + len, QOS_VALUES_STRING_SEP_STR, size);
      if(l >= size) { goto exit; }
      len += l; size -= l;
    }
    l = snprintf( psBuffer + len, size, "SNR: %i dB", pvValues->snr);
    if(l >= size) { goto exit; }
    len += l; size -= l;
  }

  exit:
  return psBuffer;
}


/**
 * Register an events client.
 *
 * @param[in] pvClient the client. MUST be NOT NULL.
 */
void ClassNetwork::registerEventClient(EventClient *pvClient)
{
  EventClient *pvec ;

  // Check that the client is not already registered
  for(pvec = this->_pvEventClients; pvec; pvec = pvec->_pvNext)
  {
    if(pvec == pvClient) return;  // Client already registered.
  }

  // Add client
  pvClient->_pvNext     = this->_pvEventClients;
  this->_pvEventClients = pvClient;
}


// =============================== Inner classes =================================

/**
 * Default implementation for the "joinStateChanged event". Does nothing.
 *
 * @param[in] pvInterface the network interface source of the event.
 * @param[in] state       the new join state.
 */
void ClassNetwork::EventClient::joinStateChanged(ClassNetwork *pvInterface,
						 JoinState     state)
{
  UNUSED(pvInterface);
  UNUSED(state);
  // Do nothing
}

/**
 * Default implementation for the "sendStateChanged event". Does nothing.
 *
 * @param[in] pvInterface the network interface source of the event.
 * @param[in] state       the new send state.
 */
void ClassNetwork::EventClient::sendStateChanged(ClassNetwork *pvInterface,
						 SendState     state)
{
  UNUSED(pvInterface);
  UNUSED(state);
  // Do nothing
}




