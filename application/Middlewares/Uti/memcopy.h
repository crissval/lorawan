/**
 * Functions to copy data between memory blocks.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef UTI_MEMCOPY_H_
#define UTI_MEMCOPY_H_

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif

  extern uint8_t *memcopy_reverse(uint8_t *pu8_dest, const uint8_t *pu8_src, uint16_t size);

#ifdef __cplusplus
}
#endif
#endif /* UTI_MEMCOPY_H_ */
