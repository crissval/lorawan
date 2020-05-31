/**
 * Base class for external sensors that use the SPI interface.
 *
 * @file   sensor_spi.cpp
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include <string.h>
#include "sensor_spi.h"
#include "connecsens.hpp"
#include "gpio.h"


/**
 * Initialise a sensor that uses SPI communication.
 *
 * @param[in] power      The power configuration to use when reading the sensor.
 * @param[in] powerSleep The power configuration required by the sensor when the node is asleep.
 * @param[in] sckFreqHz  The frequency of the SPI SCK signal, in Hz. Only used in master mode.
 * @param[in] options    The SPI options to use.
 *                       If your sensor uses several SPI devices then declare the most
 *                       complete signal usage (#OPTION_USE_MOSI and #OPTION_USE_MISO).
 *                       Because the GPIO configuration will be done only once when the
 *                       sensor is opened, so you must declare here all the SPI GPIOs
 *                       than can be used by your sensor.
 * @param[in] ptSSInfos  The list of the slave select signals informations.
 *                       The end of the list is indicated using a SS infos whose
 *                       @p psConfigKeyBaseName is NULL.
 *                       If the sensor does not use SS signal then set this parameter
 *                       to NULL.
 *                       To use a single SS signal active low then use
 *                       SSInfos::_DEFAULT_SS_INFOS.
 * @param[in] features   The features provided by the sensor.
 */
SensorSPI::SensorSPI(Power               power,
		     Power               powerSleep,
		     uint32_t            sckFreqHz,
		     SPI::Options        options,
		     const SPI::SSInfos *ptSSInfos,
		     Features            features)
: Sensor(power,     powerSleep, features),
  _spi(  sckFreqHz, options,    ptSSInfos)
{
  // Do nothing
}

/**
 * Destructor.
 */
SensorSPI::~SensorSPI()
{
  // Do nothing
}


bool SensorSPI::jsonSpecificHandler(const JsonObject& json)
{
  return this->_spi.initUsingJSON(json);
}


bool SensorSPI::openSpecific()
{
  return this->_spi.open();
}

void SensorSPI::closeSpecific()
{
  this->_spi.close();
}


/**
 * Select a slave SPI device, or no slave.
 *
 * @note This function does nothing if the SPI interface is not configured in master mode.
 *
 * @param[in] id The identifier of the slave to select.
 */
void SensorSPI::spiSelectSlave(SPI::SSId id)
{
  this->_spi.selectSlave(id);
}
