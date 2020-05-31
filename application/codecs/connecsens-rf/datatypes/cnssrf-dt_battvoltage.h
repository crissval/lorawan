/*
 * Data Type BattVoltageMV for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_BATTVOLTAGE_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_BATTVOLTAGE_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Define the battery voltage flags
   */
  typedef enum CNSSRFBattVoltageFlag
  {
    CNSSRF_BATTVOLTAGE_FLAG_NONE     = 0x00,
    CNSSRF_BATTVOLTAGE_FLAG_BATT_LOW = 0x01
  }
  CNSSRFBattVoltageFlag;
  typedef uint8_t CNSSRFBattVoltageFlags;


  bool cnssrf_dt_battvoltage_write_millivolts_to_frame(CNSSRFDataFrame *pv_frame, uint16_t mv);
  bool cnssrf_dt_battvoltage_write_volts_to_frame(     CNSSRFDataFrame *pv_frame, float    volts);

  bool cnssrf_dt_battvoltageflags_write_millivolts_to_frame(CNSSRFDataFrame       *pv_frame,
							    CNSSRFBattVoltageFlags flags,
							    uint16_t               mv);
  bool cnssrf_dt_battvoltageflags_write_volts_to_frame(     CNSSRFDataFrame       *pv_frame,
							    CNSSRFBattVoltageFlags flags,
							    float                  volts);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_BATTVOLTAGE_H_ */
