/*
 * Encode hash DataType to ConnecSenS RF data frames.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include "cnssrf-dt_hash.h"
#include "cnssrf-datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_TYPE_MM3HASH32_DATA_TYPE_ID  0x23
#define CONFIG_MM3HASH32_DATA_TYPE_ID       0x24


  /**
   * Write a SensorTypeMM3Hash32 DataType to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL.
   *                         MUST have been initialised.
   * @param[in]     hash     the hash value.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_hash_write_sensortype_mm3hash32(CNSSRFDataFrame *pv_frame, uint32_t hash)
  {
    CNSSRFValue value;

    value.type         = CNSSRF_VALUE_TYPE_UINT32;
    value.value.uint32 = hash;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  SENSOR_TYPE_MM3HASH32_DATA_TYPE_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_GLOBAL);
  }

  /**
   * Write a ConfigMM3Hash32 DataType to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL.
   *                         MUST have been initialised.
   * @param[in]     hash     the hash value.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_hash_write_config_mm3hash32(CNSSRFDataFrame *pv_frame, uint32_t hash)
  {
    CNSSRFValue value;

    value.type         = CNSSRF_VALUE_TYPE_UINT32;
    value.value.uint32 = hash;

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  CONFIG_MM3HASH32_DATA_TYPE_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_GLOBAL);
  }

#ifdef __cplusplus
}
#endif
