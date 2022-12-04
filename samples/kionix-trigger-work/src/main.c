/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

#define SLEEP_TIME_MS 500 // 200 // 4000 // 1000
#define DEFAULT_MESSAGE_SIZE 240

// Demo / early development tests:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID    (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID            (0)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ (0)

// SPI related dev work:
#define BUILD_FOR_SPI_CONNECTED_IIS2DH (1)

#define DEV_SHOW_FIRST_BYTES_SPI_RX_BUFFER

#define DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS (0)
#define DEV_TEST__KX132_ENABLE_SYNC_READINGS_WITH_HW_INTERRUPT (0)
#define DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE (0)



//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

#include <stdio.h>                 // to provide snprintf()

#include <zephyr/kernel.h>

#include <zephyr/device.h>         // to provide DEVICE_DT_GET macro and many related

#include <zephyr/drivers/sensor.h> // to provide 'struct sensor_value'

#include <zephyr/drivers/spi.h>

// https://docs.zephyrproject.org/latest/services/logging/index.html#c.LOG_MODULE_REGISTER
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kionix_driver_demo, LOG_LEVEL_DBG);


// Out-of-tree Kionix driver header:
#include <kx132-1211.h>

// - DEV 1127 - attempt to create gpio_dt_spec device instance(s) as stand alone devices,
//  outside the larger accelerometer device nested data structures:
#include "int-gpio-inst.h"



//----------------------------------------------------------------------
// - SECTION - file scoped
//----------------------------------------------------------------------

#if BUILD_FOR_SPI_CONNECTED_IIS2DH == 1
#define SIZE_SPI_TX_BUFFER 32
#define SIZE_SPI_RX_BUFFER 32

uint8_t spi_tx_buffer[SIZE_SPI_TX_BUFFER] = { 0 };
uint8_t spi_rx_buffer[SIZE_SPI_RX_BUFFER] = { 0 };

// # REF zephyr/samples/drivers/spi_bitbang/src/main.c line 63
const int stride = sizeof(spi_tx_buffer[0]);

// # REF https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/drivers/spi.h#L435
//   .len = (SIZE_SPI_TX_BUFFER * sizeof(spi_tx_buffer[0]))
struct spi_buf tx_buf = { .buf = spi_tx_buffer, .len = (SIZE_SPI_TX_BUFFER * stride) };
struct spi_buf rx_buf = { .buf = spi_rx_buffer, .len = (SIZE_SPI_RX_BUFFER * stride) };

// # REF https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/drivers/spi.h#L446
// # REF zephyr/samples/drivers/spi_bitbang/src/main.c lines 32 - 36
struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

#endif




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
#warning "- In demo main.c - compiling KX132 trigger handler,"
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
#endif
    printk("- zztop -\n");
}
#endif



static uint32_t read_registers(const struct device *dev,
//                               const uint8_t device_address,
                               const uint8_t* device_register,
                               uint8_t* data,
                               uint8_t len)
{
    uint8_t sensor_register_addr = device_register[0];
    const struct kx132_device_config *cfg = dev->config;
    uint32_t rstatus = 0;

    if ( len > SIZE_SPI_RX_BUFFER )
    {
        printk("- WARNING (demo app) - SPI read of %u bytes exceeds global SPI receive buffer size of %u!\n",
          len, SIZE_SPI_RX_BUFFER);
        printk("returning early . . .\n");
        return rstatus;
    }

    memset(spi_rx_buffer, 0, SIZE_SPI_RX_BUFFER);
    rx_set.buffers = &rx_buf;
    rx_set.count = 1;

    spi_tx_buffer[0] = (sensor_register_addr |= 0x80);  // logical OR with 0b10000000, read bit true, MS bit false.
    tx_buf.len = 2;
    rx_buf.len = len;

    rstatus = spi_transceive(
                             cfg->spi.bus,
                             &cfg->spi.config,
                             &tx_set,
                             &rx_set
                            );

//    *data = spi_rx_buffer[1];
    printk("- INFO (read_registers) - ready to copy %u bytes to callers data buffer\n", len);
//    memcpy(data, spi_rx_buffer[1], (unsigned int)len);
    memcpy(data, &spi_rx_buffer[1], (size_t)len);

#ifdef DEV_SHOW_FIRST_BYTES_SPI_RX_BUFFER
    printk("- DEV 1113 - first few bytes read back via spi_transceive():  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
      spi_rx_buffer[0], spi_rx_buffer[1], spi_rx_buffer[2], spi_rx_buffer[3], spi_rx_buffer[4]);
#endif

    return rstatus;
}



void main(void)
{

    printk("Kionix driver demo starting, on board %s . . .\n", CONFIG_BOARD);


// --- VAR BEGIN ---

    struct sensor_value value;              // sensor_value is struct with two uint32_t's, val1 and val2
    struct sensor_value requested_config;   // we use sensor_value struct to pass requested sensor configuration defined values
    int sensor_api_status = 0;              // dedicated var to capture returns statae from Zephyr sensor API

    union generic_data_four_bytes_union_t data_from_sensor;

    uint32_t i = 0;
    int main_loop_count = 0;
    char lbuf[DEFAULT_MESSAGE_SIZE];

// If you want a shorthand for specific KX132 expansion in DT_NODELABEL macro
// you may define a symbol for your app this way:

#define KX132_NODE DT_NODELABEL(kionix_sensor_1)
    const struct device *dev_kx132_1 = DEVICE_DT_GET(DT_NODELABEL(kionix_sensor_1));

    uint32_t rstatus = 0;

// --- VAR END ---


// - STEP - check whether we get a non-null device handle, then check whether device ready

    if (dev_kx132_1 == NULL)
    {
        printk("-\n- WARNING - Failed to init Kionix sensor device pointer!\n-\n");
    }
    else
    {
        if (!device_is_ready(dev_kx132_1))
        {
            snprintf(lbuf, sizeof(lbuf), "Device %s is not ready\n", dev_kx132_1->name);
            printk("%s", lbuf);
            return;
        }
        else
        {
            printk("- SUCCESS - found Kionix accelerometer and device is ready\n");
        }
    }


// - STEP - configure a couple of KX132 accelerometer settings

    if (dev_kx132_1 != NULL)
    {
        if ( DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS == 1 )
        {
            printk("- MARK 1 -\n");

            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;

            sensor_api_status = sensor_attr_set(
              dev_kx132_1,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,
              &requested_config
            );
        }

        if ( DEV_TEST__KX132_ENABLE_SYNC_READINGS_WITH_HW_INTERRUPT == 1 )
        {
            printk("- MARK 2 -\n");

            requested_config.val1 = KX132_ENABLE_SYNC_READINGS_WITH_HW_INTERRUPT;

            sensor_api_status = sensor_attr_set(
              dev_kx132_1,
              SENSOR_CHAN_ALL,
              SENSOR_ATTR_PRIV_START,
              &requested_config
            );
        }

        if ( DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE == 1 )
        {
            printk("- MARK 3 -\n");

            requested_config.val1 = KX132_ENABLE_ASYNC_READINGS;
            requested_config.val2 = KX132_ODR_3200_HZ;

            sensor_api_status = sensor_attr_set(
              dev_kx132_1,
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
            rstatus = sensor_trigger_set(dev_kx132_1, &trig, trigger_handler);
            printk("main.c, call to sensor_trigger_set() returns status value of %d\n", rstatus);

            rstatus = kx132_trigger_set(dev_kx132_1, &trig, trigger_handler);
            printk("main.c, call to kx132_trigger_set() returns status value of %d\n", rstatus);
        }

        if ( rstatus != 0) {
            printf("main.c, Trigger set failed: %d\n", rstatus);
            return;
        }

        printk("main.c, Trigger set got %d\n", rstatus);

#else
//const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 });
#define DT_DRV_COMPAT kionix_kx132_1211
const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(0, drdy_gpios, { 0 }); // hmm, this results in correct name `&gpio1`

    printk("- MARK 4 - main.c test of local gpio_dt_spec int_gpio.port->name . . .\n");
    if ( int_gpio_for_diag.port != NULL )
    { 
        printk("- MARK 5 - main.c, interrupt GPIO port name holds '%s',\n", int_gpio_for_diag.port->name);
    }
    else
    {
        printk("- MARK 5 - main.c, interrupt GPIO port found NULL!\n");
    }
#endif // CONFIG_KX132_TRIGGER
    }
    else
    {
        return;
    }


    printk("- MARK 6 - main.c, testing FOREACH generated gpio_dt_spec int_gpio_##inst.port->name . . .\n");
    if ( int_gpio_diag1.port != NULL )
    { 
        printk("- MARK 7 - main.c, interrupt GPIO port name holds '%s',\n", int_gpio_diag1.port->name);
    }


IF_ENABLED(CONFIG_KX132_TRIGGER_NONE, ( \
    printk("- MARK 8 -  main.c, testing IF_DEFINED generated gpio_dt_spec int_gpio_conditionally_compiled.port->name . . .\n"); \
    if ( int_gpio_conditionally_compiled.port != NULL ) \
    {  \
        printk("- MARK 9 - main.c, IF_ENABLED gen'd GPIO port name holds '%s',\n", int_gpio_conditionally_compiled.port->name); \
    } \
    else \
    { \
        printk("- MARK 9 - main.c, IF_ENABLED gen'd GPIO port found NULL!\n"); \
    } \
))


// - DEV 1128 - test of recently added port status data member:
    {
        const struct kx132_device_data *data = dev_kx132_1->data;
        printk("- DEV 1128 - main.c, driver gives drdy port status of %u\n", data->drdy_port_status);

        if ( data->drdy_port_status != DRDY_PORT_INITIALIZED )
        {
             printk("- DEV 1128 - main.c, requesting driver to reinit 'data ready' port . . .\n");
             requested_config.val1 = KX132_REINITIALIZE_DRDY_GPIO_PORT;
             requested_config.val2 = 0;

             sensor_api_status = sensor_attr_set(
                                                 dev_kx132_1,
                                                 SENSOR_CHAN_ALL,
                                                 SENSOR_ATTR_PRIV_START,
                                                 &requested_config
                                                );

            printk("- DEV 1128 - main.c, after reinit driver gives drdy port status of %u\n", data->drdy_port_status);
        }
        else
        {
            printk("- DEV 1128 - main.c, skipping reinit, drdy port already initialized\n");
        }
    }


    while ( 1 )
    {

        if ( dev_kx132_1 != NULL )
        {
            printk("- MARK 10 - main.c\n");

            if ( DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID )
            {
                printk("- MARK 11 - main.c, loop count %u\n", main_loop_count);

                sensor_sample_fetch_chan(dev_kx132_1, SENSOR_CHAN_KIONIX_MANUFACTURER_ID);
                sensor_channel_get(dev_kx132_1, SENSOR_CHAN_KIONIX_MANUFACTURER_ID, &value);

                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer %d\n",
                  value.val1);
                printk("%s", lbuf);
                snprintf(lbuf, sizeof(lbuf), "main.c - sensor_value.val2 holds %d\n", value.val2);
                printk("%s", lbuf);
                data_from_sensor.as_32_bit_integer = value.val1;

                printk("main.c - value.val1 as bytes:  ");
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), "0x%02X ", data_from_sensor.as_bytes[i]);
                    printk("%s", lbuf);
                }
#if 1
                printk("\n");
#else
                printk("  \"");
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), " %c ", data_from_sensor.as_bytes[i]);
                    printk("%s", lbuf);
                }
                snprintf(lbuf, sizeof(lbuf), "\"\n");
                printk("%s", lbuf);
#endif
            }

            if ( DEV_TEST__FETCH_AND_GET_PART_ID )
            {
                sensor_sample_fetch_chan(dev_kx132_1, SENSOR_CHAN_KIONIX_PART_ID);
                sensor_channel_get(dev_kx132_1, SENSOR_CHAN_KIONIX_PART_ID, &value);
                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor reports part ID of %d\n",
                  value.val1);
                printk("%s", lbuf);
            }


            if ( DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ )
            {
                sensor_api_status = sensor_sample_fetch_chan(dev_kx132_1, SENSOR_CHAN_ACCEL_XYZ);
                sensor_channel_get(dev_kx132_1, SENSOR_CHAN_ACCEL_XYZ, &value);
                snprintf(lbuf, sizeof(lbuf), "main.c - Kionix sensor x,y,z readings encoded:  0x%08x, 0x%08x\n\n",
                  value.val1, value.val2);
                printk("%s", lbuf);
            }
            else
            {
                printk("- skipping x,y,z readings fetch, to observe trigger line on o-scope\n");
            }


            {
//                printk("- -\n");
                uint8_t per_addr[4] = {0, 0, 0, 0};
                uint8_t *peripheral_reg_addr_ptr = per_addr;
                char data[40] = {0};
                char *data_ptr = data;
#define FOUR_BYTES (4)

                printk("- INFO (demo app) - testing SPI read registers routine:\n");
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, (FOUR_BYTES + 1));
                printk("- INFO (demo app) - over SPI bus read back manufacturer id string '%s'\n\n", data);
            }

        } 
        else 
        {
            snprintf(lbuf, sizeof(lbuf), "- WARNING - problem initializing KX132 Zephyr device pointer,\n");
            printk("%s", lbuf);
            snprintf(lbuf, sizeof(lbuf), "- WARNING + therefore not exercising features of this sensor.\n\n");
            printk("%s", lbuf);
        } // if dev_kx132_1 != NULL

        k_msleep(SLEEP_TIME_MS);
        ++main_loop_count;

    } // end while ( 1 )
}



