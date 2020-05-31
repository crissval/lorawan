/*
 * Data Type Config for ConnecSenS RF data frames
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#include <string.h>
#include "cnssrf-dt_config.h"



#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_ID  0x15


  /**
   * Writes a Config Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     param_id the configuration parameter's identifier.
   * @param[in]     ps_value the configuration parameter's value. Can be NULL or empty. MUST be a string.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_config_write_parameter_value(CNSSRFDataFrame    *pv_frame,
					      CNSSRFConfigParamId param_id,
					      const char         *ps_value)
  {
    uint8_t header[2];
    uint8_t len;

    // Check the value
    if(!ps_value || !*ps_value) { goto exit; }

    // Check parameters
    len = strlen(ps_value);
    if(param_id > CNSSRF_DT_CONFIG_PARAM_ID_MAX || len > CNSSRF_DT_CONFIG_VALUE_LEN_MAX) { goto error_exit; }

    // Write to the frame
    header[0] = param_id;
    header[1] = len;
    if( !cnssrf_data_frame_set_current_data_type(pv_frame, DATA_TYPE_ID, CNSSRF_META_DATA_FLAG_NONE) ||
	!cnssrf_data_frame_add_bytes(            pv_frame, header, sizeof(header))                   ||
	!cnssrf_data_frame_add_bytes(            pv_frame, (const uint8_t *)ps_value, len))
    { goto error_exit; }

    exit:
    return true;

    error_exit:
    return false;
  }


#ifdef __cplusplus
}
#endif
