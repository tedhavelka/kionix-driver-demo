/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

// 2021-08-06 - blink-custom first addition of Zephyr functionality:
#include <sys/printk.h>

// 2021-08-24
#include "kx132-1211.h"



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

// 2021-08-17 - adding symbol to allow for enabling local driver under development:
#define KX132_1211




void query_kx132_id(const struct device *dev_ptr_accelerometer, unsigned int options)
{

    if ( dev_ptr_accelerometer != NULL ) { }

    if ( options > 0 ) { }

}


void main(void)
{
    const struct device *dev;
    bool led_is_on = true;
    int ret;
    int main_loop_count = 0;

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
    if (ret < 0) {
        return;
    }


    const struct device *dev_accelerometer;
    dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));
    if (dev_accelerometer == NULL) {
        printk("Failed to init Kionix sensor device pointer!\n");
        printk("firmware exiting early, done.\n\n");
        return;
    }
    else
    {
        printk("- SUCCESS - assigned Kionix accelerometer device to pointer,\n");
    }


// 2021-08-11 WED - 
//    query_kx132_id(dev_accelerometer, 0);
    kx132_device_id_fetch(dev_accelerometer, 0);  // second parameter is 'channel', not yet used - TMH


    while (1) {
        gpio_pin_set(dev, PIN, (int)led_is_on);
        led_is_on = !led_is_on;
        k_msleep(SLEEP_TIME_MS);

//        if ( led_is_on == 0 )
        if ( (main_loop_count % 3) == 0 )
        {
            printk("Hello World! %s\n\n", CONFIG_BOARD);
        }
        else
        {
            printk("Hello World! %s\n", CONFIG_BOARD);
        }

        ++main_loop_count;
    }
}
