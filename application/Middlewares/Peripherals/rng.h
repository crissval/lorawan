/**
 * Access to the random number generator.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */

#ifndef PERIPHERALS_RNG_H_
#define PERIPHERALS_RNG_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif


  extern uint32_t rng_u32(void);
  extern uint32_t rng_u32_range(uint32_t min, uint32_t max);


#ifdef __cplusplus
}
#endif
#endif /* PERIPHERALS_RNG_H_ */
