/*
 * CNSSRF Data Type codecs in relation with depth values.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "cnssrf-dt_depth.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define DEPTH_CM_DATA_TYPE_ID  0x1C
#define DEPTH_CM_VALUE_MAX     32767


  /**
   * Writes a DepthCm Data Type to a frame using a depth value in centimeters.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     cm       the depth, in centimeters.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_depth_write_cm_to_frame(CNSSRFDataFrame *pv_frame, uint16_t cm)
  {
    CNSSRFValue values[2];
    uint8_t     nb_values;

    if(cm > DEPTH_CM_VALUE_MAX) { return false; }

    values[0].type        = CNSSRF_VALUE_TYPE_UINT8;
    values[0].value.uint8 = cm & 0x7F;
    nb_values             = 1;

    if(cm >= 128)
    {
      values[0].value.uint8 |= 0x80;
      values[1].type         = CNSSRF_VALUE_TYPE_UINT8;
      values[1].value.uint8  = (cm >> 7) & 0xFF;
      nb_values = 2;
    }

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DEPTH_CM_DATA_TYPE_ID,
						  values, nb_values,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif
