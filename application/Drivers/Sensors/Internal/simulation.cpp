#include "simulation.hpp"


const char *Simulation::TYPE_NAME = "Simulation";

const char *Simulation::_CSV_HEADER_VALUES[] =
{
    "temperatureDegC", "airHumidityRelPercent", NULL
};


Simulation::Simulation() : Sensor(POWER_NONE, POWER_NONE)
{
  // Do nothing
}

Sensor *Simulation::getNewInstance()
{
  return new Simulation();
}


const char *Simulation::type()
{
  return TYPE_NAME;
}

bool Simulation::openSpecific()
{
  __HAL_RCC_RNG_CLK_ENABLE();
  this->_hrng.Instance = RNG;
  HAL_RNG_Init(&this->_hrng);

  return true;
}

void Simulation::closeSpecific()
{
  HAL_RNG_DeInit(&this->_hrng);
  __HAL_RCC_RNG_CLK_DISABLE();
}

bool Simulation::readSpecific()
{
  uint32_t test;

  HAL_RNG_GenerateRandomNumber(&this->_hrng, &test);

  this->_temperatureDegC = -30.0 + (test & 0x0FFF) / 10;
  this->_humidity        = (test & 0xFF) / 3;

  return true;
}

bool Simulation::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_temperature_write_degc_to_frame(         pvFrame, this->_temperatureDegC, CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE) &&
         cnssrf_dt_humidity_write_air_relpercents_to_frame( pvFrame, this->_humidity);
}


const char **Simulation::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t Simulation::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.1f%c%.1f",
		 this->_temperatureDegC, OUTPUT_DATA_CSV_SEP,
		 this->_humidity);

  return len >= size ? -1 : (int32_t)len;
}
