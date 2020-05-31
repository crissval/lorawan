/**
 * Macros and functions related to interruptions.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef IT_H_
#define IT_H_

#include "defs.h"


#ifdef __cpplusplus
extern "C" {
#endif


  // BACKUP_PRIMASK MUST be implemented at the beginning of the function
  //   that implement a critical section
  //   PRIMASK is saved on STACK and recovered at the end of the function
  //   That way RESTORE_PRIMASK ensures that no irq would be triggered in case of
  //   unbalanced enable/disable, reentrant code etc...
  #define BACKUP_PRIMASK()   uint32_t primask_bit = __get_PRIMASK()
  #define DISABLE_IRQ()      __disable_irq()
  #define ENABLE_IRQ()       __enable_irq()
  #define RESTORE_PRIMASK()  __set_PRIMASK(primask_bit)


#define ENTER_CRITICAL_SECTION()  it_enter_critical_section(__get_PRIMASK())
#define EXIT_CRITICAL_SECTION()   it_exit_critical_section()

  extern void it_enter_critical_section(uint32_t primask);
  extern void it_exit_critical_section( void);


#ifdef __cpplusplus
}
#endif
#endif /* IT_H_ */
