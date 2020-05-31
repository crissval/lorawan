/*
 * Encode hash DataType to ConnecSenS RF data frames.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_HASH_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_HASH_H_


#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern bool cnssrf_dt_hash_write_sensortype_mm3hash32(CNSSRFDataFrame *pv_frame, uint32_t hash);
  extern bool cnssrf_dt_hash_write_config_mm3hash32(    CNSSRFDataFrame *pv_frame, uint32_t hash);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_HASH_H_ */
