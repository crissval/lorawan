/*
 * Data Type AirHumidityRelPercent for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_HUMIDITY_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_HUMIDITY_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_humidity_write_air_relpercents_to_frame(CNSSRFDataFrame *pv_frame, float percents);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_HUMIDITY_H_ */
