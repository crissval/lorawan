/*
 * Interface that connecSenS interruption clients must implement
 *
 *  Created on: 23 mai 2018
 *      Author: jfuchet
 */
#ifndef ENVIRONMENT_CNSSINTCLIENT_HPP_
#define ENVIRONMENT_CNSSINTCLIENT_HPP_

#include "defs.h"
#include "cnssint.hpp"


class CNSSIntClient
{

public:
  virtual void processInterruption(CNSSInt::Interruptions ints) = 0;
};



#endif /* ENVIRONMENT_CNSSINTCLIENT_HPP_ */
