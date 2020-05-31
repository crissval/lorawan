/*
 * Provide Data Type codecs in relation with heaters.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "cnssrf-dt_heater.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define HEATER_STATUS_DATA_TYPE_ID  0x1B


  /**
   * Writes a HeaterStatus Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     is_on    indicate if the heater is turned on (true), or not (false).
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_heater_write_status_to_frame(CNSSRFDataFrame *pv_frame, bool is_on)
  {
    CNSSRFValue value;

    value.type        = CNSSRF_VALUE_TYPE_UINT8;
    value.value.uint8 = is_on ? 0x01 : 0;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  HEATER_STATUS_DATA_TYPE_ID,
    						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif
