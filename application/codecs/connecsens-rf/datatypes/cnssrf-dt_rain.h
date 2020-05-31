/*
 * Data Type RainAmountMM
 *
 *  Created on: 30 mai 2018
 *      Author: FUCHET Jérôme
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_RAIN_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_RAIN_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  bool cnssrf_dt_rainamount_write_mm_to_frame(CNSSRFDataFrame *pv_frame,
					      uint32_t         timeSec,
					      float            amountMM,
					      bool             alarm);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_RAIN_H_ */
