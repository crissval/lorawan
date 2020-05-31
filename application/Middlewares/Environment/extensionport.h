/**
 * The represent's the node extension port.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#ifndef ENVIRONMENT_EXTENSIONPORT_H_
#define ENVIRONMENT_EXTENSIONPORT_H_

#include "defs.h"
#include "board.h"
#include "gpio.h"


#ifdef __cplusplus
extern "C" {
#endif


#define EXTPORT_PIN_COUNT  25  // Do not count SDI12_DATA because it uses several µC pins.


  /**
   * Defines the functions that can have a pin from the extension connector.
   */
  typedef enum ExtPortPinFunctionFlag
  {
    EXTPORT_PIN_FN_NONE               = 0,
    EXTPORT_PIN_FN_DIGITAL_INPUT      = 1u << 1,
    EXTPORT_PIN_FN_DIGITAL_OUTPUT     = 1u << 2,
    EXTPORT_PIN_FN_DGPIO              = EXTPORT_PIN_FN_DIGITAL_INPUT   | EXTPORT_PIN_FN_DIGITAL_OUTPUT,
    EXTPORT_PIN_FN_ANALOG_INPUT       = 1u << 3,
    EXTPORT_PIN_FN_ANALOG_OUTPUT      = 1u << 4,
    EXTPORT_PIN_FN_INT_LATCHED        = 1u << 5,
    EXTPORT_PIN_FN_INT_EDGE_RISING    = 1u << 6,
    EXTPORT_PIN_FN_INT_EDGE_FALLING   = 1u << 7,
    EXTPORT_PIN_FN_INT_EDGE_BOTH      = EXTPORT_PIN_FN_INT_EDGE_RISING | EXTPORT_PIN_FN_INT_EDGE_FALLING,
    EXTPORT_PIN_FN_SPI                = 1u << 8,
    EXTPORT_PIN_FN_USART              = 1u << 9,
    EXTPORT_PIN_FN_I2C                = 1u << 10,
    EXTPORT_PIN_FN_TIMER              = 1u << 11,
    EXTPORT_PIN_FN_TIMER_PWM_OUT      = 1u << 12 | EXTPORT_PIN_FN_TIMER,
    EXTPORT_PIN_FN_TIMER_CAPTURE      = 1u << 13 | EXTPORT_PIN_FN_TIMER,
    EXTPORT_PIN_FN_TIMER_OUT_COMPARE  = 1u << 14 | EXTPORT_PIN_FN_TIMER,
    EXTPORT_PIN_FN_TIMER_CLK_IN       = 1u << 15 | EXTPORT_PIN_FN_TIMER,

    EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT = 1u << 31  ///< Active VSens_Extern power supply (interruptions power supply)
  }
  ExtPortPinFunctionFlag;
  typedef uint32_t ExtPortPinFunctions;


  /**
   * This represents a pin from the extension port.
   */
  typedef struct ExtPortPin
  {
    const char         *ps_name;   ///< The pin's name.
    GPIOId              gpio;      ///< The GPIO.
    ExtPortPinFunctions functions; ///< List what this extension pin can do.
  }
  ExtPortPin;


  extern const ExtPortPin *extport_get_pininfos_with_functions(const char         *ps_name,
							       ExtPortPinFunctions functions);


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_EXTENSIONPORT_H_ */
