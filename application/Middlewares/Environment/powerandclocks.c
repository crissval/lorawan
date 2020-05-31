/*
 * Power and clocks management.
 *
 * @author Jérôme FUCHET
 * @date   2018
 */

#include "powerandclocks.h"
#include "logger.h"
#include "timer.h"


#ifdef __cplusplus
extern "C" {
#endif


  CREATE_LOGGER(pwrclk);
#define logger  pwrclk


  /* The SystemCoreClock variable is updated in three ways:
        1) by calling CMSIS function SystemCoreClockUpdate()
        2) by calling HAL API function HAL_RCC_GetHCLKFreq()
        3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
           Note: If you use this function to configure the system clock; then there
                 is no need to call the 2 first functions listed above, since SystemCoreClock
                 variable is updated automatically.
   */
  uint32_t SystemCoreClock = 4000000;

  /**
   * These definitions are required by HAL
   */
  const uint8_t  AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
  const uint8_t  APBPrescTable[8]  = { 0, 0, 0, 0, 1, 2, 3, 4 };
  const uint32_t MSIRangeTable[12] =
  {
      100000,  200000,  400000,   800000,   1000000,  2000000,
      4000000, 8000000, 16000000, 24000000, 32000000, 48000000
  };

  static struct
  {
    RCC_OscInitTypeDef       osc_init;
    RCC_ClkInitTypeDef       clk_init;
    RCC_PeriphCLKInitTypeDef periph_clk_init;
    bool                     has_been_initialised;
  }
  _pwrclk_run_cfg;

  /// Points to the list of power mode change event listeners.
  static PwrClkPowerModeChangeListener *_pwrclk_power_mode_change_listeners = NULL;



  /**
   * Type used to store a clock configuration.
   */
  typedef struct PwrClkClockConfig
  {
    PwrClkClockConfigId id;        ///< The identifier for this configuration.
    const char          *ps_name;  ///< The configuration's name.

    /**
     * Contains the clocks frequencies.
     * The table is indexed using the clock identifiers (PwrClkClockId type).
     */
    uint32_t clock_frequencies_hz[PWRCLK_CLOCKS_COUNT];

    void (*pf_set_clocks)(void);  ///< Function used to configure clocks.
  }
  PwrClkClockConfig;


  static void pwrclk_set_run_config(void);
  static void pwrclk_signal_power_mode_change(PwrClkPowerMode current_mode,
					      PwrClkPowerMode previous_mode);

  /**
   * Contains the clock configurations.
   * Indexed by clock configuration id (PwrClkClockConfigId type).
   */
  static const PwrClkClockConfig _pwrclk_clock_configs[PWRCLK_CLOCK_CONFIG_COUNT] =
  {
      {
	  PWRCLK_CLOCK_CONFIG_RUN,  // id
	  "Run",                    // ps_name
	  {
	      48000000,     // PWRCLK_CLOCK_SYSCLK
	      48000000,     // PWRCLK_CLOCK_HCLK
	      12000000,     // PWRCLK_CLOCK_PCLK1
	      12000000,     // PWRCLK_CLOCK_PCLK2
	      LSE_FREQ_HZ,  // PWRCLK_CLOCK_LSE
	      0,            // PWRCLK_CLOCK_LSI
	      48000000,     // PWRCLK_CLOCK_MSI
	      0,            // PWRCLK_CLOCK_HSI16
	      0,            // PWRCLK_CLOCK_HSE
	      0,            // PWRCLK_CLOCK_SAI1
#ifdef RCC_PLLSAI2_SUPPORT
	      0,            // PWRCLK_CLOCK_SAI2
#endif
	  },
	  pwrclk_set_run_config  // pf_set_clocks
      }
  };


  static const PwrClkClockConfig *_pv_pwrclk_current_clock_config = NULL;



  /**
   * Power and clock initialisation.
   */
  void pwrclk_init(void)
  {
#ifdef LSE_DRIVE_LEVEL
    // Set up RTC clock
    RCC_OscInitTypeDef       osc_init;
    RCC_PeriphCLKInitTypeDef periph_clk_init;

    __HAL_RCC_LSEDRIVE_CONFIG(LSE_DRIVE_LEVEL);

    osc_init.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    osc_init.LSEState       = RCC_LSE_ON;

    periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periph_clk_init.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;

    if( HAL_RCC_OscConfig(        &osc_init)        != HAL_OK ||
	HAL_RCCEx_PeriphCLKConfig(&periph_clk_init) != HAL_OK)
    {
      log_fatal(logger, "Failed to set LSE clock configuration");
    }

#ifdef RTC_OUTPUT_CLK_ON_MCO
 HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_LSE, RCC_MCODIV_1);
#endif
#else
#error "LSE_DRIVE_LEVEL is not set. Cannot configure RTC."
#endif

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddIO2();

    pwrclk_switch_clock_config_to(PWRCLK_CLOCK_CONFIG_DEFAULT);
  }


  /**
   * Switch the clock configuration.
   *
   * @param[in] cfg_id the identifier of the clock configuration to set up.
   */
  void pwrclk_switch_clock_config_to(PwrClkClockConfigId cfg_id)
  {
    const PwrClkClockConfig *pv_config;

    // Check identifier
    if(cfg_id < 0 || cfg_id >= PWRCLK_CLOCK_CONFIG_COUNT)
    {
      log_fatal(logger, "Invalid clock config id: %d", cfg_id);
    }

    // Get the configuration to set up.
    pv_config = &_pwrclk_clock_configs[cfg_id];

    // If new configuration is the same as the current one then do nothing.
    if(pv_config == _pv_pwrclk_current_clock_config) { goto exit; }

    // Set up new configuration
    pv_config->pf_set_clocks();
    _pv_pwrclk_current_clock_config = pv_config;

    // For HAL library
    SystemCoreClock = pv_config->clock_frequencies_hz[PWRCLK_CLOCK_HCLK];

    exit:
    return;
  }

  /**
   * Return the clock frequency, in Hz, of a given clock.
   *
   * @param[in] id the clock identifier. MUST be valid.
   */
  uint32_t pwrclk_clock_frequency_hz(PwrClkClockId id)
  {
    return _pv_pwrclk_current_clock_config->clock_frequencies_hz[id];
  }


  /**
   * Set up the default Run configuration.
   */
  static void pwrclk_set_run_config(void)
  {
    if(!_pwrclk_run_cfg.has_been_initialised)
    {
      _pwrclk_run_cfg.osc_init.OscillatorType      = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_HSI;
      _pwrclk_run_cfg.osc_init.MSIState            = RCC_MSI_ON;
      _pwrclk_run_cfg.osc_init.MSICalibrationValue = 0;
      _pwrclk_run_cfg.osc_init.MSIClockRange       = RCC_MSIRANGE_11;  // 48 MHz
      _pwrclk_run_cfg.osc_init.PLL.PLLState        = RCC_PLL_NONE;
      _pwrclk_run_cfg.osc_init.HSIState            = RCC_HSI_ON;
      _pwrclk_run_cfg.osc_init.HSICalibrationValue = 0;

      _pwrclk_run_cfg.clk_init.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
	  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
      _pwrclk_run_cfg.clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
      _pwrclk_run_cfg.clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1;
      _pwrclk_run_cfg.clk_init.APB1CLKDivider = RCC_HCLK_DIV4;
      _pwrclk_run_cfg.clk_init.APB2CLKDivider = RCC_HCLK_DIV4;

      // USART sets their onwn clock source.
      // Except for USART3 that is not part of the USART driver yet.
      _pwrclk_run_cfg.periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_USART3 |
	  RCC_PERIPHCLK_I2C1   |
	  RCC_PERIPHCLK_I2C2   |
	  RCC_PERIPHCLK_SDMMC1 |
	  RCC_PERIPHCLK_ADC;
      _pwrclk_run_cfg.periph_clk_init.Usart3ClockSelection = RCC_USART3CLKSOURCE_HSI;
      _pwrclk_run_cfg.periph_clk_init.I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1;
      _pwrclk_run_cfg.periph_clk_init.I2c2ClockSelection   = RCC_I2C2CLKSOURCE_PCLK1;
      _pwrclk_run_cfg.periph_clk_init.AdcClockSelection    = RCC_ADCCLKSOURCE_SYSCLK;
      _pwrclk_run_cfg.periph_clk_init.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_MSI;

    _pwrclk_run_cfg.has_been_initialised = true;
    }

    if( HAL_RCC_OscConfig(        &_pwrclk_run_cfg.osc_init)                  != HAL_OK ||
	HAL_RCC_ClockConfig(      &_pwrclk_run_cfg.clk_init, FLASH_LATENCY_2) != HAL_OK ||
	HAL_RCCEx_PeriphCLKConfig(&_pwrclk_run_cfg.periph_clk_init)           != HAL_OK)
    {
      log_fatal(logger, "Failed to set Run clock configuration.");
    }

    if(HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
      log_fatal(logger, "Failed to set Run voltage regulator configuration.");
    }
  }


  /**
   * Go to Sleep low power mode.
   *
   * Only an interruption can wake us up. Events do not.
   *
   * @post When we get out of this function then we are in Run mode,
   *       with the power and clock configuration used when entering the function.
   */
  void pwrclk_sleep(void)
  {
    pwrclk_signal_power_mode_change(PWRCLK_POWER_MODE_SLEEP, PWRCLK_POWER_MODE_RUN);

    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    pwrclk_signal_power_mode_change(PWRCLK_POWER_MODE_RUN, PWRCLK_POWER_MODE_SLEEP);
  }


  /**
   * Go to Sleep low power mode for a given at most number of milliseconds.
   *
   * Only an interruption can wake us up. Events do not.
   *
   * @post When we get out of this function then we are in Run mode,
   *       with the power and clock configuration used when entering the function.
   *
   * @param[in] ms the maximum number of milliseconds to sleep.
   */
  void pwrclk_sleep_ms_max(uint32_t ms)
  {
    Timer timer;

    timer_init( &timer, ms, TIMER_TU_MSECS | TIMER_SINGLE_SHOT | TIMER_RELATIVE);
    timer_start(&timer);

    // Go to Sleep low power mode.
    pwrclk_signal_power_mode_change(PWRCLK_POWER_MODE_SLEEP, PWRCLK_POWER_MODE_RUN);
    if(!timer_has_timed_out(&timer))
    {
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    }
    pwrclk_signal_power_mode_change(PWRCLK_POWER_MODE_RUN, PWRCLK_POWER_MODE_SLEEP);

    // We have been woken up by our timer or by any other interruption or event.
    // Stop our timer.
    timer_stop(&timer);
  }


  /**
   * Go to the Stop 2 low power mode.
   *
   * Only an interruption can wake us up. Events do not.
   *
   * @post When we get out of this function then we are in the default Run mode.
   */
  void pwrclk_stop(void)
  {
    pwrclk_signal_power_mode_change(PWRCLK_POWER_MODE_STOP1, PWRCLK_POWER_MODE_RUN);

    HAL_PWREx_EnterSTOP1Mode(PWR_SLEEPENTRY_WFI);

    // Set up default configuration
    pwrclk_switch_clock_config_to(PWRCLK_CLOCK_CONFIG_DEFAULT);

    pwrclk_signal_power_mode_change(PWRCLK_CLOCK_CONFIG_DEFAULT, PWRCLK_POWER_MODE_STOP1);
  }


  /**
   * Register a listener for power mode change events.
   *
   * @param[in] pv_listener the listener to add. MUST be NOT NULL.
   *                        MUST not have already been registered.
   */
  void pwrclk_register_power_mode_change_listener(PwrClkPowerModeChangeListener *pv_listener)
  {
    pv_listener->pv_next                = _pwrclk_power_mode_change_listeners;
    _pwrclk_power_mode_change_listeners = pv_listener;
  }

  /**
   * Signal to listeners that a power mode change event has just happened.
   *
   * @param[in] current_mode  the current power mode.
   * @param[in] previous_mode the previous power mode.
   */
  static void pwrclk_signal_power_mode_change(PwrClkPowerMode current_mode,
  					      PwrClkPowerMode previous_mode)
  {
    PwrClkPowerModeChangeListener *pv_listener;

    for(pv_listener = _pwrclk_power_mode_change_listeners;
	pv_listener;
	pv_listener = pv_listener->pv_next)
    {
      pv_listener->pf_signal_power_mode_change(current_mode, previous_mode);
    }
  }


#ifdef __cplusplus
}
#endif


