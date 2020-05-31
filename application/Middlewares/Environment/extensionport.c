/**
 * The represent's the node extension port.
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include <string.h>
#include "extensionport.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the structure used to store pin names' aliases.
   */
  typedef struct ExtPortPinAlias
  {
    const char *ps_alias;  ///< The alias.
    const char *ps_name;   ///< The actual pin's name.
  }
  ExtPortPinAlias;


  /**
   * List all the extension pins and their characteristics.
   */
  static const ExtPortPin _extport_pins[EXTPORT_PIN_COUNT] =
  {
      {
	  "ANA1",      ADC_ANA1_GPIO, EXTPORT_PIN_FN_ANALOG_INPUT
      },
      {
	  "ANA2",      ADC_ANA2_GPIO, EXTPORT_PIN_FN_ANALOG_INPUT
      },
      {
	  "INT1_INT",  INT1_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "INT2_INT",  INT2_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "GPIO1",     EXT_GPIO1_GPIO, EXTPORT_PIN_FN_DGPIO
      },
      {
	  "GPIO2",     EXT_GPIO2_GPIO, EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_INT_EDGE_BOTH
      },
      {
	  "GPIO3",     EXT_GPIO3_GPIO, EXTPORT_PIN_FN_DGPIO
      },
      {
	  "GPIO4",     EXT_GPIO4_GPIO, EXTPORT_PIN_FN_DGPIO
      },
      {
	  "GPIO5",     EXT_GPIO5_GPIO, EXTPORT_PIN_FN_DGPIO
      },
      {
	  "SPI_CS",    EXT_SPI_CS_GPIO,
	  EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_INT_EDGE_BOTH | EXTPORT_PIN_FN_SPI
      },
      {
	  "SPI_MISO",  EXT_SPI_MISO_GPIO,
	  EXTPORT_PIN_FN_DGPIO         | EXTPORT_PIN_FN_INT_EDGE_BOTH | EXTPORT_PIN_FN_SPI |
	  EXTPORT_PIN_FN_TIMER_PWM_OUT | EXTPORT_PIN_FN_TIMER_CAPTURE | EXTPORT_PIN_FN_TIMER_OUT_COMPARE
      },
      {
	  "SPI_MOSI",  EXT_SPI_MOSI_GPIO,
	  EXTPORT_PIN_FN_DGPIO         | EXTPORT_PIN_FN_INT_EDGE_BOTH | EXTPORT_PIN_FN_SPI |
	  EXTPORT_PIN_FN_TIMER_PWM_OUT | EXTPORT_PIN_FN_TIMER_CAPTURE | EXTPORT_PIN_FN_TIMER_OUT_COMPARE
      },
      {
	  "SPI_CLK",   EXT_SPI_CLK_GPIO,
	  EXTPORT_PIN_FN_DGPIO         | EXTPORT_PIN_FN_SPI |
	  EXTPORT_PIN_FN_TIMER_PWM_OUT | EXTPORT_PIN_FN_TIMER_CAPTURE | EXTPORT_PIN_FN_TIMER_OUT_COMPARE
      },
      {
	  "SPI_INT",   SPI_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "I2C_SDA",   I2C_EXTERNAL_SDA_GPIO,  EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_I2C
      },
      {
	  "I2C_SCL",   I2C_EXTERNAL_SCL_GPIO,  EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_I2C
      },
      {
	  "I2C_INT",   I2C_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "OPTO1_INT", OPTO1_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "OPTO1_INT", OPTO1_INPUT_GPIO,
	  EXTPORT_PIN_FN_DIGITAL_INPUT | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "OPTO2_INT", OPTO2_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "OPTO2_INT", OPTO2_INPUT_GPIO,
	  EXTPORT_PIN_FN_DIGITAL_INPUT | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "USART_INT", USART_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      },
      {
	  "USART_TX",  EXT_USART_TX_GPIO, EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_USART
      },
      {
	  "USART_RX",  EXT_USART_RX_GPIO, EXTPORT_PIN_FN_DGPIO | EXTPORT_PIN_FN_USART
      },
      {
	  "SDI12_INT", SDI12_INTERRUPT_GPIO,
	  EXTPORT_PIN_FN_INT_LATCHED   | EXTPORT_PIN_FN_ACTIVATE_VSENS_EXT
      }
      // Do not count SDI12_DATA because it uses several µC pins.
  };

  /**
   * List of extension pins' aliases.
   */
  static const ExtPortPinAlias _extport_aliases[] =
  {
      { "SDI_INT",   "SDI12_INT" },
      { "OPTO_1",    "OPTO1_INT" },
      { "OPTO1",     "OPTO1_INT" },
      { "OPTO_2",    "OPTO2_INT" },
      { "OPTO2",     "OPTO2_INT" },
      { "INT_1",     "INT1_INT"  },
      { "INT1",      "INT1_INT"  },
      { "INT_2",     "INT2_INT"  },
      { "INT2",      "INT2_INT"  },
      { "EXT_GPIO1", "GPIO1"     },
      { "EXT_GPIO2", "GPIO2"     },
      { "EXT_GPIO3", "GPIO3"     },
      { "EXT_GPIO4", "GPIO4"     },
      { "EXT_GPIO5", "GPIO5"     },
      { NULL, NULL }  // End of list
  };


  static const char *export_resolve_alias(const char *ps_name);


  /**
   * Return the internal name for a given alias.
   *
   * @note the search is not case sensitive.
   *
   * @param[in] ps_name the alias name.
   *
   * @return the internal name if ps_name is an actual alias.
   * @return ps_name if no matching alias has been found.
   */
  static const char *export_resolve_alias(const char *ps_name)
  {
    const ExtPortPinAlias *pv_alias;

    if(ps_name && *ps_name)
    {
      for(pv_alias = _extport_aliases; pv_alias->ps_alias; pv_alias++)
      {
	if(strcasecmp(ps_name, pv_alias->ps_alias) == 0) { return pv_alias->ps_name; }
      }
    }

    return ps_name;
  }


  /**
   * Get the informations for an extension port pin using the pin's name
   * and the functions we need.
   *
   * @note the search is not case sensitive.
   *
   * @param[in] ps_name   the extension pin's name. Can be an alias.
   * @param[in] functions a ORed combination of the functions we need the port to be able to do.
   *                      If several functions are set then the port need to be able to fulfill
   *                      all of them.
   *
   * @return the extension pin's informations.
   * @return NULL is ps_name is NULL or empty.
   * @return NULL if no extension pin informations have been found for the given name.
   * @return NULL if no extension pin informations have been found that fulfills all the
   *              function requirements.
   */
  const ExtPortPin *extport_get_pininfos_with_functions(const char         *ps_name,
							ExtPortPinFunctions functions)
  {
    uint8_t           i;
    const ExtPortPin *pv_pinfos;

    ps_name = export_resolve_alias(ps_name);

    for(i = 0, pv_pinfos = _extport_pins; i < EXTPORT_PIN_COUNT; i++, pv_pinfos++)
    {
      if(strcasecmp(ps_name, pv_pinfos->ps_name) == 0)
      {
	if((pv_pinfos->functions & functions) == functions) { return pv_pinfos; }
      }
    }

    return NULL;
  }


#ifdef __cplusplus
}
#endif
