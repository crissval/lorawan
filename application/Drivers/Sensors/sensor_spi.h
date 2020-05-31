/**
 * Base class for external sensors that use the SPI interface.
 *
 * @file   sensor_spi.h
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef SENSORS_SENSOR_SPI_H_
#define SENSORS_SENSOR_SPI_H_

#include "sensor.hpp"
#include "spi.hpp"

class SensorSPI : public Sensor
{
public:
  SensorSPI(Power               power,
	    Power               powerSleep,
	    uint32_t            sckFreqHz,
	    SPI::Options        options,
	    const SPI::SSInfos *ptSSInfos = SPI::DEFAULT_SS_INFOS,
	    Features            features  = FEATURE_BASE);
  virtual ~SensorSPI();


protected:
  virtual bool jsonSpecificHandler(const JsonObject& json);
  virtual bool openSpecific();
  virtual void closeSpecific();


  virtual void spiSelectSlave(SPI::SSId id = SPI::SS_ID_DEFAULT);

  bool spiInitUsingOptions(uint32_t sckFreq = 0, SPI::Options options = SPI::OPTION_NONE)
  {
    return this->_spi.initUsingOptions(sckFreq, options);
  }

  bool spiWrite(const uint8_t *pu8Data, uint16_t size, uint32_t timeoutMs)
  {
    return this->_spi.write(pu8Data, size, timeoutMs);
  }

  bool spiRead(uint8_t *pu8Data, uint16_t nb, uint32_t timeoutMs)
  {
    return this->_spi.read(pu8Data, nb, timeoutMs);
  }

  bool spiWriteAndRead(const uint8_t *pu8TxData,
		       uint8_t *pu8RxData,
		       uint16_t size,
		       uint32_t timeoutMs)
  {
    return this->_spi.writeAndRead(pu8TxData, pu8RxData, size, timeoutMs);
  }

  bool switchMOSIToGPIOInput(bool asGPIO = true) { return this->_spi.switchMOSIToGPIOInput(asGPIO); }
  bool switchMISOToGPIOInput(bool asGPIO = true) { return this->_spi.switchMISOToGPIOInput(asGPIO); }

  bool waitForMOSItoGo(LogicLevel level, uint32_t timeoutMs) { return this->_spi.waitForMOSItoGo(level, timeoutMs); }
  bool waitForMISOtoGo(LogicLevel level, uint32_t timeoutMs) { return this->_spi.waitForMISOtoGo(level, timeoutMs); }


private:
  SPI _spi;
};


#endif /* SENSORS_SENSOR_SPI_H_ */
