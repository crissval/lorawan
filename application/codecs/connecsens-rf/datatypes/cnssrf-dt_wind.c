/*
 * Data Types for wind measurements for ConnecSenS RF data frames.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#include "cnssrf-dt_wind.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_TYPE_SPEED_DIR_ID  0x16
#define DATA_TYPE_SPEEDMS_ID    0x17
#define DATA_TYPE_DIRDEG_ID     0x18

#define SPEED_MAX      81.9  // m/s
#define DIRECTION_MAX  360   // °


  /**
   * Writes a WindSpeedDirection Data Type to a frame.
   *
   * @param[in,out] pv_frame           the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     speed_m_per_sec    the wind speed, in meters per seconds.
   * @param[in]     speed_is_corrected is the speed value a corrected one?
   * @param[in]     dir_deg            the wind direction, in degrees from North clockwise.
   * @param[in]     dir_is_corrected   is the direction value a corrected one?
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_wind_write_speed_direction(CNSSRFDataFrame *pv_frame,
					    float            speed_m_per_sec,
					    bool             speed_is_corrected,
					    uint16_t         dir_deg,
					    bool             dir_is_corrected)
  {
    uint32_t    u32;
    CNSSRFValue value;

    if(speed_m_per_sec < 0 || speed_m_per_sec > SPEED_MAX || dir_deg > DIRECTION_MAX) { return false; }

    u32                = (uint32_t)(speed_m_per_sec * 100.0);
    value.type         = CNSSRF_VALUE_TYPE_UINT24;
    value.value.uint24 = ((u32 & 0x1FFF) << 9) | (dir_deg & 0x1FF);
    if(speed_is_corrected) { value.value.uint24 |= 1u << 23; }
    if(dir_is_corrected)   { value.value.uint24 |= 1u << 22; }

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_SPEED_DIR_ID,
    						  &value, 1,
    						  CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a WindSpeedMS Data Type to a frame.
   *
   * @param[in,out] pv_frame           the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     speed_m_per_sec    the wind speed, in meters per seconds.
   * @param[in]     speed_is_corrected is the speed value a corrected one?
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_wind_write_speed_ms(CNSSRFDataFrame *pv_frame,
				     float            speed_m_per_sec,
				     bool             speed_is_corrected)
  {
    CNSSRFValue value;

    if(speed_m_per_sec < 0 || speed_m_per_sec > SPEED_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (uint16_t)(speed_m_per_sec * 100.0);
    if(speed_is_corrected) { value.value.uint16 |= 1u << 13; }

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_SPEEDMS_ID,
    						  &value, 1,
    						  CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a WindDirectionDegN Data Type to a frame.
   *
   * @param[in,out] pv_frame         the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     dir_deg          the wind direction, in degrees from North clockwise.
   * @param[in]     dir_is_corrected is the direction value a corrected one?
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_wind_write_direction_deg(CNSSRFDataFrame *pv_frame,
					  uint16_t         dir_deg,
					  bool             dir_is_corrected)
  {
    CNSSRFValue value;

    if(dir_deg > DIRECTION_MAX) { return false; }

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = dir_deg;
    if(dir_is_corrected) { value.value.uint16 |= 1u << 9; }

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_DIRDEG_ID,
    						  &value, 1,
    						  CNSSRF_META_DATA_FLAG_NONE);
  }



#ifdef __cplusplus
}
#endif



