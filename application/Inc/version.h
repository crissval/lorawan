/*
 * version.h
 *
 *  Created on: 17 avr. 2018
 *      Author: jfuchet
 */

#ifndef VERSION_H_
#define VERSION_H_

#include "version-rev.h"
#include "defs.h"


#ifndef VERSION_SUFFIX
#define VERSION_SUFFIX
#endif


#define VERSION_MAJOR     3
#define VERSION_MINOR     1
#define VERSION_REVISION  0

#define __VERSION_STR(major, minor, rev, suffix)  #major "." #minor "." #rev #suffix
#define _VERSION_STR( major, minor, rev, suffix)  __VERSION_STR(major, minor, rev, suffix)
#define VERSION_STR \
  _VERSION_STR(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_SUFFIX) \
  " (r" VERSION_REV_ID_STR ")"


#endif /* VERSION_H_ */
