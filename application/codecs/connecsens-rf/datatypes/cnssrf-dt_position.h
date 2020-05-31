/*
 * Data Type GeographicalPosition3D and GeographicalPosition2D
 * for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_POSITION_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_POSITION_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_position_write_geo3d_to_frame(CNSSRFDataFrame *pv_frame,
					       float            latitude_deg_decimal,
					       float            longitude_deg_decimal,
					       float            altitude_meter);
  bool cnssrf_dt_position_write_geo2d_to_frame(CNSSRFDataFrame *pv_frame,
  					       float            latitude_deg_decimal,
  					       float            longitude_deg_decimal);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_POSITION_H_ */
