

#include "board.h"
#include "powerandclocks.h"
#include "rtc.h"
#include "connecsens.hpp"
#include "logger.h"


#define VECT_TAB_OFFSET  0x20000  // Take into account bootloader


//CREATE_LOGGER(main);
//#undef  _logger
//#define _logger  main


static ConnecSenS _env;


int main(void)
{
  bool     res, res_previous;
  uint32_t refMs;

  HAL_Init();
  pwrclk_init();
  rtc_init();

  // De-activate systick
  SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);

  // Use JTAG GPIO pins
  gpio_use_gpios_with_ids(JTAG_SWDIO_GPIO, JTAG_SWDCLK_GPIO);

  _env.initialisation();

  switch(_env.workingMode())
  {
    case ConnecSenS::WMODE_CAMPAIGN_RANGE:
      _env.startCampaignRange();
      break;

    default:
      _env.lookAtResetSource();
      // Wait for two consecutive process() calls that return false to call it quit.
      while(1)
      {
	res_previous = true;
	while(1)
	{
	  res = _env.process();
	  if(!res && !res_previous) break;
	  res_previous = res;
	}

	// Some interruptions can wake us up quite often, for example when receiving
	// data from UART where we may get an interruption per data byte.
	// So do not be eager to go to sleep, let's have a dry run.
	// We'll sleep not so deep for a little while and if we have not been woken up during
	// that time, then we'll go to deeper sleep
	refMs = board_ms_now();
	pwrclk_sleep_ms_max(200);
	if(!board_is_timeout(refMs, 200)) { continue; }  // Do not go to sleep.

	while(_env.process());  // To be sure that there is nothing left to do.

	_env.EndOfExecution();
      }
      break;
  }
  while (1) { board_reset(BOARD_SOFTWARE_RESET_ERROR); }
}




/**
  * Setup the microcontroller system.
  */
void SystemInit(void)
{
  // FPU settings
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif

    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set MSION bit */
    RCC->CR |= RCC_CR_MSION;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON , HSION, and PLLON bits */
    RCC->CR &= (uint32_t)0xEAF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x00001000;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;


  // Disable all interrupts
  RCC->CIER = 0x00000000;

  // Configure the Vector Table location add offset address
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE  | VECT_TAB_OFFSET; // Vector Table Relocation in Internal SRAM
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; // Vector Table Relocation in Internal FLASH
#endif
}


