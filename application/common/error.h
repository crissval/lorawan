/**
 * @file  error.h
 * @brief Header file for the error handling
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __ERROR_H__
#define __ERROR_H__


#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifndef fatal_error_handler
#error "No handler has been defined for fatal errors"
#endif
#ifndef fatal_error_exit_strategy
#define fatal_error_exit_strategy()  while(1)
#endif
#define fatal_error()                     fatal_error_with_msg(NULL)
#define fatal_error_with_msg(ps_msg, ...) do \
    { \
      fatal_error_handler(__FILE__, __LINE__, _FUNCTION_NAME_, ps_msg, __VA_ARGS__); \
      fatal_error_exit_strategy(); \
    } \
    while(0)


#ifdef __cplusplus
}
#endif
#endif /* __ERROR_H__ */
