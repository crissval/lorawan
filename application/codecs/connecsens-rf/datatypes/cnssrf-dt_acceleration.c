/*
 * Data Type Acceleration3DG for ConnecSenS RF data frames
 *
 *  Created on: 17 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_acceleration.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_ACCELERATION_3DG_ID  0x0D

#define ACCEL_G_STEP  0.001f


  /**
   * Writes a Acceleration3DG Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     xg       the acceleration, in G, along the X axis.
   * @param[in]     yg       the acceleration, in G, along the Y axis.
   * @param[in]     zg       the acceleration, in G, along the Z axis.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_acceleration_write_3dg_to_frame(CNSSRFDataFrame *pv_frame, float xg, float yg, float zg)
  {
    CNSSRFValue values[3];

    values[0].type        = CNSSRF_VALUE_TYPE_INT16;
    values[0].value.int16 = (int16_t)(xg / ACCEL_G_STEP);
    values[1].type        = CNSSRF_VALUE_TYPE_INT16;
    values[1].value.int16 = (int16_t)(yg / ACCEL_G_STEP);
    values[2].type        = CNSSRF_VALUE_TYPE_INT16;
    values[2].value.int16 = (int16_t)(zg / ACCEL_G_STEP);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_ACCELERATION_3DG_ID,
						  values, 3,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif

