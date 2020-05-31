/*
 * Driver for the BQ2589X family battery chargers?
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef MODULES_PSU_BQ2589X_H_
#define MODULES_PSU_BQ2589X_H_

#include "defs.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the battery charge statuses.
   */
  typedef enum BQ2589XChargeStatus
  {
    BQ2589X_NOT_CHARGING  = 0x00,  ///< Not charging
    BQ2589X_CHARGING_PRE  = 0x01,  ///< Charging, and in pre-charge state
    BQ2589X_CHARGING_FAST = 0x02,  ///< Charging, and in fast charge state
    BQ2589X_CHARGING_POST = 0x03   ///< Charging, and in post-charge state
  }
  BQ2589XChargeStatus;

  /**
   * Defines the identifiers of the values that can be read from the charger.
   */
  typedef enum BQ2589XValueId
  {
    BQ2589X_VALUE_NONE       = 0,        ///< No value
    BQ2589X_VALUE_VBATT      = 1u << 1,  ///< Battery voltage
#ifdef BQ2589X_USE_VSYS
    BQ2589X_VALUE_VSYS       = 1u << 2,  ///< Vsys
#endif
#ifdef BQ2589X_USE_VBUS
    BQ2589X_VALUE_VBUS       = 1u << 3,  ///< Vbus
#endif
#ifdef BQ2589X_USE_ICHG
    BQ2589X_VALUE_ICHG       = 1u << 4,  ///< Charge current
#endif
#ifdef BQ2589X_USE_IBUS_LIMIT
    BQ2589X_VALUE_IBUS_LIMIT = 1u << 5   ///< The input current limit
#endif
  }
  BQ2589XValueId;
  typedef uint8_t BQ2589XValues;    ///< Stored a ORed combination of BQ2589XValueId values.


  extern bool  bq2589x_init(  void);
  extern void  bq2589x_deinit(void);
  extern bool  bq2589x_read(  BQ2589XValues values);

  extern float    bq2589x_vbat_v(     void);
  extern uint16_t bq2589x_vbat_mv(    void);
  extern void     bq2589x_set_vbat_mv(uint16_t mv);
  extern uint32_t bq2589x_vbat_level( uint32_t min_value, uint32_t max_value, uint32_t error_value);

#ifdef BQ2589X_USE_VSYS
  extern float    bq2589x_vsys_v( void);
  extern uint16_t bq2589x_vsys_mv(void);
#endif

#ifdef BQ2589X_USE_VBUS
  extern float    bq2589x_vbus_v( void);
  extern uint16_t bq2589x_vbus_mv(void);
#endif

#ifdef BQ2589X_USE_ICHG
  extern float    bq2589x_ichg_a( void);
  extern uint16_t bq2589x_ichg_ma(void);
#endif

#ifdef BQ2589X_USE_IBUS_LIMIT
  extern float    bq2589x_ibus_limit_a( void);
  extern uint16_t bq2589x_ibus_limit_ma(void);
#endif

  extern BQ2589XChargeStatus bq2589x_charge_status(void);
  extern bool                bq2589x_is_charging(  void);


#ifdef __cplusplus
}
#endif
#endif /* MODULES_PSU_BQ2589X_H_ */
