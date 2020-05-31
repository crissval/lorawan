/*
 * Implementation of the generic Data Type handler.
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-datatypes.h"
#include "cnssrf-dataframe.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Store the size of the data for each data type value identifiers.
   * This table is indexes by CNSSRFValueType values.
   */
  static const uint8_t _cnssrf_data_type_value_sizes[CNSSRF_VALUE_TYPE_COUNT] =
  {
      1, 1,  // CNSSRF_VALUE_TYPE_UINT8 and CNSSRF_VALUE_TYPE_INT8
      2, 2,  // CNSSRF_VALUE_TYPE_UINT16 and CNSSRF_VALUE_TYPE_INT16
      3, 3,  // CNSSRF_VALUE_TYPE_UINT24 and CNSSRF_VALUE_TYPE_INT24
      4, 4,  // CNSSRF_VALUE_TYPE_UINT32 and CNSSRF_VALUE_TYPE_INT32
      4      // CNSSRF_VALUE_TYPE_FLOAT32
  };



  /**
   * Write a Data Type's values to a frame.
   *
   * @param[in,out] pv_frame  the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     type_id   the Data Type identifier.
   * @param[in]     pv_values a table, a list, of the values to write. MUST be NOT NULL.
   * @param[in]     nb_values the number of values in the table. MUST be > 0.
   * @param[in]     flags     the flags associated with the Data Type and it's data.
   *
   * @return true on success.
   * @return false otherwise.
   */
  bool cnssrf_data_type_write_values_to_frame(CNSSRFDataFrame    *pv_frame,
  					      CNSSRFDataType      type_id,
  					      const CNSSRFValue  *pv_values,
  					      uint8_t             nb_values,
					      CNSSRFMetaDataFlags flags)
  {
    return cnssrf_data_frame_set_current_data_type(pv_frame, type_id, flags) &&
	cnssrf_data_type_append_values_to_frame(   pv_frame, pv_values, nb_values);
  }

  /**
   * Append data to the current frame's Data Type.
   *
   * @param[in,out] pv_frame  the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     pv_values a table, a list, of the values to append. MUST be NOT NULL.
   * @param[in]     nb_values the number of values in the table. MUST be > 0.
   *
   * @return true on success.
   * @return false if there is no current Data Type.
   * @return false otherwise.
   */
  bool cnssrf_data_type_append_values_to_frame(CNSSRFDataFrame    *pv_frame,
					       const CNSSRFValue  *pv_values,
					       uint8_t             nb_values)
  {
    if(pv_frame->current_data_type == CNSSRF_DATA_TYPE_UNDEFINED) { goto error_exit; }

    // Write the values to the data frame
    for( ; nb_values; nb_values--, pv_values++)
    {
      if(pv_values->type <= CNSSRF_VALUE_TYPE_MAX)
      {
	if(!cnssrf_data_frame_add_bytes(pv_frame,
					pv_values->value.bytes,
					_cnssrf_data_type_value_sizes[pv_values->type])) { goto error_exit; }
      }
      else { goto error_exit; }
    }

    return true;

    error_exit:
    return false;
  }


#ifdef __cplusplus
}
#endif
