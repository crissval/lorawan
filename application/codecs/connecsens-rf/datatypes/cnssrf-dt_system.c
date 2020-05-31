/**
 * Defines functions for CNSSRF Data Types that are related to the system life.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include <string.h>
#include "cnssrf-dt_system.h"
#include "cnssrf-datatypes.h"


#define APP_VERSION_DATA_TYPE_ID   0x2B
#define APP_VERSION_MAJOR_MAX      31
#define APP_VERSION_MINOR_MAX      63
#define APP_VERSION_PATCH_MAX      15

#define RESET_SOURCE_DATA_TYPE_ID  0x2C
#define RESET_SOURCE_ID_MIN        0x1
#define RESET_SOURCE_ID_MAX        0x9



#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Writes a AppSoftwareVersionMMPHash32 Data Type to a frame.
   *
   * @param[in,out] pv_frame  The data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     major     The version's major number.
   * @param[in]     minor     The version's minor number.
   * @param[in]     patch     The version's patch number.
   * @param[in]     ps_suffix The version's suffix. Can be NULL or empty.
   * @param[in]     hash32    The 32 bits version's hash.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_system_write_app_version(CNSSRFDataFrame *pv_frame,
					  uint8_t          major,
					  uint8_t          minor,
					  uint8_t          patch,
					  const char      *ps_suffix,
					  uint32_t         hash32)
  {
    CNSSRFValue values[2];

    if( major > APP_VERSION_MAJOR_MAX ||
	minor > APP_VERSION_MINOR_MAX ||
	patch > APP_VERSION_PATCH_MAX) { return false; }

    values[0].type         = CNSSRF_VALUE_TYPE_UINT16;
    values[0].value.uint16 =
	(((uint16_t)major) << 10) | (((uint16_t)minor) << 4) | patch;
    if(ps_suffix && strcasecmp(ps_suffix, "-dev") == 0)
    {
      values[0].value.uint16 |= 1 << 15;
    }
    values[1].type         = CNSSRF_VALUE_TYPE_UINT32;
    values[1].value.uint32 = hash32;

    return cnssrf_data_type_write_values_to_frame(
	pv_frame,
	APP_VERSION_DATA_TYPE_ID,
	values, 2,
	pv_frame->current_data_channel == CNSSRF_DATA_CHANNEL_0 ?
	    CNSSRF_META_DATA_FLAG_GLOBAL :
	    CNSSRF_META_DATA_FLAG_NONE);
  }


  /**
   * Write a ResetSource Data Type to a frame.
   *
   * @param[in,out] pv_frame  The data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     source_id The reset source identifier.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_system_write_reset_source(CNSSRFDataFrame            *pv_frame,
  					   CNSSRFDTSystemResetSourceId source_id)
  {
    CNSSRFValue value;

    if(source_id < RESET_SOURCE_ID_MIN || source_id > RESET_SOURCE_ID_MAX)
    { return false; }

    value.type        = CNSSRF_VALUE_TYPE_UINT8;
    value.value.uint8 = source_id & 0x0F;

    return cnssrf_data_type_write_values_to_frame(
	pv_frame,
	RESET_SOURCE_DATA_TYPE_ID,
	&value, 1,
	CNSSRF_META_DATA_FLAG_NONE);
  }

#ifdef __cplusplus
}
#endif
