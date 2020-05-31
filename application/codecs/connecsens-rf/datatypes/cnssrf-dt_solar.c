/*
 * Data Types for solar measurements for ConnecSenS RF data frames.
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_solar.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_TYPE_ILLUMINANCE_LUX_ID  0x08
#define DATA_TYPE_IRRADIANCE_WM2_ID   0x19


#define IRRADIANCE_WM2_MAX  1638.3

  /**
   * Writes a IlluminnaceLux Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     lux      the illuminance, in Lux.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_solar_write_lux_to_frame(CNSSRFDataFrame *pv_frame, uint16_t lux)
  {
    CNSSRFValue value;

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = lux;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_ILLUMINANCE_LUX_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a SolarIrradianceWM2 Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     wm2      the irradiance, in Watts per square meters.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_solar_write_irradiance_wm2_to_frame(CNSSRFDataFrame *pv_frame, float wm2)
  {
    CNSSRFValue value;

    if(wm2 < 0 || wm2 > IRRADIANCE_WM2_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (uint16_t)(wm2 * 10);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_IRRADIANCE_WM2_ID,
    						  &value, 1,
    						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif
