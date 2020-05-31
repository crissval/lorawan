/*
 * Data Type Config for ConnecSenS RF data frames
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_CONFIG_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_CONFIG_H_


#include "cnssrf-dataframe.h"


#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defines the parameter identifiers
   */
  typedef enum CNSSRFConfigParamId
  {
    CNSSRF_CONFIG_PARAM_NAME                   = 0x01,
    CNSSRF_CONFIG_PARAM_UNIQUE_ID              = 0x02,
    CNSSRF_CONFIG_PARAM_TYPE                   = 0x03,
    CNSSRF_CONFIG_PARAM_FIRMWARE_VERSION       = 0x04,
    CNSSRF_CONFIG_PARAM_EXPERIMENT_NAME        = 0x05,
    CNSSRF_CONFIG_PARAM_SENSOR_READ_PERIOD_SEC = 0x06,
    CNSSRF_CONFIG_PARAM_SEND_CONFIG_PERIOD_SEC = 0x07,
    CNSSRF_CONFIG_PARAM_GPS_READ_PERIOD_SEC    = 0x08
  }
  CNSSRFConfigParamId;
#define CNSSRF_DT_CONFIG_PARAM_ID_MAX  0x3F

#define CNSSRF_DT_CONFIG_VALUE_LEN_MAX 0x1F


  extern bool cnssrf_dt_config_write_parameter_value(CNSSRFDataFrame    *pv_frame,
						     CNSSRFConfigParamId param_id,
						     const char         *ps_value);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_CONFIG_H_ */
