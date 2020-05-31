/*
 * Data Types in relation with radioactivity
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_RADIOACTIVITY_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_RADIOACTIVITY_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern bool cnssrf_dt_radioactivity_write_bqm3_to_frame(CNSSRFDataFrame *pv_frame, uint32_t bqm3);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_RADIOACTIVITY_H_ */
