/*
 * Data Type TempDegC and TempDegCLowRes for ConnecSenS RF data frames
 *
 *  Created on: 16 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dt_temperature.h"
#include "cnssrf-datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DATA_TYPE_DEGC_ID  0x02
#define DATA_TYPE_DEGC_VALUE_MIN  CNSSRF_DT_TEMPERATURE_DEGC_VALUE_ERROR
#define DATA_TYPE_DEGC_VALUE_MAX  1365.1
#define DATA_TYPE_DEGC_OFFSET     CNSSRF_DT_TEMPERATURE_DEGC_VALUE_ERROR

#define DATA_TYPE_DEGC_LOWRES_ID         0x03
#define DATA_TYPE_DEGC_LOWRES_VALUE_MIN  (-50.0)
#define DATA_TYPE_DEGC_LOWRES_VALUE_MAX    77.5

  //**AJOUT**********************************************************************step3
  #define DATA_TYPE_DELTA_DEGC_ID         0x31
  #define DATA_TYPE_DELTA_DEGC_VALUE_MIN  CNSSRF_DT_DELTA_TEMP_DEGC_VALUE_ERROR
  #define DATA_TYPE_DELTA_DEGC_VALUE_MAX  3276.7
  //*****************************************************************************


  /**
   * Writes a TempDegC Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     degc     the temperature, in Celsius degrees.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_temperature_write_degc_to_frame(CNSSRFDataFrame            *pv_frame,
						 float                       degc,
						 CNSSRFDTTemperatureDegCFlag flags)
  {
    CNSSRFValue value;

    if(degc < DATA_TYPE_DEGC_VALUE_MIN || degc > DATA_TYPE_DEGC_VALUE_MAX) { return false; }

    degc = (degc - DATA_TYPE_DEGC_OFFSET) * 10.0;

    value.type         = CNSSRF_VALUE_TYPE_UINT16;
    value.value.uint16 = (((uint16_t)(degc + 0.5)) & 0x3FFF) | (((uint16_t)flags) << 14);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_DEGC_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }

  /**
   * Writes a TempDegCLowRes Data Type to a frame.
   *
   * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
   * @param[in]     degc     the temperature, in Celsius degrees.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_dt_temperature_write_degc_lowres_to_frame(CNSSRFDataFrame *pv_frame, float degc)
  {
    CNSSRFValue value;

    if(degc < DATA_TYPE_DEGC_LOWRES_VALUE_MIN || degc > DATA_TYPE_DEGC_LOWRES_VALUE_MAX) { return false; }

    value.type        = CNSSRF_VALUE_TYPE_UINT8;
    value.value.uint8 = (uint8_t)((degc - DATA_TYPE_DEGC_LOWRES_VALUE_MIN) * 2);

    return cnssrf_data_type_write_values_to_frame(pv_frame,
						  DATA_TYPE_DEGC_LOWRES_ID,
						  &value, 1,
						  CNSSRF_META_DATA_FLAG_NONE);
  }

  /**AJOUT
     * Writes a DeltaTempDegC Data Type to a frame.
     *
     * @param[in,out] pv_frame the data frame to write to. MUST be NOT NULL. MUST have been initialised.
     * @param[in]     degc     the temperature delta, in Celsius degrees.
     *
     * @return true  on success.
     * @return false otherwise.
     */
    bool cnssrf_dt_delta_temp_write_degc_to_frame(CNSSRFDataFrame *pv_frame, float degc)
    {
      CNSSRFValue value;//we'll write a single value nous allons écrire une seule valeur
      //check le deltaT dans les valeurs admissibles
      if(degc < DATA_TYPE_DELTA_DEGC_VALUE_MIN || degc > DATA_TYPE_DELTA_DEGC_VALUE_MAX) { return false; }
      //configurer la valeur pour écrire dans la trame de données
      value.type        = CNSSRF_VALUE_TYPE_INT16;
      value.value.int16 = (int16_t)(degc * 10);
       //écrire la valeur dans la trame de donnée  (le data frame.)
      return cnssrf_data_type_write_values_to_frame(pv_frame,
  						  DATA_TYPE_DELTA_DEGC_ID,
  						  &value, 1,
  						  CNSSRF_META_DATA_FLAG_NONE);
    }                                    //pointeur, data type id,  seulement 1 valeur à écrire, pas de drapeau flag



#ifdef __cplusplus
}
#endif



