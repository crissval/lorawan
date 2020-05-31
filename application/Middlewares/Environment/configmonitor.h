/*
 * Detects changes in the configuration file.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#ifndef ENVIRONMENT_CONFIGMONITOR_H_
#define ENVIRONMENT_CONFIGMONITOR_H_

#include "defs.h"
#include "datetime.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern void config_monitor_init(void);
  extern bool config_monitor_config_has_changed(const uint8_t *pu8_cfg, uint32_t size, bool save);
  extern bool config_monitor_save_config(       const uint8_t *pu8_cfg, uint32_t size);

  extern bool config_monitor_manual_time_has_changed(     const Datetime *pv_dt, bool save);
  extern bool config_monitor_time_sync_method_has_changed(const char     *ps_method_name,
							  bool            default_value,
							  bool            save);

  extern bool config_monitor_hash_mm3_32(uint32_t *pu32_hash);


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_CONFIGMONITOR_H_ */
