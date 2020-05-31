/*
 * Data Type TimestampUTC for ConnecSenS RF data frames
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#include <cnssrf-dt_timestamp.h>
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_ID  0x00


  /**
   * Write a TimestampUTC Data Type to a frame using a timestamp described as the number of seconds
   * elapsed since 2000/01/01 00:00:00.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     ts       the timestamp.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_timestamp_utc_write_secs_to_frame(CNSSRFDataFrame *pv_frame, ts2000_t ts)
  {
    CNSSRFValue value;

    value.type         = CNSSRF_VALUE_TYPE_UINT32;
    value.value.uint32 = ts;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_ID,
						  &value, 1,
						  pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
						      CNSSRF_META_DATA_FLAG_GLOBAL :
						      CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Write a TimestampUTC Data Type to a frame using a timestamp represented using a Datetime object.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     pv_dt    the Datetime object. MUST be NOT NULL. MUST have been initialised.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_timestamp_utc_write_datetime_to_frame(CNSSRFDataFrame *pv_frame, const Datetime *pv_dt)
  {
    return cnssrf_dt_timestamp_utc_write_secs_to_frame(pv_frame, datetime_to_timestamp_sec_2000(pv_dt));
  }


#ifdef __cplusplus
}
#endif
