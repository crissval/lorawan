/*
 * Data Type PressAtmoHPa for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_PRESSURE_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_PRESSURE_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defines the flags for the PressurePa Data Type
   */
  typedef enum CNSSRFDTPressurePaFlag
  {
    CNSSRF_DT_PRESSURE_PA_FLAG_NONE       = 0x00,  ///< No flag.
    CNSSRF_DT_PRESSURE_PA_FLAG_ABS        = 0x02,  ///< The pressure is absolute.
    CNSSRF_DT_PRESSURE_PA_FLAG_ALARM_LOW  = 0x04,  ///< The low alarm is active.
    CNSSRF_DT_PRESSURE_PA_FLAG_ALARM_HIGH = 0x08   ///< The high alarm is active.
  }
  CNSSRFDTPressurePaFlag;
  typedef uint8_t CNSSRFDTPressurePaFlags;


  bool cnssrf_dt_pressure_write_atmo_hpa_to_frame(CNSSRFDataFrame        *pv_frame,
						  float                   hpa);
  bool cnssrf_dt_pressure_write_pa_to_frame(      CNSSRFDataFrame        *pv_frame,
						  double                  pa,
						  CNSSRFDTPressurePaFlags flags);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_PRESSURE_H_ */
