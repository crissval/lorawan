/*
 * CNSSRF Data Type codecs in relation with level values.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "cnssrf-dt_level.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define LEVEL_M_DATA_TYPE_ID  0x20
#define LEVEL_M_VALUE_MAX     65535


  /**
   * Writes a Level Data Type to a frame using a level value in meters.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     m        the level, in meters.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_level_write_m_to_frame(CNSSRFDataFrame *pv_frame, float m)
  {
    CNSSRFValue value;

    if(m * 1000.0 > LEVEL_M_VALUE_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (uint16_t)(m * 1000.0 + 0.5);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  LEVEL_M_DATA_TYPE_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif




