#include "gps.hpp"
#include "board.h"
#include "connecsens.hpp"
#include "rtc.h"
#include "logger.h"


#ifndef GPS_TIMEOUT_FIX_SEC
#define GPS_TIMEOUT_FIX_SEC  300
#endif
#define TIMEOUT_FIX_MIN_SEC  60
#ifndef GPS_TIMEOUT_BLOCKING_FIX_SEC
#define GPS_TIMEOUT_BLOCKING_FIX_SEC  600
#endif
#define TIMEOUT_BLOCKING_FIX_MIN_SEC  TIMEOUT_FIX_MIN_SEC


  CREATE_LOGGER(gps);
#undef  logger
#define logger  gps


ClassGPS::ClassGPS() : _myBuffer(200)
{
  this->_refreshFlag              = REFRESHED_NONE;
  this->_readingId                = 0;
  this->_refreshTimeoutMs         = GPS_TIMEOUT_FIX_SEC          * 1000;
  this->_blockingRefreshTimeoutMs = GPS_TIMEOUT_BLOCKING_FIX_SEC * 1000;
}


/**
 * Configure the GPS using json data.
 *
 * @param[in] json the json data.
 *
 * @return true on success.
 * @return false otherwise.
 */
bool ClassGPS::setConfiguration(const JsonObject &json)
{
  bool res = true;

  setPeriodSec(ConnecSenS::getPeriodSec(json, 0, NULL, "period"));
  if(!periodSec())
  {
    log_error(logger,
	      "When the GPS is used you MUST define it's usage period with "
	      "parameter 'periodSec', 'periodMn', 'periodHr' or 'periodDay' "
	      "and with a value > 0.");
    res = false;
  }

  if(     json["timeoutSec"].success())
  {
    setRefreshTimeoutSec(json["timeoutSec"].as<uint32_t>());
  }
  else if(json["timeoutMn"] .success())
  {
    setRefreshTimeoutSec((uint32_t)(60 * json["timeoutMn"].as<float>()));
  }

  if(     json["blockingTimeoutSec"].success())
  {
    setBlockingRefreshTimeoutSec(json["blockingTimeoutSec"].as<uint32_t>());
  }
  else if(json["blockingTimeoutMn"] .success())
  {
    setBlockingRefreshTimeoutSec((uint32_t)(60 * json["blockingTimeoutMn"].as<float>()));
  }

  return res;
}


void ClassGPS::open()
{
  GPIO_InitTypeDef init;
  GPIO_TypeDef    *pvONOFFPort, *pvWKUPPort;
  uint32_t         ONOFFPin,     WKUPPin;

  // Enable clock for USART
  PASTER3(__HAL_RCC_USART, GPS_A2135_USART_ID, _CLK_ENABLE)();

  gpio_use_gpios_with_ids(GPS_A2135_TX_GPIO,  GPS_A2135_RX_GPIO,
			  GPS_A2135_PPS_GPIO, GPS_A2135_PWR_GPIO,
			  GPS_A2135_RST_GPIO, GPS_A2135_ONOFF_GPIO,
			  GPS_A2135_WKUP_GPIO);

  // USART TX and RX
  init.Alternate = GPS_A2135_USART_AF;
  init.Mode      = GPIO_MODE_AF_PP;
  init.Pull      = GPIO_NOPULL;
  init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_RX_GPIO,    &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_TX_GPIO,    &init.Pin), &init);
  // PPS, WAKEUP
  init.Alternate = 0;
  init.Mode      = GPIO_MODE_INPUT;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_PPS_GPIO,   &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_WKUP_GPIO,  &init.Pin), &init);
  // PWR, RST, ON-OFF
  init.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_PWR_GPIO,   &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_RST_GPIO,   &init.Pin), &init);
  HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(GPS_A2135_ONOFF_GPIO, &init.Pin), &init);

  // Power On Module
  pvONOFFPort = gpio_hal_port_and_pin_from_id(GPS_A2135_ONOFF_GPIO, &ONOFFPin);
  pvWKUPPort  = gpio_hal_port_and_pin_from_id(GPS_A2135_WKUP_GPIO,  &WKUPPin);
  HAL_GPIO_WritePin(pvONOFFPort, ONOFFPin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(GPS_A2135_RST_GPIO, &init.Pin),
		    init.Pin,
		    GPIO_PIN_SET);
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(GPS_A2135_PWR_GPIO, &init.Pin),
		    init.Pin,
		    GPIO_PIN_SET);
  board_delay_ms(200);

  while(HAL_GPIO_ReadPin(pvWKUPPort, WKUPPin) != GPIO_PIN_SET)
  {
    HAL_GPIO_WritePin(pvONOFFPort, ONOFFPin, GPIO_PIN_SET);
    board_delay_ms(250);
    HAL_GPIO_WritePin(pvONOFFPort, ONOFFPin, GPIO_PIN_RESET);
    board_delay_ms(250);
  }
}

void ClassGPS::close()
{
  uint32_t pin;

  // Stop USART clock
  PASTER3(__HAL_RCC_USART, GPS_A2135_USART_ID, _CLK_DISABLE)();

  // Turn GPS power OFF
  HAL_GPIO_WritePin(gpio_hal_port_and_pin_from_id(GPS_A2135_PWR_GPIO, &pin),
		    pin,
		    GPIO_PIN_RESET);

  // Free GPIOs
  gpio_free_gpios_with_ids(GPS_A2135_TX_GPIO,  GPS_A2135_RX_GPIO,
			   GPS_A2135_PPS_GPIO, GPS_A2135_PWR_GPIO,
			   GPS_A2135_RST_GPIO, GPS_A2135_ONOFF_GPIO,
			   GPS_A2135_WKUP_GPIO);
}

void ClassGPS::initUART()
{
  /* Initialisation USART */
  this->_huart.Instance                    = PASTER2(USART, GPS_A2135_USART_ID);
  this->_huart.Init.BaudRate               = 4800;
  this->_huart.Init.WordLength             = UART_WORDLENGTH_8B;
  this->_huart.Init.StopBits               = UART_STOPBITS_1;
  this->_huart.Init.Parity                 = UART_PARITY_NONE;
  this->_huart.Init.Mode                   = UART_MODE_TX_RX;
  this->_huart.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
  this->_huart.Init.OverSampling           = UART_OVERSAMPLING_16;
  this->_huart.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
  this->_huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  HAL_UART_Init(&this->_huart);
}

void ClassGPS::deinitUART()
{
  HAL_UART_DeInit(&this->_huart);
}

void ClassGPS::setRefreshTimeoutSec(uint32_t timeoutSec)
{
  if(timeoutSec >= TIMEOUT_FIX_MIN_SEC) { this->_refreshTimeoutMs = timeoutSec * 1000; }
}

void ClassGPS::setBlockingRefreshTimeoutSec(uint32_t timeoutSec)
{
  if(timeoutSec >= TIMEOUT_BLOCKING_FIX_MIN_SEC)
  {
    this->_blockingRefreshTimeoutMs = timeoutSec * 1000;
  }
}

bool ClassGPS::refresh(bool blocking)
{
  uint8_t           data;
  HAL_StatusTypeDef status;
  bool              weHaveAFrame;
  uint32_t          timeoutMs;
  uint32_t          beginTime = HAL_GetTick();

  this->_refreshFlag = REFRESHED_NONE;
  timeoutMs          = blocking ? _blockingRefreshTimeoutMs : _refreshTimeoutMs;
  while(this->_refreshFlag         != REFRESHED_RMC &&
       (HAL_GetTick() - beginTime) <= timeoutMs)
  {
    initUART();
    this->_myBuffer.clean();
    weHaveAFrame = false;
    status       = HAL_OK;

    // Detect start of NMEA frame
    while(status != HAL_TIMEOUT && (HAL_GetTick() - beginTime) <= timeoutMs)
    {
      status = HAL_UART_Receive(&this->_huart, &data, 1, 2000);
      if(data == '$') { break; }
      if(blocking) { board_watchdog_reset(); }
      else         { ConnecSenS::yield();    }
    }

    // Get NMEA frame
    while(status != HAL_TIMEOUT && (HAL_GetTick() - beginTime) <= timeoutMs)
    {
      status = HAL_UART_Receive(&this->_huart, &data, 1, 2000);
      this->_myBuffer.push(data);
      if(data == '\n')
      {
	this->_myBuffer.push('\0');
	weHaveAFrame = true;
	break;
      }
      if(blocking) { board_watchdog_reset(); }
      else         { ConnecSenS::yield();    }
    }

    // Process frame
    if(weHaveAFrame) { this->NMEA_Handler((char *)this->_myBuffer.getBufferPtr()); }

    deinitUART();
  }

  return status != HAL_TIMEOUT && (HAL_GetTick() - beginTime) <= timeoutMs;
}

void ClassGPS::NMEA_Handler(const char *buffer)
{
  const char *pc, *pcEnd;
  uint8_t     cksum, cksumFrame;

  // Check the checksum
  // Look for the checksum delimiter character
  for(pc = buffer; *pc && *pc != '*'; pc++) { /* Do nothing */ }
  if(!pc[0] || !pc[1] || !pc[2]) { return; }
  pcEnd = pc;

  // Compute the checksum from the frame
  for(cksum = buffer[0], pc = buffer + 1; pc < pcEnd; pc++) { cksum ^= (uint8_t)*pc; }

  // Compare to the checksum in the frame
  for(cksumFrame = 0, pcEnd += 3, pc++; pc < pcEnd; pc++)
  {
    cksumFrame <<= 4;
    if(     *pc >= '0' && *pc <= '9') { cksumFrame |= (uint8_t)(*pc - '0');      }
    else if(*pc >= 'A' && *pc <= 'F') { cksumFrame |= (uint8_t)(*pc - 'A' + 10); }
    else if(*pc >= 'a' && *pc <= 'f') { cksumFrame |= (uint8_t)(*pc - 'a' + 10); }
    else
    {
      // Not a valid hexadecimal string
      return;
    }
  }
  if(cksum != cksumFrame)
  {
    // The checksums do not match.
    return;
  }

  // Process the frame
  if(buffer[2] == 'R' && buffer[3] == 'M' && buffer[4] == 'C') { RMC_Handler(buffer); }
}

void ClassGPS::GGA_Handler(const char *buffer) { UNUSED(buffer); }
void ClassGPS::GLL_Handler(const char *buffer) { UNUSED(buffer); }
void ClassGPS::GSA_Handler(const char *buffer) { UNUSED(buffer); }
void ClassGPS::GSV_Handler(const char *buffer) { UNUSED(buffer); }
void ClassGPS::VTG_Handler(const char *buffer) { UNUSED(buffer); }

void ClassGPS::RMC_Handler(const char *buffer)
{
  int         hours, minutes, seconds;
  const char *p, *ps;

  // Time
  p          = strchr(buffer, ',');
  float time = atof(p + 1);
  hours      = time / 10000;
  minutes    = time / 100 - hours * 100;
  seconds    = time       - hours * 10000 - minutes * 100;

  // Alert
  p = strchr(p + 1, ',');
  if(p[1] == 'A')
  {
    // Data is valid so copy time
    this->_time.hours   = hours;
    this->_time.minutes = minutes;
    this->_time.seconds = seconds;
  }
  else { return; }

  // Latitude
  ps = strchr(p + 1, ','); ps++;
  p  = strchr(ps,    ',');
  this->_location.latitude  = sexagecimalToFrac(ps, (uint8_t)(p - ps));
  if(p[1] == 'S') { this->_location.latitude  = -this->_location.latitude;  }

  // Longitude
  ps = strchr(p + 1, ','); ps++;
  p  = strchr(ps,    ',');
  this->_location.longitude = sexagecimalToFrac(ps, (uint8_t)(p - ps));
  if(p[1] == 'W') { this->_location.longitude = -this->_location.longitude; }

  // Ground speed
  /* Do not get it */
  p = strchr(p + 1, ',');

  // Track angle
  /* Do not get it */
  p = strchr(p + 1, ',');

  // Date
  p                 = strchr(p + 1, ',');
  float date        = atof(  p + 1);
  this->_time.day   = date / 10000;
  this->_time.month = date / 100 - this->_time.day * 100;
  this->_time.year  = date -       this->_time.day * 10000 - this->_time.month * 100 + 2000;

  this->_refreshFlag |= REFRESHED_RMC;
  this->_readingId++;

  // Save geographical position to RTC
  rtc_set_geoposition(this->_location.latitude, this->_location.longitude, 0.0);
}

float ClassGPS::sexagecimalToFrac(const char *buffer, uint8_t size)
{
  float res, f;
  UNUSED(size);

  f    = atof(buffer);
  res  = (float)(uint16_t)(f / 100.0);
  res += (f - res * 100.0)   /  60.0;

  return res;
}

