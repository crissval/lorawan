/*
 * Data Types SoilMoistureCb and SoilMoistureCbDegCHz for ConnecSenS RF data frames.
 *
 *  Created on: 10 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "cnssrf-dt_soilmoisture.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_SOIL_MOISTURE_CB_ID           0x10
#define DATA_TYPE_SOIL_MOISTURE_CB_DEPTH_MAX    511
#define DATA_TYPE_SOIL_MOISTURE_CB_DEPTH_ERROR  0

#define DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_ID          0x11
#define DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_MIN    (-50.0)
#define DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_MAX      77.0

#define DATA_TYPE_SOIL_MOISTURE_VOLUMETRIC_CONTENT_PERCENT_ID   0x1E
#define DATA_TYPE_SOIL_MOISTURE_VOLUMETRIC_CONTENT_PERCENT_MAX  100


  /**
   * Write soil moisture base reading values to a table of values to send.
   *
   * Writes the depth, the humidity in centibars end the alarm flags.
   *
   * @param[out] pv_values    where to write the data. MUST be NOT NULL and MUST point to a table
   *                          with at least 3 elements.
   * @param[in]  pv_readings  the object with the reading values to write. MUST be NOT NULL.
   * @param[in]  last_reading indicate if the current reading is the last one to write (true) or not (false).
   *
   * @return the number of values table elements written to.
   */
  static uint8_t cnssrf_dt_soilmoisture_cb_write_base_values(CNSSRFValue                   *pv_values,
							     CNSSRFDTSoilMoistureCbReading *pv_reading,
							     bool                           last_reading)
  {
    uint16_t depth_cm;
    uint8_t  v;
    uint8_t  i = 0;

    depth_cm  = pv_reading->depth_cm;
    if(!depth_cm || depth_cm > DATA_TYPE_SOIL_MOISTURE_CB_DEPTH_MAX) { depth_cm = DATA_TYPE_SOIL_MOISTURE_CB_DEPTH_ERROR; }

    pv_values[i].type = CNSSRF_VALUE_TYPE_UINT8;
    v              = (uint8_t)(depth_cm & 0x3F);
    if(!last_reading) { v |= 1u << 7; }
    if(depth_cm > 63 || pv_reading->flags)
    {
	// Use long format
	pv_values[i].value.uint8   = v | (1u << 6);
	v                          = (uint8_t)((depth_cm >> 6) & 0x07);
	if(pv_reading->flags & CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_LOW)  { v |= 1u << 6; }
	if(pv_reading->flags & CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_HIGH) { v |= 1u << 7; }
	pv_values[++i].type        = CNSSRF_VALUE_TYPE_UINT8;
	pv_values[  i].value.uint8 = v;
    }
    else
    {
	// use short format
	pv_values[i].value.uint8 = v;
    }
    pv_values[++i].type        = CNSSRF_VALUE_TYPE_UINT8;
    pv_values[  i].value.uint8 = pv_reading->centibars;

    return i + 1;
  }


  /**
   * Writes a SoilMoistureCb Data Type to a frame.
   *
   * @param[in,out] pv_frame    the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     pv_readings a table with the readings. MUST be NOT NULL.
   * @param[in]     nb_readings the number of readings in the table.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_soilmoisture_write_moisture_cb_to_frame(CNSSRFDataFrame               *pv_frame,
							 CNSSRFDTSoilMoistureCbReading *pv_readings,
							 uint8_t                        nb_readings)
  {
    CNSSRFValue values[3];
    uint8_t     i, len;
    bool        res;

    for(i = 0, res = true; i < nb_readings; i++, pv_readings++)
    {
      len = cnssrf_dt_soilmoisture_cb_write_base_values(values, pv_readings, i == nb_readings - 1);
      if(!len) { res = false; continue; }

      res = res && cnssrf_data_type_write_values_to_frame(pv_frame,
							  DATA_TYPE_SOIL_MOISTURE_CB_ID,
							  values, len,
							  CNSSRF_META_DATA_FLAG_NONE);
    }

    return res;
  }


  /**
   * Writes a SoilMoistureCbDegCHz Data Type to a frame.
   *
   * @param[in,out] pv_frame    the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     pv_readings a table with the readings. MUST be NOT NULL.
   * @param[in]     nb_readings the number of readings in the table.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_soilmoisture_write_moisture_cbdegchz_to_frame(CNSSRFDataFrame                     *pv_frame,
  							       CNSSRFDTSoilMoistureCbDegCHzReading *pv_readings,
  							       uint8_t                              nb_readings)
  {
    CNSSRFValue values[9];
    uint8_t     i, len;
    float       degc;
    uint32_t    freq;

    if(!cnssrf_data_frame_set_current_data_type(pv_frame,
						  DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_ID,
						  CNSSRF_META_DATA_FLAG_NONE)) { goto error_exit; }

    for(i = 0; i < nb_readings; i++, pv_readings++)
    {
      // Write base values
      len = cnssrf_dt_soilmoisture_cb_write_base_values(values, &pv_readings->base, (i == nb_readings - 1));

      // Add specific values
      // Write temperature
      degc = pv_readings->tempDegC;
      if(degc < DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_MIN || degc > DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_MAX)
      {
	degc = DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_ERROR;
      }
      values[len]  .type        = CNSSRF_VALUE_TYPE_UINT8;
      values[len++].value.uint8 = (uint8_t)((degc - DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_MIN) * 2);

      // Write frequency
      freq = pv_readings->freqHz;
      do
      {
	values[len].type        = CNSSRF_VALUE_TYPE_UINT8;
	values[len].value.uint8 = (uint8_t)(freq & 0x7F);
	freq >>= 7;
	if(freq) { values[len].value.uint8 |= 1u << 7; }
	len++;
      }
      while(freq);

      if(!cnssrf_data_type_append_values_to_frame(pv_frame, values, len)) { goto error_exit; }
    }

    return true;

    error_exit:
    return false;
  }


  /**
   * Write a SoilVolumetricWaterContentPercent Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     percent  the volumetric water content, in percents.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_soilmoisture_write_volumetric_content_percent_to_frame(CNSSRFDataFrame *pv_frame,
  									float            percent)
  {
    CNSSRFValue value;

    if(percent > DATA_TYPE_SOIL_MOISTURE_VOLUMETRIC_CONTENT_PERCENT_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (uint16_t)(percent * 100.0 + 0.005);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_SOIL_MOISTURE_VOLUMETRIC_CONTENT_PERCENT_ID,
    						  &value, 1,
    						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif



