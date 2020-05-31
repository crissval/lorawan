/*
 * Data Type GeographicalPosition3D and GeographicalPosition2D
 * for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_position.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_GEO_3D_ID  0x06
#define DATA_TYPE_GEO_2D_ID  0x07

#define GEO_DEG_STEP    0.000025
#define GEO_DEG_OFFSET  180.0
#define GEO_ALT_STEP    0.01
#define GEO_ALT_OFFSET  7000.00



  /**
   * Converts a position value from a decimal fraction of a degree to the DataType coding.
   *
   * @param[in] deg_decimal the value to convert.
   *
   * @return the position value coded for the position DataType.
   */
  static uint32_t cnssrf_dt_position_deg_decimal_to_int24(float deg_decimal)
  {
    return (uint32_t)((deg_decimal + GEO_DEG_OFFSET) / GEO_DEG_STEP);
  }


  /**
   * Writes a GeopgraphicalPosition3D Data Type to a frame.
   *
   * @param[in,out] pv_frame              the data frame to write to.
   *                                      MUST be NOT NULL. MUST have been initialised.
   * @param[in]     latitude_deg_decimal  the latitude, in decimal degrees.
   * @param[in]     longitude_deg_decimal the longitude, in decimal degrees.
   * @param[in]     altitude_meter        the altitude, in meters.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_position_write_geo3d_to_frame(CNSSRFDataFrame *pv_frame,
					       float            latitude_deg_decimal,
					       float            longitude_deg_decimal,
					       float            altitude_meter)
  {
    CNSSRFValue values[3];

    values[0].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[0].value.uint24 = cnssrf_dt_position_deg_decimal_to_int24(latitude_deg_decimal);
    values[1].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[1].value.uint24 = cnssrf_dt_position_deg_decimal_to_int24(longitude_deg_decimal);
    values[2].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[2].value.uint24 = (uint32_t)((altitude_meter + GEO_ALT_OFFSET) / GEO_ALT_STEP);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_GEO_3D_ID,
						  values, 3,
						  pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
						      CNSSRF_META_DATA_FLAG_GLOBAL :
						      CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a GeopgraphicalPosition2D Data Type to a frame.
   *
   * @param[in,out] pv_frame              the data frame to write to.
   *                                      MUST be NOT NULL. MUST have been initialised.
   * @param[in]     latitude_deg_decimal  the latitude, in decimal degrees.
   * @param[in]     longitude_deg_decimal the longitude, in decimal degrees.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_position_write_geo2d_to_frame(CNSSRFDataFrame *pv_frame,
  					       float            latitude_deg_decimal,
  					       float            longitude_deg_decimal)
  {
    CNSSRFValue values[2];

    values[0].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[0].value.uint24 = cnssrf_dt_position_deg_decimal_to_int24(latitude_deg_decimal);
    values[1].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[1].value.uint24 = cnssrf_dt_position_deg_decimal_to_int24(longitude_deg_decimal);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_GEO_2D_ID,
						  values, 2,
						  pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
						      CNSSRF_META_DATA_FLAG_GLOBAL :
						      CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif

