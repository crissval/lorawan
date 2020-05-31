/*
 * Function, macros and types in relation with physical units
 *
 *  Created on: Jul 20, 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef UNITS_H_
#define UNITS_H_


#ifdef __cplusplus
extern "C" {
#endif


#define units_PSI_to_kPa(psi)  ((psi) * 6.894757)

#define units_degF_to_degC(degF) (((degF) - 32) / 1.8)

#define units_feet_to_meters(feet)  ((feet) * 0.3048)


#ifdef __cplusplus
}
#endif
#endif /* UNITS_H_ */
