/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Helper functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
/**
 * Redefinition of rand() and srand() standard C functions.
 * These functions are redefined in order to get the same behavior across
 * different compiler toolchains implementations.
 */

#ifndef UTI_RAND_H_
#define UTI_RAND_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

  extern int32_t rand1( void);
  extern void    srand1(uint32_t seed);
  extern int32_t randr( int32_t  min, int32_t max);

#ifdef __cplusplus
}
#endif

#endif /* UTI_RAND_H_ */
