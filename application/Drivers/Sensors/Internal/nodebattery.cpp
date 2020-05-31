/**
 * This sensor is used to read the node's battery voltage using
 * the voltage divisor R26/R27 and an ADC input from the µC.
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2019
 */
#include <string.h>
#include "nodebattery.hpp"
#include "bq2589x.h"
#include "nodeinfo.h"
#include "connecsens.hpp"
#include "cnssrf.h"


#ifndef BATT_VOLTAGE_LOW_THRESHOLD_V
#define BATT_VOLTAGE_LOW_THRESHOLD_V  3.2  // Volts
#endif


#define ADC_LINE        ADC_LINE_BATV
#define R26R27_DIVISOR  2.0


  CREATE_LOGGER(nodebattery);
#undef  logger
#define logger  nodebattery


const char *NodeBattery::_CSV_HEADER_VALUES[] = { "nodeBattVoltageV", NULL };


NodeBattery NodeBattery::_instance;


/**
 * Constructor
 */
NodeBattery::NodeBattery() : SensorADC(POWER_NONE, POWER_NONE, ADC_LINE)
{
  this->_battVSource  = BATTV_SOURCE_UNKNOWN;
  this->_voltageLowMV = (uint16_t)(BATT_VOLTAGE_LOW_THRESHOLD_V * 1000);
  setWriteTypeHash(false);

  clearReadings();
  setName("NodeBattery");
  setCNSSRFDataChannel(CNSSRF_DATA_CHANNEL_NODE);
}

/**
 * Destructor
 */
NodeBattery::~NodeBattery()
{
  // Do nothing
}

/**
 * Return the unique instance of this class.
 *
 * @return the instance.
 */
NodeBattery *NodeBattery::instance() { return &_instance; }

/**
 * Return the sensor's type name.
 *
 * @return the type name.
 */
const char *NodeBattery::type() { return "NodeBattery"; }

/**
 * Clear current readings.
 */
void NodeBattery::clearReadings()
{
  this->_voltageMV  = 0;
  this->_isCharging = false;
}

/**
 * Open the sensor.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool NodeBattery::openSpecific()
{
  bool ok;

  // Initialise battery management IC.
  ok = bq2589x_init();

  // Get or set the source to use to get the battery voltage.
  if(this->_battVSource == BATTV_SOURCE_UNKNOWN)
  {
    this->_battVSource = nodeinfo_main_board_has_r26r27() ? BATTV_SOURCE_ADC : BATTV_SOURCE_BQ2589X;
  }

  // Open ADC if we use it
  if(this->_battVSource == BATTV_SOURCE_ADC) { ok &= SensorADC::openSpecific(); }

  return ok;
}

/**
 * Close the sensor
 */
void  NodeBattery::closeSpecific()
{
  if(this->_battVSource == BATTV_SOURCE_ADC) { SensorADC::closeSpecific(); }
  bq2589x_deinit();
}

/**
 * Read battery values.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool NodeBattery::readSpecific()
{
  float div = R26R27_DIVISOR;
  bool  ok  = false;

  // Read battery voltage
  switch(this->_battVSource)
  {
    case BATTV_SOURCE_BQ2589X:
      if((ok = bq2589x_read(BQ2589X_VALUE_VBATT))) { this->_voltageMV = bq2589x_vbat_mv(); }
      break;

    case BATTV_SOURCE_ADC:
      if((ok = SensorADC::readSpecific()))
      {
	this->_voltageMV = adcValueMV(ADC_LINE);

	if(!nodeinfo_main_board_r26r27_div())
	{
	  if(bq2589x_read(BQ2589X_VALUE_VBATT) && bq2589x_vbat_v() > BATT_VOLTAGE_EMPTY_V)
	  {
	    // Compute actual divisor value
	    div = ((float)bq2589x_vbat_mv()) / ((float)this->_voltageMV);

	    // Save divisor
	    nodeinfo_main_write_batt_r26r27_divisor(div);
	  }
	}
	else { div = nodeinfo_main_board_r26r27_div(); }

	this->_voltageMV = (uint16_t)(this->_voltageMV * div);
	bq2589x_set_vbat_mv(this->_voltageMV);  // This is done so that the voltage can be used from C code.
      }
      break;

    default:
      log_error(logger, "The source to use to read the battery voltage is not set.");
      break;
  }

  // See if the battery is charging
  this->_isCharging = bq2589x_is_charging();

  return ok;
}

/**
 * Write battery values to a RF data frame.
 *
 * @param[in,out] pvFrame the frame to write to. MUST be NOT NULL.
 *
 * @return true  on success.
 * @return false otherwise.
 */
bool NodeBattery::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  CNSSRFBattVoltageFlags battFlags = CNSSRF_BATTVOLTAGE_FLAG_NONE;

  if(this->_voltageMV <= this->_voltageLowMV) { battFlags |= CNSSRF_BATTVOLTAGE_FLAG_BATT_LOW; }

  return cnssrf_dt_battvoltageflags_write_millivolts_to_frame(pvFrame, battFlags, this->_voltageMV);
}


const char **NodeBattery::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t NodeBattery::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.3f", this->_voltageMV / 1000.0);

  return len >= size ? -1 : (int32_t)len;
}

