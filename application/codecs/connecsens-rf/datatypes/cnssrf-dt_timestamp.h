/*
 * Data Type TimestampUTC for ConnecSenS RF data frames
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_TIMESTAMP_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_TIMESTAMP_H_

#include "cnssrf-dataframe.h"
#include "datetime.h"


#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_timestamp_utc_write_secs_to_frame(    CNSSRFDataFrame *pv_frame, ts2000_t        ts);
  bool cnssrf_dt_timestamp_utc_write_datetime_to_frame(CNSSRFDataFrame *pv_frame, const Datetime *pv_dt);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_TIMESTAMP_H_ */
