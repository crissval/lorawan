/*
 * Provide Data Type codecs related to GPIOs.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include "cnssrf-dt_gpio.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define DIGITAL_INPUT_DATA_TYPE_ID  0x0B


  /**
   * Writes a DigitalInput Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     id       the digital input identifier.
   * @param[in]     state    the digital input state: false for a low level, true for a high level.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_gpio_write_state_single_with_id(CNSSRFDataFrame *pv_frame,
						 uint8_t          id,
						 bool             state)
  {
    CNSSRFValue value;

    value.type        = CNSSRF_VALUE_TYPE_UINT8;
    value.value.uint8 = id << 1;
    if(state) { value.value.uint8 |= 0x01; }

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DIGITAL_INPUT_DATA_TYPE_ID,
    						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }


#ifdef __cplusplus
}
#endif



