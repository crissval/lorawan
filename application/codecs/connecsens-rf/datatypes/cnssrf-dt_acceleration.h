/*
 * Data Type Acceleration3DG for ConnecSenS RF data frames
 *
 *  Created on: 17 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_ACCELERATION_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_ACCELERATION_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_acceleration_write_3dg_to_frame(CNSSRFDataFrame *pv_frame, float xg, float yg, float zg);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_ACCELERATION_H_ */
