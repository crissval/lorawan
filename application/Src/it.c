/**
 * Macros and functions related to interruptions.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <it.h>
#include "board.h"


#ifdef __cpplusplus
extern "C" {
#endif


  uint32_t it_cs_depth   = 0;  ///< Critical section depth counter
  uint32_t it_cs_primask;      ///< The primask saved for the top most critical section.


  /**
   * Enter a critical section.
   *
   * @post The interruptions are disabled.
   *
   * @param[in] primask the primask value to save. Is only saved for the first level of critical section.
   */
  void it_enter_critical_section(uint32_t primask)
  {
    DISABLE_IRQ();
    if(!it_cs_depth++) { it_cs_primask = primask; }
  }

  /**
   * Exit critical section.
   *
   * @post if this call closes top nested critical section then the interruptions are restored
   *       as they were before entering the first nested critical section.
   */
  void it_exit_critical_section(void)
  {
    it_cs_depth--;
    if(it_cs_depth == 0) { __set_PRIMASK(it_cs_primask); }
  }


#ifdef __cpplusplus
}
#endif
