/*
 * CNSSRF Data Type codecs in relation with permittivity.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_PERMITTIVITY_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_PERMITTIVITY_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_permittivity_write_relative_dielectric_to_frame(CNSSRFDataFrame *pv_frame, float e);

#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_PERMITTIVITY_H_ */
