/*
 * Data Type AirHumidityRelPercent for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_humidity.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_AIR_RELPERCENT_ID  0x05

  /**
   * Writes a AirHumidityRelPercent Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     percents the humidity, in relative percentage.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_humidity_write_air_relpercents_to_frame(CNSSRFDataFrame *pv_frame, float percents)
  {
    CNSSRFValue value;

    value.type        = CNSSRF_VALUE_TYPE_UINT8;
    value.value.uint8 = (uint8_t)(percents * 2);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_AIR_RELPERCENT_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif
