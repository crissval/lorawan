/*
 * defs.h
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#ifndef DEFS_H_
#define DEFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <machine/endian.h>


#define PASTER2( x, y)           PASTER2_(x, y)
#define PASTER2_(x, y)           x##y
#define PASTER3( x, y, z)        PASTER3_(x, y, z)
#define PASTER3_(x, y, z)        x##y##z
#define PASTER4( a, b, c, d)     PASTER4_(a, b, c, d)
#define PASTER4_(a, b, c, d)     a##b##c##d
#define PASTER5( a, b, c, d, e)  PASTER5_(a, b, c, d, e)
#define PASTER5_(a, b, c, d, e)  a##b##c##d##e

#define _STRINGIFY(s)  #s
#define  STRINGIFY(s) _STRINGIFY(s)


#ifndef NULL
#define NULL 0
#endif

#ifndef __cplusplus

#ifndef bool
#define bool uint8_t
  enum
  {
    false = 0,
    true  = !false
  };
#endif  // bool
#endif  // __cplusplus

#ifndef bool8_t
#define bool8_t uint8_t
#endif


#ifndef UNUSED
#define UNUSED(v)  ((void)(v))
#endif


  typedef enum LogicLevel
  {
    LOW        = 0,
    HIGH       = 1,
    LEVEL_LOW  = LOW,
    LEVEL_HIGH = HIGH
  }
  LogicLevel;


  /**
   * Define sorting order
   */
  typedef enum SortOrder
  {
    SORT_ORDER_NONE,
    SORT_ORDER_ASCENDING,
    SORT_ORDER_DESCENDING
  }
  SortOrder;

  /**
   * Defines the bytes orders
   */
#ifndef LITTLE_ENDIAN
  typedef enum Endianness
  {
    LITTLE_ENDIAN,
    BIG_ENDIAN
  }
  Endianness;
#else
typedef uint16_t Endianness;
#endif

#ifndef __bswap24
#define __bswap24(u24)  ((uint32_t)(((u24) & 0xFF0000) >> 16 | ((u24) & 0xFF00) | ((u24) & 0xFF) << 16))
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define TO_LITTLE_ENDIAN_16(  u16)  (u16)
#define TO_BIG_ENDIAN_16(     u16)  __bswap16(u16)
#define FROM_LITTLE_ENDIAN_16(u16)  (u16)
#define FROM_BIG_ENDIAN_16(   u16)  __bswap16(u16)
#define TO_LITTLE_ENDIAN_24(  u24)  (u24)
#define TO_BIG_ENDIAN_24(     u24)  __bswap24(u24)
#define FROM_LITTLE_ENDIAN_24(u24)  (u24)
#define FROM_BIG_ENDIAN_24(   u24)  __bswap24(u24)
#define TO_LITTLE_ENDIAN_32(  u32)  (u32)
#define TO_BIG_ENDIAN_32(     u32)  __bswap16(u32)
#define FROM_LITTLE_ENDIAN_32(u32)  (u32)
#define FROM_BIG_ENDIAN_32(   u32)  __bswap16(u32)
#define TO_LITTLE_ENDIAN_64(  u64)  (u64)
#define TO_BIG_ENDIAN_64(     u64)  __bswap16(u64)
#define FROM_LITTLE_ENDIAN_64(u64)  (u64)
#define FROM_BIG_ENDIAN_64(   u64)  __bswap16(u64)
#elif  _BYTE_ORDER == _BIG_ENDIAN
#define TO_LITTLE_ENDIAN_16(  u16)  __bswap16(u16)
#define TO_BIG_ENDIAN_16(     u16)  (u16)
#define FROM_LITTLE_ENDIAN_16(u16)  __bswap16(u16)
#define FROM_BIG_ENDIAN_16(   u16)  (u16)
#define TO_LITTLE_ENDIAN_24(  u24)  __bswap16(u24)
#define TO_BIG_ENDIAN_24(     u24)  (u24)
#define FROM_LITTLE_ENDIAN_24(u24)  __bswap16(u24)
#define FROM_BIG_ENDIAN_24(   u24)  (u24)
#define TO_LITTLE_ENDIAN_32(  u32)  __bswap16(u24)
#define TO_BIG_ENDIAN_32(     u32)  (u32)
#define FROM_LITTLE_ENDIAN_32(u32)  __bswap16(u24)
#define FROM_BIG_ENDIAN_32(   u32)  (u32)
#define TO_LITTLE_ENDIAN_64(  u64)  __bswap16(u32)
#define TO_BIG_ENDIAN_64(     u64)  (u64)
#define FROM_LITTLE_ENDIAN_64(u64)  __bswap16(u32)
#define FROM_BIG_ENDIAN_64(   u64)  (u64)
#else
#error "This endianess is not supported"
#endif


#define NB_SECS_IN_A_MINUTE  60
#define NB_SECS_IN_AN_HOUR   3600
#define NB_SECS_IN_A_DAY     86400
#define NB_SECS_IN_A_WEEK    604800


/**
 * Returns the minimum value between a and b.
 *
 * @param[in] a 1st value.
 * @param[in] b 2nd value.
 *
 * @return the minimum value.
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * Returns the maximum value between a and b.
 *
 * @param[in] a 1st value.
 * @param[in] b 2nd value.
 *
 * @return the maximum value.
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


// Prepocessor directive to align buffer
#define ALIGN(n)  __attribute__((aligned(n)))



#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define _FUNCTION_NAME_ __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define _FUNCTION_NAME_ __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define _FUNCTION_NAME_ __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define _FUNCTION_NAME_ __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define _FUNCTION_NAME_ __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define _FUNCTION_NAME_ __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define _FUNCTION_NAME_ __func__
#endif


  // Macros used to find out if __VA_OPT__ macro is recognised by the compiler
  #define PP_THIRD_ARG(a,b,c,...) c
  #define VA_OPT_SUPPORTED_I(...) PP_THIRD_ARG(__VA_OPT__(,),true,false,)
  #define VA_OPT_SUPPORTED VA_OPT_SUPPORTED_I(?)

  #if VA_OPT_SUPPORTED
  #define _VARGS_(...)  __VA_OPT__(,) __VA_ARGS__
  #else // WARNING: this hack is supported by gcc but probably not by other compilers.
  #define _VARGS_(...)  , ##__VA_ARGS__
  #endif


#endif /* DEFS_H_ */
