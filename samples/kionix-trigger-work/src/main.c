/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

#define SLEEP_TIME_MS 1000 // 200 // 4000 // 1000
#define DEFAULT_MESSAGE_SIZE 240

// Demo / early development tests:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID    (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID            (1)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ (1)



//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

#include <stdio.h>                 // to provide snprintf()

#include <zephyr/kernel.h>

#include <zephyr/device.h>         // to provide DEVICE_DT_GET macro and many related

#include <zephyr/drivers/sensor.h> // to provide 'struct sensor_value'

// https://docs.zephyrproject.org/latest/services/logging/index.html#c.LOG_MODULE_REGISTER
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kionix_driver_demo, LOG_LEVEL_DBG);



#include <kx132-1211.h>



//----------------------------------------------------------------------
// - SECTION - file scoped
//----------------------------------------------------------------------

// data structures in Kionix driver dev work:

union generic_data_four_bytes_union_t {
    char as_string[sizeof(int)];
    uint8_t as_bytes[sizeof(int)];
    uint32_t as_32_bit_integer;
};

// # REF https://docs.zephyrproject.org/latest/hardware/peripherals/sensor.html#triggers
// # REF https://github.com/tedhavelka/zephyr-driver-work-v2/blob/main/drivers/kionix/kx132-1211/Kconfig#L76

#ifdef CONFIG_KX132_TRIGGER
#warning "compiling KX132 sensor_trigger file scoped instance,"
static struct sensor_trigger trig;
#endif



//----------------------------------------------------------------------
// - SECTION - routines
//----------------------------------------------------------------------

#ifdef CONFIG_KX132_TRIGGER
#warning "compiling KX132 trigger handler,"
static void trigger_handler(const struct device *dev, 
                            const struct sensor_trigger *trig)

{
//    struct sensor_value xyz_readings;
//    static size_t cnt;
//    int rc;

// # REF https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/drivers/sensor.h#L61
#if 0
    printk("\n- KX132 demo app - interrupt of type SENSOR_TRIG_DATA_READY detected,\n");
    printk("- KX132 demo app - for sensor channel SENSOR_CHAN_ACCEL_XYZ\n\n");
#else
    printk("- zztop -\n");
#endif
}
#endif



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

    uint32_t rstatus = 0;

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
        if (0) // if ( DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS == 1 )
        {
            printk("- MARK 1 -\n");

            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,
              &requested_config
            );
        }

        if (1)
        {
            printk("- MARK 2 -\n");

            requested_config.val1 = KX132_ENABLE_SYNC_READINGS_WITH_HW_INTERRUPT;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,
              &requested_config
            );
        }

        if (0) // ( DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE == 1 )
        {
            printk("- MARK 3 -\n");

            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;
            requested_config.val2 = KX132_ODR_3200_HZ;

            sensor_api_status = sensor_attr_set(
              dev_accelerometer,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,          
              &requested_config
            );
        }

#ifdef CONFIG_KX132_TRIGGER
#warning "compiling KX132 trigger assignment code,"

//	if (rc == 0) // may be some windowing or threshold setup to do before assigning values to trig structure - TMH
	if ( 1 ) {
            trig.type = SENSOR_TRIG_DATA_READY;
            trig.chan = SENSOR_CHAN_ACCEL_XYZ;
            rstatus = sensor_trigger_set(dev_accelerometer, &trig, trigger_handler);
        }

        if ( rstatus != 0) {
            printf("Trigger set failed: %d\n", rstatus);
            return;
        }

        printk("Trigger set got %d\n", rstatus);

#else
//const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 });
#define DT_DRV_COMPAT kionix_kx132_1211
const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(0, drdy_gpios, { 0 }); // hmm, this results in correct name `&gpio1`
////const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, { 0 });

    printk("- MARK 4 - about to test local gpio_dt_spec int_gpio.port->name in diag statement . . .\n");
//    printk("- note symbol `inst` holds %u\n", inst);
    if ( int_gpio_for_diag.port != NULL )
    { 
        printk("- MARK 5 - in demo main.c, interrupt GPIO port name holds '%s',\n", int_gpio_for_diag.port->name);
    }
    else
    {
        printk("- MARK 5 - in demo main.c, interrupt GPIO port found NULL!\n");
    }
#endif // CONFIG_KX132_TRIGGER
    }
    else
    {
        return;
    }


    while ( 1 )
    {

        if ( dev_accelerometer != NULL )
        {
            printk("- MARK 6 -\n");

            if ( DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID )
            {
                printk("- MARK 7 - loop count %u\n", main_loop_count);

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
            else
            {
                printk("- skipping x,y,z readings fetch, to observe trigger line on o-scope\n");
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

    } // end while ( 1 )
}
