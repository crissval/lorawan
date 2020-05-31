/**
 * Access to the random number generator.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include "rng.h"
#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif



  /**
   * Return a random number between 0 and 2^32.
   *
   * @return the random number.
   */
  uint32_t rng_u32(void)
  {
    RNG_HandleTypeDef rng;
    uint32_t          res;

    rng.Instance = RNG;
    rng.State    = HAL_RNG_STATE_RESET;

    __HAL_RCC_RNG_CLK_ENABLE();
    HAL_RNG_Init(  &rng);                      // there is no reason that this does not work.
    HAL_RNG_GenerateRandomNumber(&rng, &res);  // This one either.
    HAL_RNG_DeInit(&rng);
    __HAL_RCC_RNG_CLK_DISABLE();

    return res;
  }

  /**
   * Return a random number in a given range.
   *
   * @param[in] min the number's minimum value.
   * @param[in] max the number's maximum value.
   */
  uint32_t rng_u32_range(uint32_t min, uint32_t max)
  {
    return min  + rng_u32() % (min + max);
  }


#ifdef __cplusplus
}
#endif

