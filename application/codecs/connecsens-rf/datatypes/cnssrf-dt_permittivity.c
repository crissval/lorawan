/*
 * CNSSRF Data Type codecs in relation with permittivity.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "cnssrf-dt_permittivity.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define RELATIVE_DIELECTRIC_DATA_TYPE_ID  0x1D


  /**
   * Writes a RelativeDielectricPermittivity Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     e        the relative dielectric permittivity.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_permittivity_write_relative_dielectric_to_frame(CNSSRFDataFrame *pv_frame, float e)
  {
    CNSSRFValue value;

    value.type          = CNSSRF_VALUE_TYPE_FLOAT32;
    value.value.float32 = e;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  RELATIVE_DIELECTRIC_DATA_TYPE_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif



