/*
 * Data Types SolutionConductivityUSCm and SolutionConductivityMSCm for ConnecSenS RF data frames.
 *
 *  Created on: Jul 20, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLUTIONCONDUCTIVITY_H_
#define CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLUTIONCONDUCTIVITY_H_

#include "cnssrf-dataframe.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defines the flags for the SolutionConductivity Data Types
   */
  typedef enum CNSSRFDTSolutionConductivityFlag
  {
    CNSSRF_DT_SOLUTION_CONDUCTIVITY_FLAG_NONE       = 0x00,  ///< No flag.
    CNSSRF_DT_SOLUTION_CONDUCTIVITY_FLAG_ALARM_LOW  = 0x01,  ///< The low alarm is active.
    CNSSRF_DT_SOLUTION_CONDUCTIVITY_FLAG_ALRAM_HIGH = 0x02   ///< The high alarm is active.
  }
  CNSSRFDTSolutionConductivityFlag;
  typedef uint8_t CNSSRFDTSolutionConductivityFlags;

  /**
   * Defines the type, a,d the units, of the solution conductivity.
   */
  typedef enum CNSSRFDTSolutionConductivityType
  {
    CNSSRF_DT_SOLUTION_CONDUCTIVITY_USCM,
    CNSSRF_DT_SOLUTION_CONDUCTIVITY_MSCM,
    CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_USCM,
    CNSSRF_DT_SOLUTION_SPECIFIC_CONDUCTIVITY_MSCM
  }
  CNSSRFDTSolutionConductivityType;


  bool cnssrf_dt_solutionconductivity_write_to_frame(CNSSRFDataFrame                  *pv_frame,
						     float                             value,
						     CNSSRFDTSolutionConductivityType  type,
						     CNSSRFDTSolutionConductivityFlags flags);


#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_DATATYPES_CNSSRF_DT_SOLUTIONCONDUCTIVITY_H_ */
