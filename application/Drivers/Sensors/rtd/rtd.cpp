/*
 * rtd.cpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#include "rtd.hpp"

const float RTD::DEGC_ERROR_VALUE = -300.0;

RTD::RTD() {
  this->_temperatureDegC = DEGC_ERROR_VALUE;
}

RTD::~RTD() {
  // Do nothing

}


