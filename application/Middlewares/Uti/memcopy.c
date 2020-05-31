/**
 * Functions to copy data between memory blocks.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include "memcopy.h"


#ifdef __cplusplus
extern "C" {
#endif

  uint8_t *memcopy_reverse(uint8_t *pu8_dest, const uint8_t *pu8_src, uint16_t size)
  {
    uint8_t *pu8;

    for(pu8 = pu8_dest + (size - 1); size--; ) { *pu8-- = *pu8_src++; }

    return pu8_dest;
  }

#ifdef __cplusplus
}
#endif


