/**
 * @file  utils.h
 * @brief Definition of a variety of useful functions and macros.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif


#define char_is_space(   c) ((c)  == ' ' || (c) == '\t')
#define char_is_alpha(   c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define char_is_digit(   c)  ('0' <= (c) && (c) <= '9')
#define char_is_alphanum(c) (is_char_alpha(c) || is_char_digit(c))

#define str_is_empty(ps)         ((ps)[0] == '\0')
#define str_is_null_or_empty(ps) (!(ps) || (ps)[0] == '\0')

  extern bool     strn_contains_only_digits(const char *ps_str, uint32_t size);
  extern int32_t  strn_nosign_to_uint(      const char *ps_str, uint32_t size);

  extern uint32_t str_nosign_to_uint_with_default(const char *ps_str, uint32_t default_value);

  extern char *   strn_uint_to_string(char *ps_dest, uint32_t value, uint32_t n, bool point_to_end);
  extern char *   strn_int_to_string( char *ps_dest, int32_t  value, uint32_t n,
				      bool add_sign, bool     point_to_end);

  extern uint32_t strn_string_to_uint(const char *ps_src, uint32_t size, bool *pb_ok);
  extern int32_t  strn_string_to_int( const char *ps_src, uint32_t size, bool *pb_ok);

  extern float    strn_string_to_float_trim_with_default(const char *ps_src,
							 uint32_t    size,
							 float       default_value);

  extern uint32_t strn_to_uint_with_default_and_sep(const char *ps_src,
						    uint32_t    size,
						    char        sep,
						    uint32_t    default_value,
						    char      **pps_end);

  extern float    strn_to_float_with_default_and_sep(const char *ps_src,
						     uint32_t    size,
						     char        sep,
						     float       default_value,
						     char      **pps_end);

  extern char *strncpy_return_end(char *ps_dest, const char *ps_src, uint32_t n, bool null_terminated);

  extern char *str_trim(char *pc_start, char **ppc_end);

  extern uint32_t inthexbinarystr_to_uint32(const char *ps, uint32_t default_value, bool *pb_ok);


#ifdef __cplusplus
}
#endif
#endif /* __UTILS_H__ */
