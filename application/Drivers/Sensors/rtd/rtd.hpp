/*
 * rtd.hpp
 *
 *  Created on: 29 mai 2020
 *      Author: OPGC
 */

#ifndef SENSORS_RTD_RTD_HPP_
#define SENSORS_RTD_RTD_HPP_

#include "defs.h"

class RTD {
public:
    static const float DEGC_ERROR_VALUE;
    virtual ~RTD();

    float        temperatureDegC() const { return _temperatureDegC; }
    virtual bool updateTemperatureUsingResistance(float ohms) = 0;

protected:
    RTD();
    void setTemperatureDegC(float degC) {
        _temperatureDegC = degC;
    }

private:
    float _temperatureDegC; // Current temperature, in °C.
};



#endif /* SENSORS_RTD_RTD_HPP_ */
