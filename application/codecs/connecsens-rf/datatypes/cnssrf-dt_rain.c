/*
 * Data Type RainAmountMM
 *
 *  Created on: 30 mai 2018
 *      Author: FUCHET Jerome
 */
#include "cnssrf-dt_rain.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DATA_TYPE_ID  0x0E

#define AMOUNT_MASK   0x3FFF
#define ALARM_FLAG    0x4000
#define ALARM_NONE    0x0000


  /**
   * Writes a RainAmountMM Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to.
   *                         MUST be NOT NULL. MUST have been initialised.
   * @param[in]     timeSec  the time interval, in seconds, the rain amount corresponds to.
   * @param[in]     amountMM the amount of rain, in millimeters.
   * @param[in]     alarm    is the value in the alarm range?
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_rainamount_write_mm_to_frame(CNSSRFDataFrame *pv_frame,
  					      uint32_t         timeSec,
  					      float            amountMM,
  					      bool             alarm)
  {
    CNSSRFValue values[2];

    uint16_t v             = (uint16_t)(amountMM * 10);
    values[0].type         = CNSSRF_VALUE_TYPE_UINT24;
    values[0].value.uint24 = timeSec;
    values[1].type         = CNSSRF_VALUE_TYPE_UINT16;
    values[1].value.uint16 = (v & AMOUNT_MASK) | (uint16_t)(alarm ? ALARM_FLAG : ALARM_NONE);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_ID,
						  values, 2,
						  CNSSRF_META_DATA_FLAG_NONE);
  }

#ifdef __cplusplus
}
#endif
