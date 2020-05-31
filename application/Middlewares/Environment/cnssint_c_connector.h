/**
 * C connector for using some of CNSSInt functions.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */

#ifndef ENVIRONMENT_CNSSINT_C_CONNECTOR_H_
#define ENVIRONMENT_CNSSINT_C_CONNECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif


  extern void cnssint_clear_internal_interruptions(void);
  extern void cnssint_clear_external_interruptions(void);


#ifdef __cplusplus
}
#endif



#endif /* ENVIRONMENT_CNSSINT_C_CONNECTOR_H_ */
