/**
 * Defines functions and data related to batteries
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#include <string.h>
#include "battery.h"


#ifdef __cplusplus
extern "C" {
#endif



  /**
   * List of battery descriptions.
   */
  static BatteryCellDesc _battery_descs[BATT_TYPES_COUNT] =
  {
      { BATT_TYPE_PB,      "Pb",      2100, 2270, 1830, 2000 },
      { BATT_TYPE_LIPO,    "LiPo",    3600, 4200, 2700, 3200 },
      { BATT_TYPE_LIFEPO4, "LiFePO4", 3300, 3600, 2500, 2900 },
      { BATT_TYPE_NIMH,    "NiMH",    1250, 1400, 1000, 1180 },
      { BATT_TYPE_NICD,    "NiCd",    1250, 1400, 1000, 1180 }
  };



  /**
   * Initialise a battery using it's type, the number of cells it is made of and
   * the cell type's description.
   *
   * @param[out] pv_batt  the battery object to initialise. MUST be NOT NULL.
   * @param[in]  pv_desc  the cell's type's description. MUST be NOT NULL.
   * @param[in]  nb_cells the number of cells used to make the battery. MUST be > 0.
   *
   * @return true  on success.
   * @return false if the description is not valid.
   */
  static bool battery_init_with_nb_cells_and_desc(Battery               *pv_batt,
						  const BatteryCellDesc *pv_desc,
						  uint8_t                nb_cells)
  {
    if(!pv_desc || pv_desc->type_id == BATT_TYPE_UNKNOWN) { return false; }

    pv_batt->pv_cell_desc   = pv_desc;
    pv_batt->nb_cells       = nb_cells;

    // Compute default values
    pv_batt->voltage_low_mv = pv_batt->pv_cell_desc->voltage_low_mv     * nb_cells;
    pv_batt->voltage_mv     = pv_batt->pv_cell_desc->voltage_nominal_mv * nb_cells;

    return true;
  }


  /**
   * Initialise a battery using it's type and the number of cells it is made of.
   *
   * @param[out] pv_batt  the battery object to initialise. MUST be NOT NULL.
   * @param[in]  type_id  the identifier of the cells the battery is built with.
   * @param[in]  nb_cells the number of cells used to make the battery. MUST be > 0.
   *
   * @return true  on success.
   * @return false if the type is not valid.
   */
  bool battery_init_with_nb_cells(Battery *pv_batt, BatteryTypeId type_id, uint8_t  nb_cells)
  {
    return battery_init_with_nb_cells_and_desc(pv_batt,
					       battery_get_type_description_using_id(type_id),
					       nb_cells);
  }

  /**
   * Initialise a battery using it's type and the battery's nominal voltage.
   *
   * @param[out] pv_batt  the battery object to initialise. MUST be NOT NULL.
   * @param[in]  type_id  the identifier of the cells the battery is built with.
   * @param[in]  nb_cells the number of cells used to make the battery. MUST be > 0.
   */
  bool battery_init_with_voltage(Battery *pv_batt, BatteryTypeId type_id, uint32_t voltage_mv)
  {
    uint8_t                nb_cells;
    const BatteryCellDesc *pv_desc = battery_get_type_description_using_id(type_id);

    if(!pv_desc) { return false; }
    nb_cells = (voltage_mv + pv_desc->voltage_nominal_mv / 2) / pv_desc->voltage_nominal_mv;

    return battery_init_with_nb_cells_and_desc(pv_batt, pv_desc, nb_cells);
  }

  /**
   * Initialise a battery using it's type name and the battery's nominal voltage.
   *
   * @param[out] pv_batt  the battery object to initialise. MUST be NOT NULL.
   * @param[in]  type_id  the identifier of the cells the battery is built with.
   * @param[in]  nb_cells the number of cells used to make the battery. MUST be > 0.
   */
  bool battery_init_with_voltage_and_type_name(Battery    *pv_batt,
					       const char *ps_type_name,
					       uint32_t    voltage_mv)
  {
    uint8_t                nb_cells;
    const BatteryCellDesc *pv_desc = battery_get_type_description_using_type_name(ps_type_name);

    if(!pv_desc) { return false; }
    nb_cells = (voltage_mv + pv_desc->voltage_nominal_mv / 2) / pv_desc->voltage_nominal_mv;

    return battery_init_with_nb_cells_and_desc(pv_batt, pv_desc, nb_cells);
  }


  /**
   * Return the description for a battery cell type.
   *
   * @param[in] type_id the cell's type.
   *
   * @return the cell type's description.
   * @return NULL if no description has been found for the given type.
   */
  const BatteryCellDesc *battery_get_type_description_using_id(BatteryTypeId type_id)
  {
    uint8_t i;

    for(i = 0; i < BATT_TYPES_COUNT; i++)
    {
      if(_battery_descs[i].type_id == type_id) { return &_battery_descs[i]; }
    }

    return NULL;
  }

  /**
   * Return the description for a battery cell type using the type's name.
   *
   * @note the comparison is not case sensitive.
   *
   * @param[in] ps_type_name the type's name. Can be NULL or empty.
   *
   * @return the cell type's description.
   * @return NULL if no description has been found for the given type.
   */
  const BatteryCellDesc *battery_get_type_description_using_type_name(const char *ps_type_name)
  {
    uint8_t i;

    if(!ps_type_name || !*ps_type_name) { goto error_exit; }
    for(i = 0; i < BATT_TYPES_COUNT; i++)
    {
      if(strcasecmp(ps_type_name, _battery_descs[i].ps_type_name) == 0) { return &_battery_descs[i]; }
    }

    error_exit:
    return NULL;
  }




#ifdef __cplusplus
}
#endif
