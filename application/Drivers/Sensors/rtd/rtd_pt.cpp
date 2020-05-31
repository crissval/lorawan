/*
 * rtd_pt.cpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */



//d_pt.cpp
#include <string.h>  // To use string manipulation functions.
#include "rtd_pt.hpp"
#include "utils.h"   // Some string functions that we wrote.

/*#include "logger.h"
#include "ext_gpio.h"

CREATE_LOGGER(rtd_pt);  // Create logger
#undef  _loggers
#define _loggers  rtd_pt
*/

RTDPT::RTDPT(float ohmsAt0DegC) : RTD() {
  this->_ohmsAt0DegC = ohmsAt0DegC;
}

void RTDPT::setOhmsAt0DegC(float ohmsAt0DegC) {
  if(ohmsAt0DegC > 0) { this->_ohmsAt0DegC = ohmsAt0DegC; }
}

bool RTDPT::setOhmsAt0DegCUsingName(const char *psName) {
  int32_t len;
  float   f;

  // Perform some basic checks on the name.
  if(!psName || (len = strlen(psName)) < 4  ||
     (psName[0] != 'P' && psName[0] != 'p') ||
     (psName[1] != 'T' && psName[1] != 't')) { return false; }

  // Get integer value from name
  f = strn_string_to_float_trim_with_default(&psName[2], len - 2,0.0);
  if(f <= 0) { return false; }

  this->_ohmsAt0DegC = f;
  return true;
}

bool RTDPT::updateTemperatureUsingResistance(float ohms) {
  float tempDegC, num, denom;

  if(ohms <= 0) { return false; }
  if(this->_ohmsAt0DegC != 100.0) {  // Normalise to PT100.
    ohms  *= this->_ohmsAt0DegC / 100.0;
  }
  //******************test gpio externe**************

/*
  log_info(_loggers, "PASSAGE GPIO DEBUT.");

           gpio_turn_on( GPIO_1);


  log_info(_loggers, "PASSAGE GPIO FIN.");
*/

    //**************************************************

  // Compute temperature using normalised resistance.
  num      = ohms * (2.5293 + ohms   * (-0.066046 + ohms *
             (4.0422e-3 - 2.0697e-6  * ohms)));
  denom    = 1.0 + ohms * (-0.025422 + ohms *
             (1.6883e-3 - 1.3601e-6  * ohms));
  tempDegC = -245.19 + num / denom;
  if(tempDegC < -200.0) { tempDegC = RTD::DEGC_ERROR_VALUE; }

  // Set temperature (using parent's function).
  setTemperatureDegC(tempDegC);
  return tempDegC != RTD::DEGC_ERROR_VALUE;
}

