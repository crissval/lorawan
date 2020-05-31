/**
 * Defines functions and data related to batteries
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#ifndef BATTERY_BATTERY_H_
#define BATTERY_BATTERY_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defined the battery types identifiers.
   */
  typedef enum BatteryTypeId
  {
    BATT_TYPE_PB,       ///< Lead-acid batteries,
    BATT_TYPE_LIPO,     ///< Generic Lithium polymer battery
    BATT_TYPE_LIFEPO4,  ///< Lithium iron phosphate battery
    BATT_TYPE_NIMH,     ///< Nickel metal hydride battery
    BATT_TYPE_NICD,     ///< Nickel Cadmium battery
    BATT_TYPES_COUNT,   ///< Not an actual type; use to count them
    BATT_TYPE_UNKNOWN = BATT_TYPES_COUNT
  }
  BatteryTypeId;

  /**
   * Description of a battery cell.
   */
  typedef struct BatteryCellDesc
  {
    BatteryTypeId type_id;             ///< It's type identifier.
    const char   *ps_type_name;        ///< The type's name.

    uint16_t      voltage_nominal_mv;  ///< The cell's nominal voltage, in millivolts.
    uint16_t      voltage_abs_max_mv;  ///< The cell's absolute maximal voltage, in millivolts.
    uint16_t      voltage_abs_min_mv;  ///< The cell's absolute minimal voltage, in millivolts.
    uint16_t      voltage_low_mv;      ///< The voltage under which the cell's voltage is considered low, in millivolts.
  }
  BatteryCellDesc;

  /**
   * Defines a battery
   */
  typedef struct Battery
  {
    const BatteryCellDesc *pv_cell_desc;  ///< Description of the cells the battery is made of.
    uint8_t                nb_cells;      ///< The number of cells in the battery.

    uint32_t voltage_low_mv;  ///< The battery's low voltage threshold, in millivolts.
    uint32_t voltage_mv;      ///< The battery's voltage, in millivolts.
  }
  Battery;


  extern bool battery_init_with_nb_cells(             Battery      *pv_batt,
						      BatteryTypeId type_id,
						      uint8_t       nb_cells);
  extern bool battery_init_with_voltage(              Battery      *pv_batt,
						      BatteryTypeId type_id,
						      uint32_t      voltage_mv);
  extern bool battery_init_with_voltage_and_type_name(Battery    *pv_batt,
						      const char *ps_type_name,
						      uint32_t    voltage_mv);

  extern const BatteryCellDesc *battery_get_type_description_using_id(       BatteryTypeId type_id);
  extern const BatteryCellDesc *battery_get_type_description_using_type_name(const char    *ps_type_name);

#define battery_voltage_mv(        pv_batt)        ((pv_batt)->voltage_mv)
#define battery_set_voltage_mv(    pv_batt, v_mv)  ((pv_batt)->voltage_mv     = (v_mv))
#define battery_voltage_low_mv(    pv_batt)        ((pv_batt)->voltage_low_mv)
#define battery_set_voltage_low_mv(pv_batt, v_mv)  ((pv_batt)->voltage_low_mv = (v_mv))
#define battery_voltage_is_low(    pv_batt)        ((pv_batt)->voltage_mv <= (pv_batt)->voltage_low_mv)



#ifdef __cplusplus
}
#endif
#endif /* BATTERY_BATTERY_H_ */
