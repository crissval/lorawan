/*
 * Provide informations about the node
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */
#ifndef ENVIRONMENT_NODEINFO_H_
#define ENVIRONMENT_NODEINFO_H_


#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the possible RTC capacitor values.
   */
  typedef enum NodeInfoRTCCap
  {
    NODEINFO_RTC_CAP_UNKNOWN,  ///< Unknown RTC capacitor value.
    NODEINFO_RTC_CAP_12PF,     ///< RTC use 12 pF capacitors with its crystal.
    NODEINFO_RTC_CAP_20PF      ///< RTC use 20 pF capacitors with its crystal.
  }
  NodeInfoRTCCap;


  /**
   * The node's informations
   */
  typedef struct NodeInfo
  {
    struct {
      char           serial_number[12];    ///< The serial number
      NodeInfoRTCCap rtc_cap;              ///< The RTC capacitors' value.
      bool           has_batt_r26r27;      ///< Does the board has R26-R27 battery voltage divisor?
      float          batt_r26r27_divisor;  ///< The divisor value of the R26-R27 voltage divisor. 0 if no divisor value is set.
    }
    main_board;                ///< Informations about the main board

    struct {
      char serial_number[12];  ///< The serial number
    }
    sensors_board;             /// Informations about the board with the internal sensor's.
  }
  NodeInfo;


  extern void            nodeinfo_init();
  extern const NodeInfo *nodeinfo_infos();

  extern bool nodeinfo_main_write_batt_r26r27_divisor(float div);

#define nodeinfo_main_board_SN()          ((const char *)        nodeinfo_infos()->main_board.serial_number)
#define nodeinfo_main_board_rtc_cap()     ((const NodeInfoRTCCap)nodeinfo_infos()->main_board.rtc_cap)
#define nodeinfo_main_board_has_r26r27()  ((const bool)          nodeinfo_infos()->main_board.has_batt_r26r27)
#define nodeinfo_main_board_r26r27_div()  ((const float)         nodeinfo_infos()->main_board.batt_r26r27_divisor)
#define nodeinfo_sensors_board_SN()       ((const char *)        nodeinfo_infos()->sensors_board.serial_number)


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_NODEINFO_H_ */
