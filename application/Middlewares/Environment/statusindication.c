/*
 * statusindication.c
 *
 *  Created on: 3 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */
#include "statusindication.h"
#include "leds.h"
#include "defs.h"
#include "config.h"
#include "buzzer.h"
#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defines the activity status flags
   */
  typedef enum StatusFlag
  {
    STATUS_FLAG_NONE        = 0,
    STATUS_FLAG_AWAKE       = 1u << 0,
    STATUS_FLAG_CFG_ERROR   = 1u << 1,
    STATUS_FLAG_GPS_FIX     = 1u << 2,
    STATUS_FLAG_RF_ACTIVITY = 1u << 3
  }
  StatusFlag;
  typedef uint8_t Status;  ///< A ORed combination of StatusFlag values;

#define LED1_FLAGS  (STATUS_FLAG_AWAKE | STATUS_FLAG_RF_ACTIVITY)
#define LED2_FLAGS  (STATUS_FLAG_GPS_FIX)

  static Status status_ind_status; // = STATUS_FLAG_NONE


  /**
   * Indicate current status
   */
  void status_ind_set_status(StatusInd status)
  {
    uint16_t i;

    switch(status)
    {
      case STATUS_IND_CFG_ERROR:
	status_ind_status |= STATUS_FLAG_CFG_ERROR;
	// If the USB cable is plugged while in the loop then the USB interruption will take over.
	for(i = 0; i < 100; i++)  // 100 => Loop for about 40 seconds.
	{
	  leds_turn_on( LED_ALL);
	  board_delay_ms(200);
	  leds_turn_off(LED_ALL);
	  board_delay_ms(200);
	}
	board_reset(BOARD_SOFTWARE_RESET_OK); // Reset; the watchdog should have reseted us by now.
	break;

      case STATUS_IND_AWAKE:
	status_ind_status |=  STATUS_FLAG_AWAKE;
	break;

      case STATUS_IND_ASLEEP:
	status_ind_status &= ~STATUS_FLAG_AWAKE;
	break;

      case STATUS_IND_GPS_FIX_TRYING:
	status_ind_status |=  STATUS_FLAG_GPS_FIX;
	break;

      case STATUS_IND_GPS_FIX_OK:
	leds_blinky(LED_2, 100, 30);
      case STATUS_IND_GPS_FIX_KO:
	status_ind_status &= ~STATUS_FLAG_GPS_FIX;
	break;

      case STATUS_IND_RF_SEND_TRYING:
	status_ind_status |=  STATUS_FLAG_RF_ACTIVITY;
	break;

      case STATUS_IND_RF_SEND_OK:
	for(i = 0; i < 30; i++)
	{
	  leds_toggle(LED_1);
	  buzzer_toggle();
	  board_delay_ms(100);
	}
      case STATUS_IND_RF_SEND_KO:
	buzzer_turn_off();
	status_ind_status &= ~STATUS_FLAG_RF_ACTIVITY;
	break;

      default:
	// This should not happen; do nothing.
	break;
    }

    // Update LEDs
    if(status_ind_status == STATUS_FLAG_NONE)
    {
      leds_turn_off(LED_ALL);
      leds_deinit();  // For power saving reasons.
    }
    else
    {
      leds_init();  // To be sure
      leds_turn(LED_1, (LEDState)((status_ind_status & LED1_FLAGS) != 0));
      leds_turn(LED_2, (LEDState)((status_ind_status & LED2_FLAGS) != 0));
    }
  }

#ifdef __cplusplus
}
#endif
