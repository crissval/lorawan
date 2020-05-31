/*
 * Power and clocks management.
 *
 * @author Jérôme FUCHET
 * @date   2018
 */

#ifndef ENVIRONMENT_POWERANDCLOCKS_H_
#define ENVIRONMENT_POWERANDCLOCKS_H_

#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * The clock configuration identifiers.
   */
  typedef enum PwrClkClockConfigId
  {
    PWRCLK_CLOCK_CONFIG_RUN,  ///< Normal run mode, with USB.
    PWRCLK_CLOCK_CONFIG_COUNT,         ///< Not an actual identifier. Is after last valid id to count the number of identifiers.
    PWRCLK_CLOCK_CONFIG_NONE    = PWRCLK_CLOCK_CONFIG_COUNT,  ///< Id to indicate no id set.
    PWRCLK_CLOCK_CONFIG_DEFAULT = PWRCLK_CLOCK_CONFIG_RUN     ///< Default clock configuration
  }
  PwrClkClockConfigId;

  /**
   * Clock identifiers.
   */
  typedef enum PwrClkClockId
  {
    PWRCLK_CLOCK_SYSCLK,
    PWRCLK_CLOCK_HCLK,
    PWRCLK_CLOCK_PCLK1,
    PWRCLK_CLOCK_PCLK2,
    PWRCLK_CLOCK_LSE,
    PWRCLK_CLOCK_LSI,
    PWRCLK_CLOCK_MSI,
    PWRCLK_CLOCK_HSI16,
    PWRCLK_CLOCK_HSE,
    PWRCLK_CLOCK_SAI1,
#ifdef RCC_PLLSAI2_SUPPORT
    PWRCLK_CLOCK_SAI2,
#endif

    // Count the ids
    PWRCLK_CLOCKS_COUNT,   ///< Not an actual clock identifier; Is their count.

    // Aliases
    PWRCLK_CLOCK_AHB  = PWRCLK_CLOCK_HCLK,
    PWRCLK_CLOCK_APB1 = PWRCLK_CLOCK_PCLK1,
    PWRCLK_CLOCK_APB2 = PWRCLK_CLOCK_PCLK2
  }
  PwrClkClockId;

  /**
   * Defines the wakeup from sleep sources.
   */
  typedef enum PwrClkWakeupSource
  {
    PWRCLK_WAKEUP_ON_EVENT     = 0x00,
    PWRCLK_WAKEUP_ON_INTERRUPT = 0x01
#ifdef PWR_CR3_EWUP1
    ,PWRCLK_WAKEUP_ON_WKUP1    = (1u << 2) + PWRCLK_WAKEUP_ON_EVENT
#endif
#ifdef PWR_CR3_EWUP2
    ,PWRCLK_WAKEUP_ON_WKUP2    = (1u << 3) + PWRCLK_WAKEUP_ON_EVENT
#endif
#ifdef PWR_CR3_EWUP3
    ,PWRCLK_WAKEUP_ON_WKUP3    = (1u << 4) + PWRCLK_WAKEUP_ON_EVENT
#endif
#ifdef PWR_CR3_EWUP4
    ,PWRCLK_WAKEUP_ON_WKUP4    = (1u << 5) + PWRCLK_WAKEUP_ON_EVENT
#endif
#ifdef PWR_CR3_EWUP5
    ,PWRCLK_WAKEUP_ON_WKUP5    = (1u << 6) + PWRCLK_WAKEUP_ON_EVENT
#endif
  }
  PwrClkWakeupSource;

  /**
   * Lists power modes.
   * The higher the value the deepest the sleep.
   */
  typedef enum PwrClkPowerMode
  {
    PWRCLK_POWER_MODE_RUN,
    PWRCLK_POWER_MODE_LPRUN,
    PWRCLK_POWER_MODE_SLEEP,
    PWRCLK_POWER_MODE_LPSLEEP,
    PWRCLK_POWER_MODE_STOP0,
    PWRCLK_POWER_MODE_STOP1,
    PWRCLK_POWER_MODE_STOP2,
    PWRCLK_POWER_MODE_STANDBY,
    PWRCLK_POWER_MODE_SHUTDOWN
  }
  PwrClkPowerMode;

  /**
   * Used to store information about a listener for 'power mode change event'.
   */
  struct PwrClkPowerModeChangeListener;
  typedef struct PwrClkPowerModeChangeListener PwrClkPowerModeChangeListener;
  struct PwrClkPowerModeChangeListener
  {
    const char *ps_name;  ///< The listener's name. MUST be NOT NULL and NOT EMPTY.

    /**
     * Function called when the power mode has changed.
     *
     * The function pointer MUST be NOT NULL.
     *
     * @param[in] current_mode  the current power mode.
     * @param[in] previous_mode the previous power mode.
     */
    void (*pf_signal_power_mode_change)(PwrClkPowerMode current_mode, PwrClkPowerMode previous_mode);

    PwrClkPowerModeChangeListener *pv_next;  ///< Next listener in the list. NULL if last in the list.
  };


  extern void     pwrclk_init(void);
  extern void     pwrclk_switch_clock_config_to(PwrClkClockConfigId cfg_id);
  extern uint32_t pwrclk_clock_frequency_hz(    PwrClkClockId id);

  extern void pwrclk_sleep(void);
  extern void pwrclk_stop( void);

  extern void pwrclk_sleep_ms_max(uint32_t ms);

  extern void pwrclk_register_power_mode_change_listener(PwrClkPowerModeChangeListener *pv_listener);
  /**
   * Indicate if a power mode is at most a given low power mode.
   *
   * @param[in] pwmode  the power mode we are wondering about.
   * @param[in] ref     the reference, the highest low power mode to return true.
   *
   * @return true  if the power mode is lowest or equal to the reference.
   * @return false otherwise.
   */
#define pwrclk_is_power_mode_at_most(pwmode, ref)  ((pwmode) >= (ref))


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_POWERANDCLOCKS_H_ */
