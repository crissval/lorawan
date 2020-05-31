/*
 * Data Types for solar measurements for ConnecSenS RF data frames.
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLAR_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLAR_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_solar_write_lux_to_frame(           CNSSRFDataFrame *pv_frame, uint16_t lux);
  bool cnssrf_dt_solar_write_irradiance_wm2_to_frame(CNSSRFDataFrame *pv_frame, float    wm2);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLAR_H_ */
