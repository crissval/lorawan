/**
 * @file  sdi12gen_manager.h
 * @brief Header file for the SDI-12 interfaces generic manager.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __SDI_12_SDI12GEN_MANAGER_H__
#define __SDI_12_SDI12GEN_MANAGER_H__

#include "sdi12.h"
#include "sdi12gen.h"

#ifdef __cplusplus
extern "C" {
#endif


  extern void sdi12_gen_mgr_init(   void);
  extern bool sdi12_gen_mgr_process(void);

  extern bool sdi12_gen_mgr_register_sensor_command_descriptions(const SDI12GenSensorCommands *p_sensor_cmds);
  extern const SDI12GenCommandDescription *sdi12_gen_mgr_get_description_for_command(
      const char *ps_cmd_name,
      const char *ps_sensor_type_name);

  extern bool sdi12_gen_mgr_register_interface(const char     *ps_name,
					       SDI12Interface *p_interface,
					       SDI12PHY       *p_phy);
  extern bool sdi12_gen_mgr_create_and_register_interface(const char *ps_name, SDI12PHY *p_phy);

  extern SDI12Interface *sdi12_gen_mgr_get_interface_by_name(const char *ps_name);

  extern bool sdi12_gen_mgr_send_command(const char               *ps_interface_name,
					 const char               *ps_cmd_name,
					 const char               *ps_sensor_type_name,
					 char                      addr,
					 const char               *ps_cmd_var_values,
					 sdi12_command_result_cb_t success_cb,
					 void                     *pv_success_cbargs,
					 sdi12_command_result_cb_t failed_cb,
					 void                     *pv_failed_cbargs);


#ifdef __cplusplus
}
#endif
#endif /* __SDI_12_SDI12GEN_MANAGER_H__ */
