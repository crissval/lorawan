/**
 * C connector for using some of CNSSInt functions.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#include "cnssint_c_connector.h"
#include "cnssint.hpp"

#ifdef __cplusplus
extern "C" {
#endif


  void cnssint_clear_internal_interruptions(void)
  {
    CNSSInt::instance()->clearInternalInterruptions();
  }

  void cnssint_clear_external_interruptions(void)
  {
    CNSSInt::instance()->clearExternalInterruptions();
  }


#ifdef __cplusplus
}
#endif
