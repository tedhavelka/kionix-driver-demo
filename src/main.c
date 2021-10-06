/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

// 2021-08-24, 2021-08-26 losing double quotes in favor of arrow brackets:
// ( We can arrow bracket this included file's name thanks to out-of-tree,
//  KX132_1211 driver CMakeLists.txt file and zephyr_include_directories()
//  stanza, which points to the path from top dir of driver to this
//  header file. )
#include <kx132-1211.h>


// 2021-08-26 Blindly borrowing next four active lines from Jared Wolff's
//  air-quality-wing-zephyr-demo project:
#include <logging/log.h>
LOG_MODULE_REGISTER(demo);

/* Device name defintions*/
//#define SHTC3 DT_INST(0, sensirion_shtc3)
#define KX132_1211 DT_INST(0, kionix_kx132_1211)
#define CONFIG_KX132_1211_DEV_NAME DT_LABEL(KX132_1211)

// https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
#define KIONIX_ACCELEROMETER DT_NODELABEL(kionix_sensor)



//----------------------------------------------------------------------
// - SECTION - defines from Nordic ncs Zephyr sample apps
//----------------------------------------------------------------------

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1500 // 1000

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
// - SECTION - defines, data structures in Kionix driver dev work
//----------------------------------------------------------------------

union generic_data_four_bytes_union_t {
    char as_string[sizeof(int)];
    uint8_t as_bytes[sizeof(int)];
    uint32_t as_32_bit_integer;
};


//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

void main(void)
{
    const struct device *dev;
    bool led_is_on = true;
    int ret;
    int main_loop_count = 0;

// 2021-10-05
// REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
    const struct device *uart_for_cli;
//    uart_for_cli = device_get_binding(DT_LABEL(UART_2));
    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
    if ( uart_for_cli == NULL )
    {
        printk("Failed to assign pointer to UART2 device!\n");
    }

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
    if (ret < 0) {
        return;
    }


//    const struct device *dev_accelerometer;
//    dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));
//    const struct device *dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));
    const struct device *dev_accelerometer = device_get_binding(DT_LABEL(KIONIX_ACCELEROMETER));
    struct sensor_value value;
    union generic_data_four_bytes_union_t data_from_sensor;
    uint32_t i = 0;


    if (dev_accelerometer == NULL) {
        printk("Failed to init Kionix sensor device pointer!\n");
        printk("firmware exiting early, done.\n\n");
        return;
    }

#if 1
    if (!device_is_ready(dev_accelerometer))
    {
        printk("Device %s is not ready\n", dev_accelerometer->name);
        return;
    }
    else
#endif
    {
        printk("- SUCCESS - found Kionix accelerometer and device is ready\n");
    }


// 2021-08-11 WED - 
//    query_kx132_id(dev_accelerometer, 0);
//    kx132_device_id_fetch(dev_accelerometer, 0);  // second parameter is 'channel', not yet used - TMH


    while (1) {
        gpio_pin_set(dev, PIN, (int)led_is_on);
        led_is_on = !led_is_on;
        k_msleep(SLEEP_TIME_MS);

        printk("Hello World! %s - built via Segger Nordic Edition v5.60\n", CONFIG_BOARD);

// Calls to KX132-1211 driver API:
// NOTE:  these routines do not appear by these names in Zephyr RTOS project,
//  but rather they are generated at build time by Python or other scripts
//  of the build process, and are in part (or full) compiled from developer's
//  project code.  Scratch-the-surface documentation on this at:
// *  https://docs.zephyrproject.org/1.14.1/reference/peripherals/sensor.html
        sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID);
        sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID, &value);

        printk("main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer %d\n", value.val1);
        printk("main.c - sensor_value.val2 holds %d\n", value.val2);
//        memcpy(data_from_sensor.as_32_bit_integer, value.val1, sizeof(value.val1));
        data_from_sensor.as_32_bit_integer = value.val1;

        printk("main.c - value.val1 as bytes:  ");
        for ( i = 0; i < sizeof(int); i++ )
        {
            printk("0x%2X ", data_from_sensor.as_bytes[i]);
        }
        printk("  \"");
        for ( i = 0; i < sizeof(int); i++ )
        {
            printk(" %c ", data_from_sensor.as_bytes[i]);
        }
        printk("\"\n");


        sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID);
        sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID, &value);
        printk("main.c - Kionix sensor reports part ID of %d\n", value.val1);


// Output periodic or multi-phasic blank line to highlight scrolling in terminal window:

//        if ( led_is_on == 0 )
        if ( (main_loop_count % 3) == 0 )
//        if (((main_loop_count % 3) == 0 ) || ( (main_loop_count % 5) == 0 ))
        {
            printk("\n\n");
        }


// --- UART_2 CLI work begin ---
        if ( uart_for_cli != NULL )
        {
            char lbuf[160];
            unsigned char* msg = lbuf;
            uart_poll_in(uart_for_cli, msg);

            printk("%s", msg);
        }
// --- UART_2 CLI work end ---

        ++main_loop_count;
    }
}
