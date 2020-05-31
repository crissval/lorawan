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
/* Driver LPS25 for ConnecSenS Device *********************/
/**********************************************************/
#pragma once

#include "sensor_internal.hpp"


class LPS25 : public SensorInternal
{
public:
  static const char *TYPE_NAME;


public :
  LPS25();
  static Sensor *getNewInstance();
  const char    *type();


private:
  bool WhoAmI();

  bool readSpecific();
  bool configAlertSpecific();
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);



private:
  float pressure;
  float LowThreshold;
  float HighThreshold;

  static const char *_CSV_HEADER_VALUES[];
};

