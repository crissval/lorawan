/*
 * Data Types in relation with radioactivity
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include "cnssrf-dt_radioactivity.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_BQM3_ID  0x1A


  /**
   * Write a RadioactivityBqM3 Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     bqm3     the radioactivity level, in Becquerels per cubic meter.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_radioactivity_write_bqm3_to_frame(CNSSRFDataFrame *pv_frame, uint32_t bqm3)
  {
    CNSSRFValue values[5];
    uint8_t     len = 0;

    do
    {
      values[len].type        = CNSSRF_VALUE_TYPE_UINT8;
      values[len].value.uint8 = (uint8_t)(bqm3 & 0x7F);
      bqm3 >>= 7;
      if(bqm3) { values[len].value.uint8 |= 1u << 7; }
      len++;
    }
    while(bqm3);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_BQM3_ID,
						  values, len,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif
