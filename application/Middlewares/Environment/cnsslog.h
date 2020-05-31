/*
 * Initialisation of the ConnecSenS log sub-system.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef ENVIRONMENT_CNSSLOG_H_
#define ENVIRONMENT_CNSSLOG_H_


#ifdef __cplusplus
extern "C" {
#endif


  extern void cnsslog_init(  void);
  extern void cnsslog_sleep( void);
  extern void cnsslog_wakeup(void);

  extern void cnsslog_enable_serial_logging(const char *ps_usart_name);


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_CNSSLOG_H_ */
