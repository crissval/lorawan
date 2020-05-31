/*
 * Data Type TempDegC and TempDegCLowRes for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_TEMPERATURE_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_TEMPERATURE_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CNSSRF_DT_TEMPERATURE_DEGC_VALUE_ERROR (-273.2)
  /**
   * Defines the flags for the TempDegC Data Type.
   */
  typedef enum CNSSRFDTTemperatureDegCFlag
  {
    CNSSRF_DT_TEMPERATURE_DEGC_FLAG_NONE       = 0x00,  ///< No flag
    CNSSRF_DT_TEMPERATURE_DEGC_FLAG_ALARM_LOW  = 0x01,  ///< the low alarm is active
    CNSSRF_DT_TEMPERATURE_DEGC_FLAG_ALARM_HIGH = 0x02   ///< The high alarm is active
  }
  CNSSRFDTTemperatureDegCFlag;
  typedef uint8_t CNSSRFDTTemperatureDegCFlags;

  //**AJOUT*********************************************************setp3
  #define CNSSRF_DT_DELTA_TEMP_DEGC_VALUE_ERROR  (-3276.8)
  //****************************************************************

  bool cnssrf_dt_temperature_write_degc_to_frame(       CNSSRFDataFrame            *pv_frame,
							float                       degc,
							CNSSRFDTTemperatureDegCFlag flags);
  bool cnssrf_dt_temperature_write_degc_lowres_to_frame(CNSSRFDataFrame            *pv_frame,
							float                       degc);
  //**AJOUT****************************************************************************************step3
    bool cnssrf_dt_delta_temp_write_degc_to_frame(        CNSSRFDataFrame            *pv_frame,
  							float                       degc);
  //************************************************************************************************


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_TEMPERATURE_H_ */
