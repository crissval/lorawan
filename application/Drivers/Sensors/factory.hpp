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
/* Sensor Factory for Abstract Configuration **************/
/**********************************************************/
#pragma once
#include "sensor.hpp"


class SensorFactory
{
private:
  typedef struct SensorParams
  {
    const char *ps_typeName;  ///< The sensor's type name.

    /**
     * Return a new instance of the sensor.
     *
     * Delete the returned instance when you no longer need it.
     *
     * @return a new instance of the sensor.
     */
    Sensor *(*getNewInstance)(void);
  }
  SensorParams;


public:
  static Sensor *getNewSensorInstanceUsingType(const char *ps_type);


private:
  /**
   * The list of the all the sensors' parameters.
   * The end of the list is indicated using an element whose
   * ps_typeName parameter is set to NULL.
   */
  static const SensorParams SENSOR_PARAMS[];
};
