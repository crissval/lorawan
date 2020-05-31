/**
 * @file  board.c
 * @brief Board implementation file.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include "gpio.h"
#include "board.h"
#include "logger.h"
#include "cnssint_c_connector.h"
#include "rtc.h"
#include "sdcard.h"

#ifdef __cplusplus
extern "C" {
#endif


  CREATE_LOGGER(board);
#undef  logger
#define logger  board


  /**
   * Options are:
   *
   * + BOR Level 1, threshold around 2.0 V
   * + No reset when entering Stop mode
   * + No reset when entering the Standby mode
   * + No reset when entering the shutdown mode
   * + Software independent watchdog
   * + Freeze independent watchdog in stop mode
   * + Freeze independent watchdog in standby mode
   * + Software window watchdog
   * + Disable dual-bank boot
   * + Dual-bank Flash
   * + System boot
   * + SRAM2 parity check disabled
   * + Do not erase SRAM2 on reset
   */
#define USER_OPTION_BYTES_MASK (\
  FLASH_OPTR_BOR_LEV_Msk    | \
  FLASH_OPTR_nRST_STOP_Msk  | \
  FLASH_OPTR_nRST_STDBY_Msk | \
  FLASH_OPTR_nRST_SHDW_Msk  | \
  FLASH_OPTR_IWDG_SW_Msk    | \
  FLASH_OPTR_IWDG_STOP_Msk  | \
  FLASH_OPTR_IWDG_STDBY_Msk | \
  FLASH_OPTR_WWDG_SW_Msk    | \
  FLASH_OPTR_BFB2_Msk       | \
  FLASH_OPTR_DUALBANK_Msk   | \
  FLASH_OPTR_nBOOT1_Msk     | \
  FLASH_OPTR_SRAM2_PE_Msk   | \
  FLASH_OPTR_SRAM2_RST_Msk)
#define USER_OPTION_BYTES \
  (OB_BOR_LEVEL_1         | \
  OB_STOP_NORST           | \
  OB_STANDBY_NORST        | \
  OB_SHUTDOWN_NORST       | \
  OB_IWDG_SW              | \
  OB_IWDG_STOP_FREEZE     | \
  OB_IWDG_STDBY_FREEZE    | \
  OB_WWDG_SW              | \
  OB_BFB2_DISABLE         | \
  OB_DUALBANK_DUAL        | \
  OB_BOOT1_SYSTEM         | \
  OB_SRAM2_PARITY_DISABLE | \
  OB_SRAM2_RST_NOT_ERASE)
#define USER_OPTION_BYTES_IDS \
  (OB_USER_BOR_LEV   | \
  OB_USER_nRST_STOP  | \
  OB_USER_nRST_STDBY | \
  OB_USER_nRST_SHDW  | \
  OB_USER_IWDG_SW    | \
  OB_USER_IWDG_STOP  | \
  OB_USER_IWDG_STDBY | \
  OB_USER_WWDG_SW    | \
  OB_USER_BFB2       | \
  OB_USER_DUALBANK   | \
  OB_USER_nBOOT1     | \
  OB_USER_SRAM2_PE   | \
  OB_USER_SRAM2_RST)


  // Watchdog
  IWDG_HandleTypeDef hiwdg;

  /**
   * Store informations about a power supply.
   */
  typedef struct PowerInfo
  {
    const char    *ps_name;              ///< The power supply's name.
    BoardPowerFlag id_flag;              ///< The power supply's identifier.
    GPIOId         state_gpio_id;        ///< Id of the GPIO used to query the power supply's state.
    GPIOId         enable_gpio_id;       ///< Id of the GPIO used to enable or the power supply.
    uint32_t       turned_on_settle_ms;  ///< Time to wait, in milliseconds, for power to settle down once it has been turned on.
    void         (*pf_on_on)(void);      ///< Function to call when the power is turned on. Can be NULL.
  }
  PowerInfo;

#define BOARD_POWER_INFOS_COUNT  3
  static const PowerInfo _board_power_infos[BOARD_POWER_INFOS_COUNT] =
  {
      {
	  "internal sensors",
	  BOARD_POWER_INTERNAL_SENSORS,
	  STATE_PWR_INTERNAL_GPIO,
	  ENABLE_PWR_INTERNAL_GPIO,
	  10,
	  cnssint_clear_internal_interruptions
      },
      {
	  "external sensors",
	  BOARD_POWER_EXT_ADJ,
	  STATE_PWR_EXTERNAL_GPIO,
	  ENABLE_PWR_EXTERNAL_GPIO,
	  1000,
	  NULL
      },
      {
	  "external interruptions",
	  BOARD_POWER_EXT_INT,
	  STATE_PWR_EXTINTERRUPT_GPIO,
	  ENABLE_PWR_EXTINTERRUPT_GPIO,
	  0,
	  cnssint_clear_external_interruptions
      }
  };


  static void board_init_option_bytes(void);
  static void board_init_watchdog(void);
  static void board_enable_power(bool enable, const PowerInfo *pv_info);


  /**
   * Initialises all the boards features.
   */
  void board_init(void)
  {
    board_init_watchdog();
    board_init_option_bytes();
  }


  /**
   * Initialisation, if necessary, of the STM32 option bytes.
   */
  static void board_init_option_bytes(void)
  {
    FLASH_OBProgramInitTypeDef ob_init;
    bool                       lock_flash;

    // Get current option byte configuration
    ob_init.WRPArea     = 0;  // Do not get write protection area information
    ob_init.PCROPConfig = 0;  // Do not get flash banks informations
    HAL_FLASHEx_OBGetConfig(&ob_init);

    // See if current user option bytes value is the expected one.
    // And if not, then change them to the value we want.
    if((ob_init.USERConfig & USER_OPTION_BYTES_MASK) != USER_OPTION_BYTES)
    {
      // Write option user bytes
      ob_init.OptionType = OPTIONBYTE_USER;
      ob_init.USERConfig = USER_OPTION_BYTES;
      ob_init.USERType   = USER_OPTION_BYTES_IDS;
      lock_flash         = (HAL_FLASH_Unlock() == HAL_OK);
      HAL_FLASH_OB_Unlock();
      HAL_FLASHEx_OBProgram(&ob_init);
      FLASH->CR |= FLASH_CR_OBL_LAUNCH;  // This should generate a system reset
      HAL_FLASH_OB_Lock();
      if(lock_flash) { HAL_FLASH_Lock(); }
    }
  }




  /**
   * Initialise and start the watchdog
   */
  static void board_init_watchdog(void)
  {
#ifndef DISABLE_WATCHDOG
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Window    = 4095;
    hiwdg.Init.Reload    = 4095;
    HAL_IWDG_Init(&hiwdg);
#endif
  }

  /**
   * Reset the watchdog.
   */
  void board_watchdog_reset(void)
  {
    HAL_IWDG_Refresh(&hiwdg);
  }


  /**
   * Set the controllable power supplies configuration.
   *
   * @param[in] power the configuration to set.
   */
  void board_set_power(BoardPower power)
  {
    const PowerInfo *pv_info;
    bool             enable;
    uint8_t          i;

    for(i = 0; i < BOARD_POWER_INFOS_COUNT; i++)
    {
      pv_info  = &_board_power_infos[i];
      enable   = ((power & pv_info->id_flag) != 0);
      board_enable_power(enable, pv_info);
    }
  }

  /**
   * Turn additional power supplies ON if there are not already.
   */
  void board_add_power(BoardPower power)
  {
    const PowerInfo *pv_info;
    uint8_t          i;

    for(i = 0; i < BOARD_POWER_INFOS_COUNT; i++)
    {
      pv_info  = &_board_power_infos[i];
      if(power & pv_info->id_flag)
      {
	board_enable_power(true, pv_info);
      }
    }
  }

  /**
   * Enable or disable a power supply.
   *
   * @param[in] enable  turn on (true) or off (false) the power supply.
   * @param[in] pv_info the power supply's informations. MUST be NOT NULL.
   */
  static void board_enable_power(bool enable, const PowerInfo *pv_info)
  {
    GPIO_TypeDef    *pv_state_port, *pv_enable_port;
    GPIO_InitTypeDef init;
    uint32_t         state_pin, enable_pin;
    bool             state;
    char             *ps_state = enable ? "ON" : "OFF";

    pv_state_port  = gpio_hal_port_and_pin_from_id( pv_info->state_gpio_id,  &state_pin);
    pv_enable_port = gpio_hal_port_and_pin_from_id( pv_info->enable_gpio_id, &enable_pin);
    gpio_use_gpios_with_ids(pv_info->state_gpio_id, pv_info->enable_gpio_id);

    // Enable and configure GPIOs
    // Power state pin
    init.Alternate = 0;
    init.Mode	   = GPIO_MODE_INPUT;
    init.Pull 	   = GPIO_NOPULL;
    init.Speed	   = GPIO_SPEED_FREQ_HIGH;
    init.Pin       = state_pin;
    HAL_GPIO_Init(pv_state_port, &init);
    // Power enable pins
    init.Mode	   = GPIO_MODE_OUTPUT_PP;
    init.Pin       = enable_pin;
    HAL_GPIO_Init(pv_enable_port, &init);

    state = (HAL_GPIO_ReadPin(pv_state_port, state_pin) == GPIO_PIN_SET);
    if(state != enable)
    {
      log_debug(logger, "Turning %s's power supply %s...", pv_info->ps_name, ps_state);
    }
    else
    {
      log_debug(logger, "Power to %s already is %s.", pv_info->ps_name, ps_state);
      goto exit;
    }

    // Turn ON or OFF
    if(enable)
    {
      while(HAL_GPIO_ReadPin(pv_state_port, state_pin) != GPIO_PIN_SET)
      {
        HAL_GPIO_WritePin(pv_enable_port, enable_pin, GPIO_PIN_SET);
        board_delay_ms(1);
        HAL_GPIO_WritePin(pv_enable_port, enable_pin, GPIO_PIN_RESET);
        board_delay_ms(1);
      }
      if(pv_info->pf_on_on)            { pv_info->pf_on_on(); }
      if(pv_info->turned_on_settle_ms) { board_delay_ms(pv_info->turned_on_settle_ms); }
    }
    else
    {
      while(HAL_GPIO_ReadPin(pv_state_port, state_pin) != GPIO_PIN_RESET)
      {
        HAL_GPIO_WritePin(pv_enable_port, enable_pin, GPIO_PIN_SET);
        board_delay_ms(1);
        HAL_GPIO_WritePin(pv_enable_port, enable_pin, GPIO_PIN_RESET);
        board_delay_ms(1);
      }
    }
    log_info(logger, "Power to %s: %s.", pv_info->ps_name, ps_state);

    exit:
    // De-init and disable GPIO, the hardware flip-flop will keep the power state
    gpio_free_gpios_with_ids(pv_info->state_gpio_id, pv_info->enable_gpio_id);
  }


  /**
   * Request a software reset of the node.
   *
   * @param[in] type The type of software request.
   */
  void board_reset(BoardSoftwareResetType type)
  {
    RTCSystemStatusValue t = RTC_SYSTEM_STATUS_SOFTWARE_RESET_NONE;

    // Set up the software reset type in the RTC registers
    switch(type)
    {
      case BOARD_SOFTWARE_RESET_OK: t = RTC_SYSTEM_STATUS_SOFTWARE_RESET_OK; break;
      case BOARD_SOFTWARE_RESET_ERROR:
      default:
	t = RTC_SYSTEM_STATUS_SOFTWARE_RESET_ERROR;
	break;
    }
    rtc_software_reset_set_type(t);

    // Reset
    sdcard_deinit();
    NVIC_SystemReset();
  }

  /**
   * Get the reset flags that indicate the cause of the last reset.
   */
  BoardResetFlags board_reset_source_flags(bool clear)
  {
    RTCSystemStatusValue soft_reset_type;
    BoardResetFlags      res   = BOARD_RESET_FLAG_NONE;
    uint32_t             flags = RCC->CSR;

    if(flags & RCC_CSR_LPWRRSTF) { res |= BOARD_RESET_FLAG_LOW_PWR_ERROR;        }
    if(flags & RCC_CSR_WWDGRSTF) { res |= BOARD_RESET_FLAG_WATCHDOG;             }
    if(flags & RCC_CSR_IWDGRSTF) { res |= BOARD_RESET_FLAG_INDEPENDENT_WATCHDOG; }
    if(flags & RCC_CSR_BORRSTF)  { res |= BOARD_RESET_FLAG_BOR;                  }
    if(flags & RCC_CSR_OBLRSTF)  { res |= BOARD_RESET_FLAG_OPTION_BYTES_LOADING; }
    if(flags & RCC_CSR_FWRSTF)   { res |= BOARD_RESET_FLAG_FIREWALL;             }
    // Pin and software reset are not tested because they are always generated.
    //if(flags & RCC_CSR_PINRSTF)  { res |= BOARD_RESET_FLAG_PIN;                  }
    //if(flags & RCC_CSR_SFTRSTF)  { res |= BOARD_RESET_FLAG_SOFTWARE;             }

    // Look in RTC for software reset requests
    soft_reset_type = rtc_software_reset_type(clear);
    switch(soft_reset_type)
    {
      case RTC_SYSTEM_STATUS_SOFTWARE_RESET_OK:
	res |= BOARD_RESET_FLAG_SOFTWARE;
	break;

      case RTC_SYSTEM_STATUS_SOFTWARE_RESET_ERROR:
	res |= BOARD_RESET_FLAG_SOFTWARE_ERROR;
	break;

      default: break;  // Do nothing
    }

    if(clear) { board_reset_source_clear(); }

    return res;
  }

  /**
   * Clear the reset source flags.
   */
  void board_reset_source_clear(void)
  {
    RCC->CSR |= RCC_CSR_RMVF;
  }



#ifdef __cplusplus
}
#endif
