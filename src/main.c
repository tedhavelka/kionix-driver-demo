/*
 * Copyright (c) 2021 Neela Nurseries
 *
 * Based on 2016 Intel Corporation sample Zephyr app, "basic/blinky"
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define KIONIX_DRIVER_REV_MAJOR 0
#define KIONIX_DRIVER_REV_MINOR 2
// revision updated 2021-10-06

//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

// Zephyr RTOS includes:

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

// 2021-08-26 - air-quality-wing-zephyr-demo by Jared Wolff includes this Zephyr project header:
#include <drivers/sensor.h>

// 2021-08-06 - blink-custom first addition of Zephyr functionality:
#include <sys/printk.h>

// 2021-10-05 - CLI incorporation work, see Zephyr v2.6.0 file "zephyr/subsys/console/tty.c"
#include <drivers/uart.h>   // to provide uart_poll_in()

// 2021-08-26 Blindly borrowing next four active lines from Jared Wolff's
//  air-quality-wing-zephyr-demo project:
// REF https://docs.zephyrproject.org/2.6.0/reference/logging/index.html?highlight=log_module_register#c.LOG_MODULE_REGISTER
#include <logging/log.h>
LOG_MODULE_REGISTER(demo);


// newlib C includes:

#include <stdio.h> // to provide snprintf()


// Out-Of-Tree driver includes:

// 2021-08-24, 2021-08-26 losing double quotes in favor of arrow brackets:
// ( We can arrow bracket this included file's name thanks to out-of-tree,
//  KX132_1211 driver CMakeLists.txt file and zephyr_include_directories()
//  stanza, which points to the path from top dir of driver to this
//  header file. )
#include <kx132-1211.h>

// Local project includes:

// 2021-10-06 - to add wrapper about Zephyr printk(), for early CLI development work:
#include "diagnostic.h"

// 2021-10-16 -
#include "thread-iis2dh.h"
#include "thread-lis2dh.h"
#include "thread-simple-cli.h"
#include "scoreboard.h"

#include "version.h"



//----------------------------------------------------------------------
// - SECTION - symbol defines
//----------------------------------------------------------------------

/* Device name defintions*/
//#define SHTC3 DT_INST(0, sensirion_shtc3)
#define KX132_1211 DT_INST(0, kionix_kx132_1211)
#define CONFIG_KX132_1211_DEV_NAME DT_LABEL(KX132_1211)

// https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
#define KIONIX_ACCELEROMETER DT_NODELABEL(kionix_sensor)


// defines from Nordic sdk-nrf sample apps:

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000 // 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0     DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN      DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS    DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0    ""
#define PIN      0
#define FLAGS    0
#endif



//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

// --- DEVELOPMENT FLAGS BEGIN ---

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL

// KX132-1211 Configuration:
#define DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS (1)
#define DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE         (0)

// KX132-1211 Readings:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID           (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID                   (1)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ        (1)


#define NN_DEV__ENABLE_INT_MAIN_TESTS                     (0)
#define NN_DEV__ENABLE_THREAD_IIS2DH_SENSOR               (1)
#define NN_DEV__ENABLE_THREAD_LIS2DH_SENSOR               (0)
#define NN_DEV__ENABLE_THREAD_SIMPLE_CLI                  (1)

#define NN_DEV__ENABLE_IIS2DH_TEMPERATURE_READGINGS       (0)

#define NN_DEV__TEST_SCOREBOARD_GLOBAL_SETTING            (1)

// --- DEVELOPMENT FLAGS END ---



//----------------------------------------------------------------------
// - SECTION - data structures in Kionix driver dev work
//----------------------------------------------------------------------

union generic_data_four_bytes_union_t {
    char as_string[sizeof(int)];
    uint8_t as_bytes[sizeof(int)];
    uint32_t as_32_bit_integer;
};

// Statically declare banner message string allows us to memset once at start of int main():
static char banner_msg[128];



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

void banner(const char* caller)
{
    snprintf(banner_msg, sizeof(banner_msg), "+++ Kionix Driver Demo version %up%u +++\n",
      KIONIX_DRIVER_REV_MAJOR, KIONIX_DRIVER_REV_MINOR);
    dmsg(banner_msg, DIAG_NORMAL);

    (void)caller;
}



#if 0
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// - DEV BEGIN - 2021-10-06 thread addition work
// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#spawning-a-thread

#define CLI_STACK_SIZE 500
#define CLI_THREAD_PRIORITY 5

// Zephyr required signature for Zephyr thread entry point routines:
// extern void my_entry_point(void *, void *, void *);

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(cli_stack_area, CLI_STACK_SIZE);
struct k_thread cli_thread_data;

void cli_entry_point(void* arg1, void* arg2, void* arg3)
{
// stub function
}


// - DEV END -
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#endif



void main(void)
{
// --- LOCAL VAR BEGIN ---
    const struct device *dev;
    bool led_is_on = true;
    int main_loop_count = 0;
//    int ret;
    uint32_t rstatus = 0;

    int thread_set_up_status = 0;

    int sensor_api_status = 0;
    struct sensor_value requested_config;

    char lbuf[DEFAULT_MESSAGE_SIZE];
// --- LOCAL VAR END ---


    memset(banner_msg, 0, sizeof(banner_msg));
    banner("main");


#if 0
// 2021-10-05
// REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
    const struct device *uart_for_cli;
//    uart_for_cli = device_get_binding(DT_LABEL(UART_2));
//    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));

// Selecting between UART instances prior to additonal wiring:
    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
//    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
    if ( uart_for_cli == NULL )
    {
        dmsg("Failed to assign pointer to UART2 device!\n", PROJECT_DIAG_LEVEL);
    }
#endif


    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    rstatus = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
    if (rstatus < 0) {
        return;
    }

    const struct device *dev_accelerometer = device_get_binding(DT_LABEL(KIONIX_ACCELEROMETER));
//    struct sensor_value value;
//    union generic_data_four_bytes_union_t data_from_sensor;
//    uint32_t i = 0;


    if (dev_accelerometer == NULL)
    {
        dmsg("Failed to init Kionix sensor device pointer!\n", PROJECT_DIAG_LEVEL);
        snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "Zephyr macros '(DT_LABEL(KIONIX_ACCELEROMETER))' expand to value '%s'\n",
          (DT_LABEL(KIONIX_ACCELEROMETER)));
        dmsg(lbuf, PROJECT_DIAG_LEVEL);
//        dmsg("firmware exiting early, done.\n\n", PROJECT_DIAG_LEVEL);
//        return;
    }
    else
    {
#if 1
        if (!device_is_ready(dev_accelerometer))
        {
            snprintf(lbuf, sizeof(lbuf), "Device %s is not ready\n", dev_accelerometer->name);
            dmsg(lbuf, PROJECT_DIAG_LEVEL);
            return;
        }
        else
#endif
        {
            dmsg("- SUCCESS - found Kionix accelerometer and device is ready\n", PROJECT_DIAG_LEVEL);
        }
    }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// In following sensor configuration tests first three parameters
//  1 device pointer, 2 sensor channel, 3 sensor attribute stay the same
//  to honor two sensor API enumerations in Zephyr v2.6.0.  These enums
//  do not in any obvious way support arbitrary sensor channels and
//  attributes.  Thus to stay with Zephyr's API way, and pass custom
//  settings to configuring routines of our out-of-tree driver those
//  values we pass in the final parameter.
//
//  Zephyr 2.6.0 project header of interest in this matter is:
//  zephyr/include/drivers/sensor.h.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if (dev_accelerometer != NULL)
    {
        if ( DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS == 1 )
        {
            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,          
              &requested_config
            );
        }

        if ( DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE == 1 )
        {
            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;
            requested_config.val2 = KX132_ODR_3200_HZ;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,          
              &requested_config
            );
        }

        thread_set_up_status = 0;
    }
    else
    {
        thread_set_up_status = 1;
    }


#if 0
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// - DEV START -

// 2021-10-06 - work to add and test Zephyr thread:
// - DEV THREAD WORK BEGIN -

// Set up first development thread, code for this one entirely in main.c:
        k_tid_t cli_tid = k_thread_create(&cli_thread_data, cli_stack_area,
                                          K_THREAD_STACK_SIZEOF(cli_stack_area),
                                          cli_entry_point,
                                          NULL, NULL, NULL,
                                          CLI_THREAD_PRIORITY, 0, K_NO_WAIT);
// - DEV END -
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#endif

// - DEV THREAD WORK END -


#if NN_DEV__ENABLE_THREAD_IIS2DH_SENSOR == 1
    {
        dmsg("- DEV - starting IIS2DH test thread . . .\n", DIAG_NORMAL);
        thread_set_up_status = initialize_thread_iis2dh_task();
    }
#endif

#if NN_DEV__ENABLE_THREAD_LIS2DH_SENSOR == 1
    dmsg("- DEV - starting comparative LIS2DH test thread . . .\n", DIAG_NORMAL);
    thread_set_up_status = initialize_thread_lis2dh_task();
#endif

#if NN_DEV__ENABLE_THREAD_SIMPLE_CLI == 1
    dmsg("- DEV - starting simple Zephyr based CLI thread . . .\n", DIAG_NORMAL);
    thread_set_up_status = initialize_thread_simple_cli_task();
#endif

#if NN_DEV__TEST_SCOREBOARD_GLOBAL_SETTING == 1
// Note this static var declared in thread_simple_cli.h:
//    dmsg("- DEV - setting scoreboard test value to 4 . . .\n", DIAG_NORMAL);
//    global_test_value = 4;
//    dmsg("- DEV - (this assignment doesn't seem to work.)\n", DIAG_NORMAL);
        dmsg("- DEV - setting scoreboard test value to 5 . . .\n", DIAG_NORMAL);
        rstatus = set_global_test_value(5);
#endif


    while ( 1 )
    {
        gpio_pin_set(dev, PIN, (int)led_is_on);
        led_is_on = !led_is_on;

#if NN_DEV__TEST_SCOREBOARD_GLOBAL_SETTING == 1
//        dmsg("- DEV - setting scoreboard test value to 5 in while loop . . .\n", DIAG_NORMAL);
//        rc = set_global_test_value(5);
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calls to KX132-1211 driver API:
// NOTE:  these routines do not appear by these names in Zephyr RTOS project,
//  but rather they are generated at build time by Python or other scripts
//  of the build process, and are in part (or full) compiled from developer's
//  project code.  Scratch-the-surface documentation on this at:
// *  https://docs.zephyrproject.org/1.14.1/reference/peripherals/sensor.html
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NN_DEV__ENABLE_INT_MAIN_TESTS == 1

        if (dev_accelerometer != NULL)
        {

            if ( DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID )
            {
                sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID);
                sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID, &value);

                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer %d\n", value.val1);
                dmsg(lbuf, PROJECT_DIAG_LEVEL);
                snprintf(lbuf, sizeof(lbuf), "main.c - sensor_value.val2 holds %d\n", value.val2);
                dmsg(lbuf, PROJECT_DIAG_LEVEL);
                data_from_sensor.as_32_bit_integer = value.val1;

                dmsg("main.c - value.val1 as bytes:  ", PROJECT_DIAG_LEVEL);
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), "0x%2X ", data_from_sensor.as_bytes[i]);
                    dmsg(lbuf, PROJECT_DIAG_LEVEL);
                }
                dmsg("  \"", PROJECT_DIAG_LEVEL);
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), " %c ", data_from_sensor.as_bytes[i]);
                    dmsg(lbuf, PROJECT_DIAG_LEVEL);
                }
                dmsg("\"\n", PROJECT_DIAG_LEVEL);
            }

            if ( DEV_TEST__FETCH_AND_GET_PART_ID )
            {
                sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID);
                sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID, &value);
                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports part ID of %d\n", value.val1);
                dmsg(lbuf, PROJECT_DIAG_LEVEL);
            }

            if ( DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ )
            {
                sensor_api_status = sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_ACCEL_XYZ);
            }

        } 
        else 
        {
            dmsg("- WARNING - problem initializing KX132 Zephyr device pointer,", PROJECT_DIAG_LEVEL);
            dmsg("- WARNING + therefore not exercising features of this sensor.", PROJECT_DIAG_LEVEL);
        } 

// Output periodic or multi-phasic blank line to highlight scrolling in terminal window (note 1):

        if ( (main_loop_count % 3) == 0 )
        {
            dmsg("\n\n", PROJECT_DIAG_LEVEL);
            banner("main");
        }

#endif // NN_DEV__ENABLE_INT_MAIN_TESTS 


#if 0
// --- UART_2 CLI work begin ---
        if ( uart_for_cli != NULL )
        {
            char lbuf[160];
            memset(lbuf, 0, sizeof(lbuf));
            unsigned char* msg = lbuf;
            uart_poll_in(uart_for_cli, msg);

            snprintf(lbuf, sizeof(lbuf), "zzz - %s - zzz\n", msg);
            dmsg(lbuf, DIAG_NORMAL);
        }
// --- UART_2 CLI work end ---
#endif

        k_msleep(SLEEP_TIME_MS);
        ++main_loop_count;
    }
}



//----------------------------------------------------------------------
// - SECTION - Notes
//----------------------------------------------------------------------

/*

(1)  Output periodic or multi-phasic blank line to highlight scrolling in terminal window:

//        if ( led_is_on == 0 )
//        if ( (main_loop_count % 3) == 0 )
//        if (((main_loop_count % 3) == 0 ) || ( (main_loop_count % 5) == 0 ))


(2)  Could not successfully compile when using Zephr DEVICE_DT_GET(...) macro:
//    const struct device *dev_accelerometer;
//    dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));
//    const struct device *dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));
    const struct device *dev_accelerometer = device_get_binding(DT_LABEL(KIONIX_ACCELEROMETER));



*/

// --- EOF --
