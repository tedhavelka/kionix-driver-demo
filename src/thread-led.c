//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/tedhavelka/kionix-driver-demo
//
//      File:  thread_led.c
//
//----------------------------------------------------------------------

/*
 *  @Brief:  Zephyr thread to control LEDs on app targeted board.
 */



//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

// Zephyr RTOS includes:

#include <device.h>                // to provide device_get_binding()


// application includes:
#include "return-values.h"
#include "module-ids.h"
#include "development-flags.h"



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------


#define KD_THREAD_LED_PRIORITY 10

#define THREAD_LED_STACK_SIZE 1024
#define THREAD_LED_PRIORITY KD_THREAD_LED_PRIORITY



//----------------------------------------------------------------------
// - SECTION - prototypes
//----------------------------------------------------------------------

void thread_led_entry_point(void* arg1, void* arg2, void* arg3);



//----------------------------------------------------------------------
// - SECTION - file scoped
//----------------------------------------------------------------------







