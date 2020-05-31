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
/**********************************************************/
#pragma once

#include <string.h>
#include "network.hpp"
#include "buffer.hpp"

class ClassNwkSimul : public ClassNetwork
{
public:
  static const char *TYPE_NAME;  ///< The  network type's name.


public :
  ClassNwkSimul();

  const char *typeName() const;

  uint32_t maxPayloadSize();


protected:
  bool openSpecific();
  void closeSpecific();
  void sleepSpecific();
  void wakeupSpecific();
  bool sendSpecific(const uint8_t *pu8_data,
		    uint16_t       size,
		    SendOptions    options);
};
