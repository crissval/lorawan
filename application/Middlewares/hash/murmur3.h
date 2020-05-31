/*
 * Implementation for the murmur3 hash algorithm.
 *
 * This is the implementation for little-endian.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef HASH_MURMUR3_H_
#define HASH_MURMUR3_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MM3_SEED_CONNECSENS  0x434E5353


  /**
   * Type used to store the stream context for a 32 bits murmur3 hash.
   */
  typedef struct MM332Stream
  {
    uint32_t h;     ///< The current hash value
    uint32_t size;  ///< The amount of bytes digested yet.
  }
  MM332Stream;


  extern uint32_t mm3_32(const uint8_t *pu8_data, uint32_t size, uint32_t seed);
#define mm3_32_cnss(pu8_data, size)  mm3_32(pu8_data, size, MM3_SEED_CONNECSENS)

  extern void     mm3_32_stream_init(  MM332Stream   *pv_stream, uint32_t seed);
  extern void     mm3_32_stream_digest(MM332Stream   *pv_stream,
				       const uint8_t *pu8_data,
				       uint32_t       size);
  extern uint32_t mm3_32_stream_finish(MM332Stream   *pv_stream);
#define mm3_32_stream_init_cnss(pv_stream) \
  mm3_32_stream_init(pv_stream, MM3_SEED_CONNECSENS)


#ifdef __cplusplus
}
#endif
#endif /* HASH_MURMUR3_H_ */
