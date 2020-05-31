/*
 * Data Type BattVoltageMV for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_battvoltage.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BATTVOLTAGE_DATA_TYPE_ID       0x01
#define BATTVOLTAGEFALGS_DATA_TYPE_ID  0x0F


  /**
   * Writes a BattVoltageMV Data Type to a frame using a battery voltage expressed in millivolts.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     mv       the battery voltage, in millivolts.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_battvoltage_write_millivolts_to_frame(CNSSRFDataFrame *pv_frame, uint16_t mv)
  {
    CNSSRFValue value;

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = mv;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  BATTVOLTAGE_DATA_TYPE_ID,
						  &value, 1,
						  pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
						      CNSSRF_META_DATA_FLAG_GLOBAL :
						      CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a BattVoltageMV Data Type to a frame using a battery voltage expressed in Volts.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     volts    the battery voltage, in Volts. MUST be > 0 V and < 65.536 V.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_battvoltage_write_volts_to_frame(CNSSRFDataFrame *pv_frame, float volts)
  {
    return cnssrf_dt_battvoltage_write_millivolts_to_frame(pv_frame, (uint16_t)(volts * 1000));
  }


  /**
   * Writes a BattVoltageFlagsMV Data Type to a frame using a battery voltage expressed in millivolts.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     flags    the battery flags to write.
   * @param[in]     mv       the battery voltage, in millivolts.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_battvoltageflags_write_millivolts_to_frame(CNSSRFDataFrame       *pv_frame,
							    CNSSRFBattVoltageFlags flags,
							    uint16_t               mv)
  {
    uint16_t    v;
    CNSSRFValue value;

    // Set up the battery voltage
    if((v = mv / 10) > 4096) { return false; }

    // Set up the flags
    v |= ((uint16_t)flags) << 12;

    // Prepare the value to write
    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = v;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  BATTVOLTAGEFALGS_DATA_TYPE_ID,
						  &value, 1,
						  pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
						      CNSSRF_META_DATA_FLAG_GLOBAL :
						      CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a BattVoltageFlagsMV Data Type to a frame using a battery voltage expressed in Volts.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     flags    the battery flags to write.
   * @param[in]     volts    the battery voltage, Volts.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_battvoltageflags_write_volts_to_frame(CNSSRFDataFrame       *pv_frame,
						       CNSSRFBattVoltageFlags flags,
						       float                  volts)
  {
    return cnssrf_dt_battvoltageflags_write_millivolts_to_frame(pv_frame, flags, (uint16_t)(volts * 1000));
  }

#ifdef __cplusplus
}
#endif


