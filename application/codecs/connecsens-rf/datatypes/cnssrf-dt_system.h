/**
 * Defines functions for CNSSRF Data Types that are related to the system life.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_SYSTEM_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_SYSTEM_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  extern bool cnssrf_dt_system_write_app_version(CNSSRFDataFrame *pv_frame,
						 uint8_t          major,
						 uint8_t          minor,
						 uint8_t          patch,
						 const char      *ps_suffix,
						 uint32_t         hash32);

  /**
   * Defines the reset source identifiers.
   */
  typedef enum CNSSRFDTSystemResetSourceId
  {
    CNSSRF_DT_SYS_RESET_SOURCE_BOR                  = 0x1,
    CNSSRF_DT_SYS_RESET_SOURCE_PIN                  = 0x2,
    CNSSRF_DT_SYS_RESET_SOURCE_SOFTWARE             = 0x3,
    CNSSRF_DT_SYS_RESET_SOURCE_SOFTWARE_ERROR       = 0x4,
    CNSSRF_DT_SYS_RESET_SOURCE_WATCHOG              = 0x5,
    CNSSRF_DT_SYS_RESET_SOURCE_INDEPENDENT_WATCHDOG = 0x6,
    CNSSRF_DT_SYS_RESET_SOURCE_LOW_POWER_ERROR      = 0x7,
    CNSSRF_DT_SYS_RESET_SOURCE_OPTION_BYTES_LOADING = 0x8,
    CNSSRF_DT_SYS_RESET_SOURCE_FIREWALL             = 0x9
  }
  CNSSRFDTSystemResetSourceId;

  extern bool cnssrf_dt_system_write_reset_source(CNSSRFDataFrame            *pv_frame,
						  CNSSRFDTSystemResetSourceId source_id);

#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_SYSTEM_H_ */
