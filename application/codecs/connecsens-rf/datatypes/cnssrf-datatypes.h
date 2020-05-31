/*
 * Header to the generic Data Type handler.
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DATATYPES_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DATATYPES_H_

#include "defs.h"
#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defines the possible types for the data values
   */
  typedef enum CNSSRFValueType
  {
    CNSSRF_VALUE_TYPE_UINT8,
    CNSSRF_VALUE_TYPE_INT8,
    CNSSRF_VALUE_TYPE_UINT16,
    CNSSRF_VALUE_TYPE_INT16,
    CNSSRF_VALUE_TYPE_UINT24,
    CNSSRF_VALUE_TYPE_INT24,
    CNSSRF_VALUE_TYPE_UINT32,
    CNSSRF_VALUE_TYPE_INT32,
    CNSSRF_VALUE_TYPE_FLOAT32,
    CNSSRF_VALUE_TYPE_COUNT     ///< Not an actual type; use to count the identifiers.
  }
  CNSSRFValueType;
#define CNSSRF_VALUE_TYPE_MAX  (CNSSRF_VALUE_TYPE_COUNT - 1)

  /**
   * Defines a data value
   */
  typedef struct CNSSRFValue
  {
    CNSSRFValueType type; ///< The value's type

    union
    {
      uint8_t  uint8;
      int8_t   int8;
      uint16_t uint16;
      int16_t  int16;
      uint32_t uint24;
      int32_t  int24;
      uint32_t uint32;
      int32_t  int32;
      float    float32;
      uint8_t  bytes[4];
    }
    value; ///< The actual value
  }
  CNSSRFValue;


  bool cnssrf_data_type_write_values_to_frame( CNSSRFDataFrame    *pv_frame,
					       CNSSRFDataType      type_id,
					       const CNSSRFValue  *pv_values,
					       uint8_t             nb_values,
					       CNSSRFMetaDataFlags flags);
  bool cnssrf_data_type_append_values_to_frame(CNSSRFDataFrame    *pv_frame,
					       const CNSSRFValue  *pv_values,
					       uint8_t             nb_values);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DATATYPES_H_ */
