/*
 * Data Types for wind measurements for ConnecSenS RF data frames.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_WIND_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_WIND_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif



  bool cnssrf_dt_wind_write_speed_direction(CNSSRFDataFrame *pv_frame,
					    float            speed_m_per_sec,
					    bool             speed_is_corrected,
					    uint16_t         dir_deg,
					    bool             dir_is_corrected);
  bool cnssrf_dt_wind_write_speed_ms(       CNSSRFDataFrame *pv_frame,
					    float            speed_m_per_sec,
					    bool             speed_is_corrected);
  bool cnssrf_dt_wind_write_direction_deg(  CNSSRFDataFrame *pv_frame,
					    uint16_t         dir_deg,
					    bool             dir_is_corrected);

#ifdef __cplusplus
}
#endif

#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_WIND_H_ */
