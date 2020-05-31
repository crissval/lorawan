/*
 * Data Types SoilMoistureCb and SoilMoistureCbDegCHz for ConnecSenS RF data frames.
 *
 *  Created on: 10 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOILMOISTURE_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOILMOISTURE_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defined the flags used for a SoilMoitureCb Data Type.
   */
  typedef enum CNSSRFDTSoilMoistureCbFlag
  {
    CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_NONE       = 0x00,
    CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_LOW  = 0x01,
    CNSSRF_DT_SOIL_MOISTURE_CB_FLAG_ALARM_HIGH = 0x02
  }
  CNSSRFDTSoilMoistureCbFlag;
  typedef uint8_t CNSSRFDTSoilMoistureCbFlags;


  /**
   * Defines the type used to store the reading values for a SoilMoitureCb Data Type.
   */
  typedef struct CNSSRFDTSoilMoistureCbReading
  {
    CNSSRFDTSoilMoistureCbFlags flags;      ///< The flags.
    uint8_t                     centibars;  ///< The soil moisture in centibars.
    uint16_t                    depth_cm;   ///< The depth, in centimeters.
  }
  CNSSRFDTSoilMoistureCbReading;

  /**
   * Defines the type used to store the reading values for a SoilMoitureCbDegCHz Data Type.
   */
  typedef struct CNSSRFDTSoilMoistureCbDegCHzReading
  {
    CNSSRFDTSoilMoistureCbReading base;

    float    tempDegC; ///< The soil temperature, in Celsius degrees.
    uint32_t freqHz;   ///< The sensor's frequency, in Hz.
  }
  CNSSRFDTSoilMoistureCbDegCHzReading;
#define DATA_TYPE_SOIL_MOISTURE_CBDEGCHZ_DEGC_ERROR    77.5 ///< The temperature value error for the SoilMoistureCbDegCHz Data Type

  bool cnssrf_dt_soilmoisture_write_moisture_cb_to_frame(CNSSRFDataFrame               *pv_frame,
							 CNSSRFDTSoilMoistureCbReading *pv_readings,
							 uint8_t                        nb_readings);

  bool cnssrf_dt_soilmoisture_write_moisture_cbdegchz_to_frame(CNSSRFDataFrame                     *pv_frame,
							       CNSSRFDTSoilMoistureCbDegCHzReading *pv_readings,
							       uint8_t                              nb_readings);

  bool cnssrf_dt_soilmoisture_write_volumetric_content_percent_to_frame(CNSSRFDataFrame *pv_frame,
									float            percent);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOILMOISTURE_H_ */
