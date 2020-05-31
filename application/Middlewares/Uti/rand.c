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

#include "rand.h"

#ifdef __cplusplus
extern "C" {
#endif


  // Standard random functions redefinition start
#define RAND_LOCAL_MAX  2147483647L

  static uint32_t _rand_next = 1;

  int32_t rand1(void)
  {
    return ((_rand_next = _rand_next * 1103515245L + 12345L) % RAND_LOCAL_MAX);
  }

  void srand1(uint32_t seed)
  {
    _rand_next = seed;
  }
  // Standard random functions redefinition end

  int32_t randr(int32_t min, int32_t max)
  {
    return (int32_t)rand1() % (max - min + 1) + min;
  }


#ifdef __cplusplus
}
#endif

