#include "NwkSimul.hpp"
#include "defs.h"


const char *ClassNwkSimul::TYPE_NAME  = "Simulation";


ClassNwkSimul::ClassNwkSimul()
{
  // Do nothing
}

bool ClassNwkSimul::openSpecific()
{
  // Do nothing
  return true;
}

void ClassNwkSimul::closeSpecific()
{
  // Do nothing
}

void ClassNwkSimul::sleepSpecific()
{
  // Do nothing
}

void ClassNwkSimul::wakeupSpecific()
{
  // Do nothing
}


const char *ClassNwkSimul::typeName() const
{
  return TYPE_NAME;
}

uint32_t ClassNwkSimul::maxPayloadSize()
{
  return 255;
}


bool ClassNwkSimul::sendSpecific(const uint8_t *pu8_data,
				 uint16_t       size,
				 SendOptions    options)
{
  UNUSED(pu8_data);
  UNUSED(size);
  UNUSED(options);
  // Do nothing
  return true;
}

