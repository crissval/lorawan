/*
 * Implementation for the murmur3 hash algorithm.
 *
 * This is the implementation for little-endian.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include "murmur3.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Generate 32 bits hash.
   *
   * @param[in] pu8_data the data to hash. MUST be NOT NULL.
   * @param[in] size     the number of data bytes to hash.
   * @param[in] seed     the seed to use.
   *
   * @return the data hash.
   */
  uint32_t mm3_32(const uint8_t *pu8_data, uint32_t size, uint32_t seed)
  {
    const uint32_t *key_x4;
    uint32_t        i, k;
    uint32_t        h = seed;

    if(size > 3)
    {
      key_x4 = (const uint32_t *)pu8_data;
      i      = size >> 2;
      do
      {
	k  = *key_x4++;
	k *= 0xcc9e2d51;
	k  = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	h ^= k;
	h  = (h << 13) | (h >> 19);
	h  = h * 5 + 0xe6546b64;
      }
      while(--i);

      pu8_data = (const uint8_t *)key_x4;
    }

    if(size & 3)
    {
      i        = size & 3;
      k        = 0;
      pu8_data = &pu8_data[i - 1];
      do
      {
	k <<= 8;
	k |= *pu8_data--;
      }
      while (--i);

      k *= 0xcc9e2d51;
      k  = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
    }

    h ^= size;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
  }


  /**
   * Start a murmur3 32 bits stream digest.
   *
   * @param[out] pv_stream the stream context object. MUST be NOT NULL.
   * @param[in]  seed      the seed to use.
   */
  void mm3_32_stream_init(MM332Stream *pv_stream, uint32_t seed)
  {
    pv_stream->h    = seed;
    pv_stream->size = 0;
  }

  /**
   * Digest a bunch of bytes into a murmur3 32 bits stream hash.
   *
   * @pre mm3_32_stream_init() MUST have been called before using this function.
   * @pre the intermediate data chunks' size must be a multiple of 4.
   *
   * @param[in,out] pv_stream the stream context object. MUST be NOT NULL.
   * @param[in]     pu8_data  the data to digest. MUST be NOT NULL.
   * @param[in]     size      the number of data bytes to digest.
   *                          Must be a multiple of 4 for intermediate data chunks.
   *                          Can be anything for the last chunk.
   */
  void mm3_32_stream_digest(MM332Stream *pv_stream, const uint8_t *pu8_data, uint32_t size)
  {
    const uint32_t *key_x4;
    uint32_t        i, k;
    uint32_t        h = pv_stream->h;

    if(size > 3)
    {
      key_x4 = (const uint32_t *)pu8_data;
      i      = size >> 2;
      do
      {
	k  = *key_x4++;
	k *= 0xcc9e2d51;
	k  = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	h ^= k;
	h  = (h << 13) | (h >> 19);
	h  = h * 5 + 0xe6546b64;
      }
      while(--i);

      pu8_data = (const uint8_t *)key_x4;
    }

    if(size & 3)
    {
      i        = size & 3;
      k        = 0;
      pu8_data = &pu8_data[i - 1];
      do
      {
	k <<= 8;
	k |= *pu8_data--;
      }
      while (--i);

      k *= 0xcc9e2d51;
      k  = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
    }

    pv_stream->h     = h;
    pv_stream->size += size;
  }

  /**
   * End a murmur3 32 bits stream digest and return the resulting hash.
   *
   * @pre mm3_32_stream_init() MUST have been called before using this function.
   *
   * @param[in] pv_stream the stream context object. MUST be NOT NULL.
   *
   * @return the hash.
   */
  uint32_t mm3_32_stream_finish(MM332Stream *pv_stream)
  {
    uint32_t h = pv_stream->h;

    h ^= pv_stream->size;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
  }



#ifdef __cplusplus
}
#endif




