/**
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <ctype.h>
#include <string.h>
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Check if a string contains only digits.
   *
   * @param[in] ps_str the string. MUST be NOT NULL.
   * @param[in] size   the size of the buffer.
   *
   * @return true  if the string only contains digits.
   * @return true  if the string is empty.
   * @return false otherwise.
   */
  bool strn_contains_only_digits(const char *ps_str, uint32_t size)
  {
    const char *pc_end = ps_str + size;

    for(; *ps_str && ps_str != pc_end; ps_str++)
    {
      if(*ps_str < '0' || *ps_str > '9') return false;
    }

    return true;
  }

  /**
   * Convert a string to an unsigned value.
   *
   * The string cannot contain a sign, even the '+' sign.
   * Must only contain digits.
   *
   * @param[in] ps_str the string. MUST be NOT NULL.
   * @param[in] size   the size of the buffer.
   *
   * @return the value.
   * @return -1 if the string does not translate to an unsigned integer value.
   */
  int32_t strn_nosign_to_uint(const char *ps_str, uint32_t size)
  {
    const char *pc_end = ps_str + size;
    int32_t     res    = 0;

    for(; *ps_str && ps_str != pc_end; ps_str++)
    {
      if(*ps_str < '0' || *ps_str > '9') return -1;

      res = res * 10 + *ps_str - '0';
    }

    return res;
  }

  /**
   * Convert a string to an unsigned value.
   *
   * The string cannot contain a sign, even the '+' sign.
   * Must only contain digits.
   *
   * @param[in] ps_str        the string. MUST be NOT NULL.
   * @param[in] default_value the default value to return if the conversion failed.
   *
   * @return the value.
   * @return the default value if the conversion failed.
   */
  uint32_t str_nosign_to_uint_with_default(const char *ps_str, uint32_t default_value)
  {
    int32_t res = 0;

    for( ; *ps_str; ps_str++)
    {
      if(*ps_str < '0' || *ps_str > '9') return default_value;

      res = res * 10 + *ps_str - '0';
    }

    return res;
  }


  /**
   * Converts an unsigned integer to a string.
   *
   * @param[in,out] ps_dest      pointer of the first character to write. MUST be NOT NULL.
   * @param[in]     value        the unsigned integer value to convert.
   * @param[in]     n            the maximum number of characters to write.
   * @param[in]     point_to_end does the returned pointer points to '\0' ending character
   *                             of the resulting string or to its beginning?
   *
   * @return a pointer to the beginning or the end of the resulting string,
   *         depending on the value of the <code>point_to_end</code> parameter.
   * @return an empty string if <code>n</code> is too small for the given value.
   */
  char *strn_uint_to_string(char *ps_dest, uint32_t value, uint32_t n, bool point_to_end)
  {
    char *ps, *pe, *pc, c;

    if(n <= 2)
    {
      *ps_dest = '\0';
      return ps_dest;
    }

    // Get a reversed string from the value
    pc = ps_dest;
    do
    {
      *pc++  = (value % 10) + '0';
      value /= 10;
      n--;
    }
    while(value && n);
    if(value)
    {
      // Number is more than n digits long.
      *ps_dest = '\0';
      return ps_dest;
    }
    *pc = '\0';

    // Reverse the string to get the final string
    pe = pc;
    for(ps = ps_dest, pc = pe - 1; pc > ps; ps++, pc--)
    {
      c   = *ps;
      *ps = *pc;
      *pc = c;
    }

    return point_to_end ? pe : ps_dest;
  }

  /**
   * Converts a signed integer to a string.
   *
   * @param[in,out] ps_dest      pointer of the first character to write. MUST be NOT NULL.
   * @param[in]     value        the signed integer value to convert.
   * @param[in]     n            the maximum number of characters to write.
   * @param[in]     add_sign     add a plus sign to positive values?
   * @param[in]     point_to_end does the returned pointer points to the end of the resulting string or
   *                             to its beginning?
   *
   * @return a pointer to the beginning or the end of the resulting string,
   *         depending on the value of the <code>point_to_end</code> parameter.
   * @return an empty string if <code>n</code> is too small for the given value.
   */
  char *strn_int_to_string(char *ps_dest, int32_t value, uint32_t n, bool add_sign, bool point_to_end)
  {
    char *ps, *pe, *pd, *pc, c;

    if(n <= 2)
    {
      *ps_dest = '\0';
      return ps_dest;
    }

    // Set the sign
    pd       = ps_dest;
    add_sign = add_sign || value < 0;
    if(add_sign)
    {
      *pd++ = value >= 0 ? '+' : '-';
      n--;
    }

    // Get a reversed string from the value
    pc = pd;
    do
    {
      *pc++  = (value % 10) + '0';
      value /= 10;
      n--;
    }
    while(value && n);
    if(value)
    {
      // Number is more than n digits long.
      *ps_dest = '\0';
      return ps_dest;
    }
    *pc = '\0';

    // Reverse the string to get the final string
    pe = pc;
    for(ps = pd, pc = pe - 1; pc > ps; ps++, pc--)
    {
      c   = *ps;
      *ps = *pc;
      *pc = c;
    }

    return point_to_end ? pe : ps_dest;
  }

  /**
   * Convert a string to an unsigned integer value.
   *
   * @note overflow are not detected by this function.
   *
   * @param[in] ps_src the string. MUST be NOT NULL.
   * @param[in] size   the maximum number of characters read from the string.
   * @param[in] pb_ok  set to true if the conversion has been successful.
   *                   Set to false otherwise.
   *                   Can be NULL if we are not interested in this information.
   *
   * @return the unsigned integer value of the number represented in the string.
   * @return 0 in case of error.
   */
  uint32_t strn_string_to_uint(const char *ps_src, uint32_t size, bool *pb_ok)
  {
    char    *p_c;
    uint32_t res = 0;

    // Check if we have something to work with
    if(!*ps_src || !size)
    {
      if(pb_ok) { *pb_ok = false; }
      return 0;
    }

    // If we begin by a sign then skip it.
    if(*ps_src == '+')
    {
      ps_src++;
      size--;
    }

    // Convert
    for(p_c = (char *)ps_src ; *p_c && size; p_c++, size--)
    {
      // Check that we have a digit
      if(*p_c < '0' || *p_c > '9')
      {
	if(pb_ok) { *pb_ok = false; }
	return 0;
      }

      res = res * 10 + (*p_c - '0');
    }

    if(pb_ok) { *pb_ok = true; }
    return res;
  }

  /**
   * Convert a string to a signed integer value.
   *
   * @note overflow are not detected by this function.
   *
   * @param[in] ps_src the string. MUST be NOT NULL.
   * @param[in] size   the maximum number of characters read from the string.
   * @param[in] pb_ok  set to true if the conversion has been successful.
   *                   Set to false otherwise.
   *                   Can be NULL if we are not interested in this information.
   *
   * @return the unsigned integer value of the number represented in the string.
   * @return 0 in case of error.
   */
  int32_t strn_string_to_int(const char *ps_src, uint32_t size, bool *pb_ok)
  {
    char    *p_c;
    bool     dummy;
    uint32_t res = 0;

    // Check if we have something to work with
    if(!pb_ok) { pb_ok = &dummy; }
    if(!*ps_src || !size)
    {
      *pb_ok = false;
      return 0;
    }

    // If we begin by a sign then skip it. We'll handle it later.
    p_c = (char *)ps_src;
    if(*ps_src == '+' || *ps_src == '-')
    {
      p_c++;
      size--;
    }

    // Convert
    for( ; *p_c && size; p_c++, size--)
    {
      // Check that we have a digit
      if(*p_c < '0' || *p_c > '9')
      {
	*pb_ok = false;
	return 0;
      }

      res = res * 10 + (*p_c - '0');
    }

    *pb_ok = true;
    return *ps_src == '-' ? -res : res;
  }


  /**
   * Convert a string to a float value.
   *
   * Leading space characters are ignored. Conversion stops as soon as a non digit is found.
   * For example, the value 45.789 is returned for the string  "  \t45.789rft".
   *
   * @param[in] ps_src        the string. Can be NULL or empty.
   * @param[in] size          the string size.
   * @param[in] default_value the default value.
   *
   * @return the float value.
   * @return default_value if the string does not contain a float value.
   */
  float strn_string_to_float_trim_with_default(const char *ps_src,
					       uint32_t    size,
					       float       default_value)
  {
    const char *ps_end    = ps_src + size;
    uint32_t    value     = 0;
    uint32_t    div       = 0;
    bool        negative  = false;
    float       res       = default_value;

    // If no string then return default value.
    if(!ps_src || !*ps_src) { goto exit; }

    // Trim leading space
    for( ; isspace(*ps_src) && ps_src < ps_end; ps_src++) ;  // Do nothing

    // Check sign
    if(     *ps_src == '+') { ps_src++;                  } // Skip it
    else if(*ps_src == '-') { ps_src++; negative = true; }

    // Check that first character is a digit
    if(ps_src < ps_end && !isdigit(*ps_src)) { goto exit; }  // Return default value

    // Convert
    for( ; ps_src < ps_end && *ps_src; ps_src++)
    {
      if(*ps_src == '.')    { div = 1; continue; }
      if(!isdigit(*ps_src)) { break;             }

      value = value * 10 + (*ps_src - '0');
      div  *= 10;
    }
    res = div ? ((float)value) / ((float)div) : ((float)value);
    if(negative) { res = -res; }

    exit:
    return res;
  }


  /**
   * Convert a string to an unsigned integer value until it reaches a given separator
   * or the end of the string.
   *
   * @note overflow are not detected by this function.
   *
   * @param[in]  ps_src        the string. MUST be NOT NULL.
   * @param[in]  size          the maximum number of characters read from the string.
   * @param[in]  sep           the separator used to indicate the end of the number.
   * @param[in]  default_value the default value to use in case of error.
   * @param[out] pps_end       points to the last character considered for the conversion.
   *                           It can be the character separator, the null character
   *                           or the character that made the conversion fail.
   *                           Can be set to NULL if you are not interested in this information.
   *
   * @return the unsigned integer value of the number represented in the string.
   * @return the default value in case of error.
   */
  uint32_t strn_to_uint_with_default_and_sep(const char *ps_src,
					     uint32_t    size,
					     char        sep,
					     uint32_t    default_value,
					     char      **pps_end)
  {
    char    *_pps_end;
    uint32_t res = 0;

    if(!pps_end)            { pps_end = &_pps_end;                }
    if(!ps_src || !*ps_src) { res     = default_value; goto exit; }

    // Convert
    for( ; *ps_src && size && *ps_src != sep; ps_src++, size--)
    {
      // Check that we have a digit
      if(*ps_src < '0' || *ps_src > '9') { res = default_value; goto exit; }

      res = res * 10 + (*ps_src - '0');
    }

    exit:
    *pps_end = (char *)ps_src;
    return res;
  }


  /**
   * Convert a string to a float value until it reaches a given separator or the end of the string.
   *
   * @note overflow are not detected by this function.
   *
   * @param[in]  ps_src        the string. MUST be NOT NULL.
   * @param[in]  size          the maximum number of characters read from the string.
   * @param[in]  sep           the separator used to indicate the end of the number.
   * @param[in]  default_value the default value to use in case of error.
   * @param[out] pps_end       points to the last character considered for the conversion.
   *                           It can be the character separator, the null character
   *                           or the character that made the conversion fail.
   *                           Can be set to NULL if you are not interested in this information.
   *
   * @return the unsigned integer value of the number represented in the string.
   * @return the default value in case of error.
   */
  extern float strn_to_float_with_default_and_sep(const char *ps_src,
						  uint32_t    size,
						  char        sep,
						  float       default_value,
						  char      **pps_end)
  {
    char    *_pps_end;
    float    res      = default_value;
    uint32_t value    = 0;
    uint32_t div      = 0;
    bool     negative = false;

    if(!pps_end)            { pps_end = &_pps_end; }
    if(!ps_src || !*ps_src) { goto exit;           }

    // Check sign
    if(     *ps_src == '+') { ps_src++;                  } // Skip it
    else if(*ps_src == '-') { ps_src++; negative = true; }

    // Convert
    for( ; *ps_src && size && *ps_src != sep; ps_src++, size--)
    {
      if(*ps_src == '.') { div = 1; continue; }

      // Check that we have a digit
      if(*ps_src < '0' || *ps_src > '9') { goto exit; }

      value = value * 10 + (*ps_src - '0');
      div  *= 10;
    }
    res = ((float)value) / ((float)div);
    if(negative) { res = -res; }

    exit:
    *pps_end = (char *)ps_src;
    return res;
  }


  /**
   * Copy a string and return a pointer to the end of the string.
   *
   * The idea is to use it to build strings without needing to get the strings' lengths.
   * Copies <code>ps_src</code> starting at destination address <code>ps_dest</code>
   * until the end of <code>ps_src</code> or until <code>n</code> characters have been copied.
   *
   * @param[in,out] ps_dest     pointer to the first address to write to. MUST be NOT NULL.
   * @param[in] ps_src          the string to copy. Can be NULL or empty.
   * @param[in] n               the maximum number of characters to write.
   * @param[in] null_terminated is the resulting string null terminated or not?
   *                            If it is then the returned pointer points to the ending null character,
   *                            otherwise the returned pointer points the the next address after the last
   *                            character copied.
   *
   * @return the pointer to the end of the resulting string.
   */
  char *strncpy_return_end(char *ps_dest, const char *ps_src, uint32_t n, bool null_terminated)
  {
    if(ps_src && n)
    {
      if(null_terminated) { n--; } // To make sure that we have enough room for the null ending character.
      for(; *ps_src && n; n--)
      {
	*ps_dest++ = *ps_src++;
      }
      if(null_terminated) { *ps_dest = '\0'; }
    }

    return ps_dest;
  }


  /**
   * Trim a string that is indicated by a pointers to it's first and last character.
   *
   * @param[in]     pc_start pointer to the string's first character. MUST be NOT NULL.
   * @param[in,out] ppc_end  address of a pointer to the string's last character.
   *                         After the function has been called it will point to the last character
   *                         of the trimmed string. No null character is added at the end of the trimmed
   *                         string.
   *                         If set to NULL then pc_start MUST point to a valid string (null terminated)
   *                         and the source string will me modified to move the null character at the
   *                         end of the trimmed string.
   *
   * @return a pointer to the first character of the trimmed string.
   * @return NULL if the trimmed string is empty.
   */
  char *str_trim(char *pc_start, char **ppc_end)
  {
    char *pc_end;

    if(ppc_end)
    {
      while( pc_start <= *ppc_end && isspace((unsigned char)*pc_start))   pc_start++;
      while( *ppc_end >= pc_start && isspace((unsigned char)**ppc_end)) (*ppc_end)--;

      return pc_start <= *ppc_end ? pc_start : NULL;
    }

    while(*pc_start && isspace((unsigned char)*pc_start)) pc_start++;
    if(  !*pc_start) { return NULL; }
    for(pc_end = pc_start + strlen(pc_start) - 1; isspace((unsigned char)*pc_end); pc_end--);

    pc_end[1]  = '\0';
    return pc_start;
  }


  /**
   * Convert a string containing a number, represented in base 2, 10 or 16 to a 32 bits unsigned integer.
   *
   * @param[in]  ps            the string to work with.
   *                           In base 2 the string must begin with "0b" or "0B"
   *                           and contains only '0' and '1' characters.
   *                           In base 10 the string should contain only digit characters.
   *                           In base 16 the string must begin with "0x" or "0X".
   * @param[in]  default_value the default value to use if the conversion fails.
   * @param[out] pb_ok         set to true if the conversion was successful.
   *                           Set to false otherwise.
   *                           Can be NULL if we are not interested in this information.
   *
   * @return the result from the conversion.
   * @return the default value in case of error.
   */
  uint32_t inthexbinarystr_to_uint32(const char *ps, uint32_t default_value, bool *pb_ok)
  {
    bool     ok;
    uint32_t v = 0;

    if(!pb_ok) { pb_ok = &ok; }
    *pb_ok = true;

    if(!ps || !*ps) { goto exit_error; }

    if(*ps++ == '0')
    {
      if(!*ps) { return v; }

      if(     *ps == 'b' || *ps == 'B')  // Binary string
      {
	for(++ps; *ps; ps++)
	{
	  if(*ps != '0' && *ps != '1') { goto exit_error; }
	  v = v * 2 + (*ps - '0');
	}
      }
      else if(*ps == 'x' || *ps == 'X')  // Hexa string
      {
	for(++ps; *ps; ps++)
	{
	  v *= 16;
	  if(     *ps >= '0' && *ps <= '9') { v += *ps - '0';      }
	  else if(*ps >= 'a' && *ps <= 'z') { v += *ps - 'a' + 10; }
	  else if(*ps >= 'A' && *ps <= 'Z') { v += *ps - 'A' + 10; }
	  else                              { goto exit_error;    }
	}
      }
      else { goto exit_error; }
    }
    else
    {
      v = strn_string_to_uint(ps, 10, pb_ok);  // 10 because a uint32 value is 10 characters long at maximum
    }

    if(!*pb_ok) { v = default_value; }
    return v;

    exit_error:
    *pb_ok = false;
    return default_value;
  }

#ifdef __cplusplus
}
#endif
