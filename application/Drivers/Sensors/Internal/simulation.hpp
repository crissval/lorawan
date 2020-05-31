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
/* Sensor Simulation for ConnecSenS Device ****************/
/**********************************************************/
#pragma once

#include "stm32l4xx_hal.h"
#include "sensor.hpp"

class Simulation : public Sensor
{
public:
  static const char *TYPE_NAME;

public:
  Simulation();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool openSpecific();
  void closeSpecific();
  bool readSpecific();
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);


private:
  float _temperatureDegC;
  float _humidity;
  RNG_HandleTypeDef _hrng;

  static const char *_CSV_HEADER_VALUES[];
};

