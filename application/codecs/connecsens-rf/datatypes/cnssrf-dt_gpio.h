/*
 * Provide Data Type codecs related to GPIOs.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_GPIO_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_GPIO_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern bool cnssrf_dt_gpio_write_state_single_with_id(CNSSRFDataFrame *pv_frame,
							uint8_t          id,
							bool             state);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_GPIO_H_ */
