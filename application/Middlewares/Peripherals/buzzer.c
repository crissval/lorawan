/**
 * Function to pilot a buzzer
 *
 * @author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date:   2018
 */

#include "buzzer.h"
#include "extensionport.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Define the buezzer's configuration options.
   */
  typedef enum BuzzerConfigFlag
  {
    BUZZER_CFG_NONE   = 0,
    BUZZER_CFG_ON_OFF = 1u << 1  // The buzzer is an ON/OFF model.
  }
  BuzzerConfigFlag;
  typedef uint8_t BuzzerConfig;  ///< Stores a buzzer's configuration.
#define buzzer_is_on_off_model(pv_buzzer)  ((pv_buzzer)->config & BUZZER_CFG_ON_OFF)


  /**
   * Defines a buzzer
   */
  typedef struct Buzzer
  {
    const ExtPortPin *pv_pinfos;   ///< The informations about the pin used to control the buzzer.
    BuzzerConfig      config;      ///< The buzzer's configuration.
    bool              is_enabled;  ///< Is the buzzer enabled?
    bool              is_on;       ///< Is the buzzer turned on?
  }
  Buzzer;


  static Buzzer _buzzer;  ///< The buzzer object


  /**
   * Enable or disable the buzzer.
   *
   * @param[in] enable enable (true) or disable(false)?
   */
  void buzzer_set_enable(bool enable)
  {
    _buzzer.is_enabled = enable;
    if(!enable) { buzzer_turn_off(); }
  }


  /**
   * Configure to use a buzzer that can only be turned on or off.
   *
   * @param[in] ps_pin_name the name of the pin, on the extension connector,
   *                        that controls the buzzer.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool buzzer_configure_on_off(const char *ps_pin_name)
  {
    if(!(_buzzer.pv_pinfos = extport_get_pininfos_with_functions(ps_pin_name,
								 EXTPORT_PIN_FN_DIGITAL_OUTPUT)))
    {
      return false;
    }

    _buzzer.config = BUZZER_CFG_ON_OFF;
    buzzer_turn_off();

    return true;
  }

  /**
   * Turn an ON/OFF buzzer ON or OFF.
   *
   * @param[in] on turn the buzzer ON (true) or OFF (false)?
   */
  void buzzer_set_turn_on(bool on)
  {
    GPIO_InitTypeDef init;
    GPIO_TypeDef    *pv_port;

    if(!_buzzer.pv_pinfos || !buzzer_is_on_off_model(&_buzzer)) { goto exit; }
    if(!_buzzer.is_enabled ) { on = false; }

    // Configure control pin.
    if(on)
    {
      gpio_use_gpio_with_id(_buzzer.pv_pinfos->gpio);
      pv_port        = gpio_hal_port_and_pin_from_id(_buzzer.pv_pinfos->gpio, &init.Pin);
      init.Alternate = 0;
      init.Mode      = GPIO_MODE_OUTPUT_PP;
      init.Pull      = GPIO_NOPULL;
      init.Speed     = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(    pv_port, &init);
      HAL_GPIO_WritePin(pv_port, init.Pin, GPIO_PIN_SET);
    }
    else
    {
      // Set GPIO to analog mode; which turns the buzzer OFF.
      gpio_free_gpio_with_id(_buzzer.pv_pinfos->gpio);
      HAL_GPIO_DeInit(gpio_hal_port_and_pin_from_id(_buzzer.pv_pinfos->gpio, &init.Pin), init.Pin);
    }

    _buzzer.is_on = on;

    exit:
    return;
  }

  /**
   * Toggle a ON/OFF buzzer.
   */
  extern void buzzer_toggle()
  {
    buzzer_set_turn_on(!_buzzer.is_on);
  }


#ifdef __cplusplus
}
#endif
