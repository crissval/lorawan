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
/* GPS Class with A2135 Module ****************************/
/**********************************************************/
#pragma once

#include <string.h>
#include "board.h"
#include "datetime.h"
#include "buffer.hpp"
#include "periodic.hpp"
#include "json.hpp"


class ClassGPS : public ClassPeriodic
{
public:
  /**
   * The location data retrieved by the GPS.
   */
  typedef struct LocationData
  {
    float latitude;
    float longitude;
  }
  LocationData;

  /**
   * The flags used to indicated what has been retrieved by the GPS
   */
  typedef enum RefreshedFlag
  {
    REFRESHED_NONE = 0x00,
    REFRESHED_RMC  = (1u << 0)
  }
  RefreshedFlag;
  typedef uint32_t RefreshedFlags;


public:
  ClassGPS();

  bool setConfiguration(const JsonObject &json);

  void open();
  void close();
  bool refresh(bool blocking = false);
  void setRefreshTimeoutSec(        uint32_t timeoutSec);
  void setBlockingRefreshTimeoutSec(uint32_t timeoutSec);

  bool                hasLocation() { return (this->_refreshFlag & REFRESHED_RMC) != REFRESHED_NONE; }
  const LocationData &location()    { return  this->_location;  }
  const Datetime     &time()        { return  this->_time;      }
  uint32_t            readingId()   { return  this->_readingId; }


private:
  void initUART();
  void deinitUART();

  void NMEA_Handler(const char *buffer);
  void GGA_Handler( const char *buffer);
  void GLL_Handler( const char *buffer);
  void GSA_Handler( const char *buffer);
  void GSV_Handler( const char *buffer);
  void VTG_Handler( const char *buffer);
  void RMC_Handler( const char *buffer);

  float sexagecimalToFrac(const char *buffer, uint8_t size);


private:
  UART_HandleTypeDef _huart;
  RefreshedFlags     _refreshFlag;
  LocationData       _location;
  Datetime           _time;
  buffer<uint8_t>    _myBuffer;
  uint32_t           _readingId;
  uint32_t           _refreshTimeoutMs;
  uint32_t           _blockingRefreshTimeoutMs;
};
