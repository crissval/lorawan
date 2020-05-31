/*
 * Data Types PressAtmoHPa and PressurePa for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_pressure.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DATA_TYPE_ATMO_HPA_ID         0x04
#define DATA_TYPE_ATMO_HPA_VALUE_MIN  700.0
#define DATA_TYPE_ATMO_HPA_VALUE_MAX  1355.36

#define DATA_TYPE_PRESSURE_PA_ID         0x14
#define DATA_TYPE_PRESSURE_PA_VALUE_MIN -26843545.5
#define DATA_TYPE_PRESSURE_PA_VALUE_MAX  26843545.5


  /**
   * Writes a PressAtmoHPa Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     hpa      the pressure in hectopascals.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_pressure_write_atmo_hpa_to_frame(CNSSRFDataFrame *pv_frame, float hpa)
  {
    CNSSRFValue value;

    if(hpa < DATA_TYPE_ATMO_HPA_VALUE_MIN || hpa > DATA_TYPE_ATMO_HPA_VALUE_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (uint16_t)((hpa - DATA_TYPE_ATMO_HPA_VALUE_MIN) * 100);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_ATMO_HPA_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


  /**
   * Writes a PressurePa Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     pa       the pressure, in Pascals.
   * @param[in]     flags    the flags associated with the value.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_pressure_write_pa_to_frame(CNSSRFDataFrame        *pv_frame,
					    double                  pa,
					    CNSSRFDTPressurePaFlags flags)
  {
    CNSSRFValue value;

    if(pa < DATA_TYPE_PRESSURE_PA_VALUE_MIN || pa > DATA_TYPE_PRESSURE_PA_VALUE_MAX) { return false; }
    if(pa < 0) { pa = -pa; flags |= 0x01; }  // Set the sign bit.

    value.type         = CNSSRF_VALUE_TYPE_UINT32;
    value.value.uint32 = (((uint32_t)(pa * 10.0 + 0.5)) & 0x0FFFFFFF) | (((uint32_t)flags) << 28);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_PRESSURE_PA_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif

