/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#define SLEEP_TIME_MS 1000
#define DEFAULT_MESSAGE_SIZE 240

// Demo / early development tests:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID    (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID            (1)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ (1)



#include <stdio.h>                 // to provide snprintf()

#include <zephyr/kernel.h>

#include <zephyr/device.h>         // to provide DEVICE_DT_GET macro and many related

#include <zephyr/drivers/sensor.h> // to provide 'struct sensor_value'

// https://docs.zephyrproject.org/latest/services/logging/index.html#c.LOG_MODULE_REGISTER
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kionix_driver_demo, LOG_LEVEL_DBG);



#include <kx132-1211.h>



//----------------------------------------------------------------------
// - SECTION - data structures in Kionix driver dev work
//----------------------------------------------------------------------

union generic_data_four_bytes_union_t {
    char as_string[sizeof(int)];
    uint8_t as_bytes[sizeof(int)];
    uint32_t as_32_bit_integer;
};





void main(void)
{

    printk("Kionix driver demo starting, on board %s . . .\n", CONFIG_BOARD);


// --- VAR BEGIN ---

    struct sensor_value value;  // sensor_value is struct with two uint32_t's, val1 and val2

    struct sensor_value requested_config;

    int sensor_api_status = 0;

    union generic_data_four_bytes_union_t data_from_sensor;
    uint32_t i = 0;

    int main_loop_count = 0;
    char lbuf[DEFAULT_MESSAGE_SIZE];

// If you want a shorthand for specific KX132 expansion in DT_NODELABEL macro
// you may define a symbol for your app this way:

#define KX132_NODE DT_NODELABEL(kionix_sensor)

    const struct device *dev_accelerometer = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor));

// --- VAR END ---


// - STEP - check whether we get a non-null device handle, then check whether device ready

    if (dev_accelerometer == NULL)
    {
        printk("-\n- WARNING - Failed to init Kionix sensor device pointer!\n-\n");
    }
    else
    {
        if (!device_is_ready(dev_accelerometer))
        {
            snprintf(lbuf, sizeof(lbuf), "Device %s is not ready\n", dev_accelerometer->name);
            printk("%s", lbuf);
            return;
        }
        else
        {
            printk("- SUCCESS - found Kionix accelerometer and device is ready\n");
        }
    }


// - STEP - configure a couple of KX132 accelerometer settings

    if (dev_accelerometer != NULL)
    {
        if (1) // if ( DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS == 1 )
        {
            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,          
              &requested_config
            );
        }

        if (1) // ( DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE == 1 )
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
    }


    while ( 1 )
    {

        if ( dev_accelerometer != NULL )
        {

            if ( DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID )
            {
                sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID);
                sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_MANUFACTURER_ID, &value);

                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer %d\n",
                  value.val1);
                printk("%s", lbuf);
                snprintf(lbuf, sizeof(lbuf), "main.c - sensor_value.val2 holds %d\n", value.val2);
                printk("%s", lbuf);
                data_from_sensor.as_32_bit_integer = value.val1;

                printk("main.c - value.val1 as bytes:  ");
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), "0x%2X ", data_from_sensor.as_bytes[i]);
                    printk("%s", lbuf);
                }
                printk("  \"");
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), " %c ", data_from_sensor.as_bytes[i]);
                    printk("%s", lbuf);
                }
                snprintf(lbuf, sizeof(lbuf), "\"\n");
                printk("%s", lbuf);
            }

            if ( DEV_TEST__FETCH_AND_GET_PART_ID )
            {
                sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID);
                sensor_channel_get(dev_accelerometer, SENSOR_CHAN_KIONIX_PART_ID, &value);
                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports part ID of %d\n",
                  value.val1);
                printk("%s", lbuf);
            }


            if ( DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ )
            {
                sensor_api_status = sensor_sample_fetch_chan(dev_accelerometer, SENSOR_CHAN_ACCEL_XYZ);
                sensor_channel_get(dev_accelerometer, SENSOR_CHAN_ACCEL_XYZ, &value);
                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor x,y,z readings encoded:  0x%08x, 0x%08x\n\n",
                  value.val1, value.val2);
                printk("%s", lbuf);
            }

        } 
        else 
        {
            snprintf(lbuf, sizeof(lbuf), "- WARNING - problem initializing KX132 Zephyr device pointer,\n");
            printk("%s", lbuf);
            snprintf(lbuf, sizeof(lbuf), "- WARNING + therefore not exercising features of this sensor.\n\n");
            printk("%s", lbuf);
        } // if dev_accelerometer != NULL

        k_msleep(SLEEP_TIME_MS);
        ++main_loop_count;
    } 
}
