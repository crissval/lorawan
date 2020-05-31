/*
 * Data Types SolutionConductivityUSCm and SolutionConductivityMSCm for ConnecSenS RF data frames.
 *
 *  Created on: Jul 20, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "cnssrf-dt_solutionconductivity.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_USCM_ID           0x12
#define DATA_TYPE_MSCM_ID           0x13
#define DATA_TYPE_SPECIFIC_USCM_ID  0x21
#define DATA_TYPE_SPECIFIC_MSCM_ID  0x22
#define DATA_TYPE_VALUE_MIN         0.0
#define DATA_TYPE_VALUE_MAX         419430.3


  /**
   * Stores Data Type id by CNSSRFDTSolutionConductivityType.
   */
  static const uint8_t _cnssrf_dt_solcdt_type_to_id[] =
  {
      DATA_TYPE_USCM_ID,
      DATA_TYPE_MSCM_ID,
      DATA_TYPE_SPECIFIC_USCM_ID,
      DATA_TYPE_SPECIFIC_MSCM_ID
  };


  /**
   * Writes a SolutionConductivityUSCm or SolutionConductivityMSCm Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     value    the conductivity value.
   * @param[in]     type     the conductivity type, and units.
   * @param[in]     flags    flags associated with the value.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_solutionconductivity_write_to_frame(CNSSRFDataFrame                  *pv_frame,
  						     float                             value,
						     CNSSRFDTSolutionConductivityType  type,
  						     CNSSRFDTSolutionConductivityFlags flags)
  {
    CNSSRFValue v;

    if(value < DATA_TYPE_VALUE_MIN || value > DATA_TYPE_VALUE_MAX ||
	type > CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_MSCM) { return false; }

    v.type         = CNSSRF_VALUE_TYPE_UINT24;
    v.value.uint24 = (((uint32_t)(value * 10.0)) & 0x3FFFFF) | (((uint32_t)flags) << 22);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  _cnssrf_dt_solcdt_type_to_id[type],
						  &v, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }



#ifdef __cplusplus
}
#endif
