/*
 * CNSSRF Data Type codecs in relation with level values.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_LEVEL_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_LEVEL_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  bool cnssrf_dt_level_write_m_to_frame(CNSSRFDataFrame *pv_frame, float m);

#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_LEVEL_H_ */
