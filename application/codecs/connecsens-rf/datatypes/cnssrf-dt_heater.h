/*
 * Provide Data Type codecs in relation with heaters.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_HEATER_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_HEATER_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern bool cnssrf_dt_heater_write_status_to_frame(CNSSRFDataFrame *pv_frame, bool is_on);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_HEATER_H_ */
