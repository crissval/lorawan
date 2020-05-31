/*
 * rtd_pt.hpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#ifndef SENSORS_RTD_RTD_PT_HPP_
#define SENSORS_RTD_RTD_PT_HPP_

#include "rtd.hpp"

class RTDPT : public RTD {
public:
  RTDPT(float ohmsAt0DegC = 0.0);

  void setOhmsAt0DegC(         float          ohmsAt0DegC);
  bool setOhmsAt0DegCUsingName(const char    *psName);
  bool updateTemperatureUsingResistance(float ohms);

private:
  float _ohmsAt0DegC;  // Sensor's resistance at 0°C.
};



#endif /* SENSORS_RTD_RTD_PT_HPP_ */
