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
#include <drivers/sensor.h>        // to provide ...()
#include <drivers/gpio.h>          //


// application includes:
#include "return-values.h"
#include "module-ids.h"
#include "development-flags.h"



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------


#define KD_THREAD_LED_PRIORITY 10

#define THREAD_LED_STACK_SIZE 1024
// NEED to create app header file for app thread priorities - TMH
#define THREAD_LED_PRIORITY KD_THREAD_LED_PRIORITY

#define TASK_LED_MAIN_LOOP_PERIOD_IN_MS (500)



//----------------------------------------------------------------------
// - SECTION - prototypes
//----------------------------------------------------------------------

void thread_led_entry_point(void* arg1, void* arg2, void* arg3);



//----------------------------------------------------------------------
// - SECTION - file scoped
//----------------------------------------------------------------------

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
// #define LED0_LABEL   DT_GPIO_LABEL(LED0_NODE, gpios)  // DT_GPIO_LABEL() macro deprecated as of Zephyr 3.2.0
#define LED0_PIN     DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS   DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
// #define LED0_LABEL   ""
#define LED0_PIN     0
#define LED0_FLAGS   0
#endif


#define LED2_NODE DT_ALIAS(led2)

#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
// #define LED2_LABEL   DT_GPIO_LABEL(LED2_NODE, gpios)  // DT_GPIO_LABEL() macro deprecated as of Zephyr 3.2.0
#define LED2_PIN     DT_GPIO_PIN(LED2_NODE, gpios)
#define LED2_FLAGS   DT_GPIO_FLAGS(LED2_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink a blue LED. */
#warning "Unsupported board: led2 devicetree alias is not defined"
// #define LED2_LABEL   ""
#define LED2_PIN     0
#define LED2_FLAGS   0
#endif

const struct device *dev_led_red = NULL;
const struct device *dev_led_blue = NULL;



//----------------------------------------------------------------------
// - SECTION - routines
//----------------------------------------------------------------------

K_THREAD_STACK_DEFINE(thread_led_stack_area, THREAD_LED_STACK_SIZE);

struct k_thread thread_led_data;


int initialize_thread_led(void)
{
    int rstatus = 0;

    k_tid_t task_led_tid = k_thread_create(&thread_led_data, thread_led_stack_area,
                                            K_THREAD_STACK_SIZEOF(thread_led_stack_area),
                                            thread_led_entry_point,
                                            NULL, NULL, NULL,
                                            THREAD_LED_PRIORITY,
                                            0,
                                            K_MSEC(1000)); // K_NO_WAIT);

// REF https://docs.zephyrproject.org/2.6.0/reference/kernel/threads/index.html?highlight=k_thread_create#c.k_thread_name_set
// int k_thread_name_set(k_tid_t thread, const char *str)
    rstatus = k_thread_name_set(task_led_tid, MODULE_ID__THREAD_LED);
    if ( rstatus == 0 ) { } // avoid compiler warning about unused variable - TMH

    return (int)task_led_tid;
}






void thread_led_entry_point(void* arg1, void* arg2, void* arg3)
{

// --- VAR BEGIN ---
    int loop_count = 0;

    uint32_t led_is_on = 0;

    uint32_t rstatus = ROUTINE_OK;
// --- VAR END ---



    (void)loop_count;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - STEP - device readiness detection
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// # REF https://docs.zephyrproject.org/latest/kernel/drivers/index.html#c.device_get_binding
// # REF https://docs.zephyrproject.org/latest/kernel/drivers/index.html#c.DEVICE_DT_NAME

// LED red:

    dev_led_red = device_get_binding(DEVICE_DT_NAME(LED0_NODE));
    if (dev_led_red == NULL) {
        return;
    }

    rstatus = gpio_pin_configure(dev_led_red, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    if (rstatus < 0) {
        return;
    }


// LED blue:

#if 0 // if board does not have led2 node or node alias
    dev_led_blue = device_get_binding(LED2_LABEL);
    if (dev_led_blue == NULL) {
        return;
    }

    rstatus = gpio_pin_configure(dev_led_blue, LED2_PIN, GPIO_OUTPUT_ACTIVE | LED2_FLAGS);
    if (rstatus < 0) {
        return;
    }
#endif


    while ( 1 )
    {
        gpio_pin_set(dev_led_red, LED0_PIN, (int)led_is_on);
        led_is_on = !led_is_on;

#if 0 // if board does not have led2 node or node alias
        gpio_pin_set(dev_led_blue, LED2_PIN, (int)led_is_on);
#endif

        k_msleep(TASK_LED_MAIN_LOOP_PERIOD_IN_MS);
    }

}




// --- EOF ---
