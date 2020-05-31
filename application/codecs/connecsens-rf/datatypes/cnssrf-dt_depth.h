/*
 * CNSSRF Data Type codecs in relation with depth values.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_DEPTH_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_DEPTH_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_depth_write_cm_to_frame(CNSSRFDataFrame *pv_frame, uint16_t cm);

#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_DEPTH_H_ */
