/*
 * factory.cpp
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include <string.h>
#include "factory.hpp"
#include "simulation.hpp"
#include "sht35.hpp"
#include "opt3001.hpp"
#include "lps25.hpp"
#include "lis3dh.hpp"
#include "raingaugecontact.hpp"
#include "soilmoisturewatermarki2c.hpp"
#include "insituaquatroll200sdi12.hpp"
#include "batteryadc.hpp"
#include "gillmaximetsdi12.h"
#include "algadeaerttserial.hpp"
#include "truebnersmt100sdi12.hpp"
#include "digitalinput.hpp"
#include "max31865.hpp"

const SensorFactory::SensorParams SensorFactory::SENSOR_PARAMS[] =
{
    { Simulation::TYPE_NAME,        &Simulation::getNewInstance        },
    { SHT35::TYPE_NAME,             &SHT35::getNewInstance             },
    { OPT3001::TYPE_NAME,           &OPT3001::getNewInstance           },
    { LPS25::TYPE_NAME,             &LPS25::getNewInstance             },
    { LIS3DH::TYPE_NAME,            &LIS3DH::getNewInstance            },
#ifdef USE_SENSOR_DIGITAL_INPUT
    { DigitalInput::TYPE_NAME,      &DigitalInput::getNewInstance      },
#endif
#ifdef USE_SENSOR_RAIN_GAUGE_CONTACT
    { RainGaugeContact::TYPE_NAME,        &RainGaugeContact::getNewInstance        },
#endif
#ifdef USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C
    { SoilMoistureWatermakI2C::TYPE_NAME, &SoilMoistureWatermakI2C::getNewInstance },
#endif
#ifdef USE_SENSOR_INSITU_AQUATROLL_200_SDI12
    { InSituAquaTROLL200SDI12::TYPE_NAME, &InSituAquaTROLL200SDI12::getNewInstance },
#endif
#ifdef USE_SENSOR_BATTERY_ADC
    { BatteryADC::TYPE_NAME,              &BatteryADC::getNewInstance              },
#endif
#ifdef USE_SENSOR_GILL_MAXIMET_SDI12
    { GillMaximMetSDI12::TYPE_NAME,       &GillMaximMetSDI12::getNewInstance       },
#endif
#ifdef USE_SENSOR_ALGADE_AERTT_SERIAL
    { AlgadeAERTTSerial::TYPE_NAME,       &AlgadeAERTTSerial::getNewInstance       },
#endif
#ifdef USE_SENSOR_TRUEBNER_SMT100_SDI12
    { TruebnerSMT100SDI12::TYPE_NAME,     &TruebnerSMT100SDI12::getNewInstance     },
#endif
#ifdef USE_SENSOR_MAX31865
    { MAX31865::TYPE_NAME,     &MAX31865::getNewInstance     },
#endif
    { NULL, NULL }
};


/**
 * Return a new instance of a sensor using it's type.
 *
 * @param[in] ps_type the sensor's type. MUST be NOT NULL.
 *                    The type name comparison is not case sensitive.
 *
 * @return the new instance. Delete it when you no longer need it.
 * @return NULL if the type is unknown.
 */
Sensor *SensorFactory::getNewSensorInstanceUsingType(const char *ps_type)
{
  for(const SensorParams *pv_params = SENSOR_PARAMS;
      pv_params->ps_typeName;
      pv_params++)
  {
    if(strcasecmp(ps_type, pv_params->ps_typeName) == 0)
    {
      return pv_params->getNewInstance();
    }
  }

  return NULL;
}

