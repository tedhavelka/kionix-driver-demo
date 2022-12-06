/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

#define SLEEP_TIME_MS 2000
#define DEFAULT_MESSAGE_SIZE 240

// Demo / early development tests:
#define DEV_TEST__FETCH_AND_GET_MANUFACTURER_ID    (1)
#define DEV_TEST__FETCH_AND_GET_PART_ID            (0)
#define DEV_TEST__FETCH_ACCELEROMETER_READINGS_XYZ (1)

// SPI related dev work:
#define BUILD_FOR_SPI_CONNECTED_IIS2DH (1)

//#define DEV_SHOW_FIRST_BYTES_SPI_RX_BUFFER

#define DEV_TEST__ENABLE_KX132_1211_ASYNCHRONOUS_READINGS      (0)
#define DEV_TEST__KX132_ENABLE_SYNC_READINGS_WITH_HW_INTERRUPT (1)
#define DEV_TEST__SET_KX132_1211_OUTPUT_DATA_RATE              (0)

#define SPI_MSBIT_AUTOINC_REG_ADDR_SET     1
#define SPI_MSBIT_AUTOINC_REG_ADDR_CLEARED 0
#define SPI_MSBIT_SET     SPI_MSBIT_AUTOINC_REG_ADDR_SET
#define SPI_MSBIT_CLEARED SPI_MSBIT_AUTOINC_REG_ADDR_CLEARED



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



static uint32_t read_registers(const struct device *dev,       // Zephyr pointer to peripheral or sensor device
                               const uint8_t* device_register, // peripheral register address in small array
                               uint8_t* data,                  // buffer to hold data we read from peripheral
                               uint8_t len,                    // count of bytes to read from peripheral
                               uint8_t option)                 // in SPI used to select auto-inc periph addr or not
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

    if ( option == SPI_MSBIT_SET )
    {
        spi_tx_buffer[0] = (sensor_register_addr |= 0xC0);  // 0x80 = logical OR with 0b10000000, read bit true, MS bit true
    }
    else
    {
        spi_tx_buffer[0] = (sensor_register_addr |= 0x80);  // 0x80 = logical OR with 0b10000000, read bit true, MS bit false
    }

    tx_buf.len = 2;
    rx_buf.len = (len + 1);

#if 0
printk("- read_registers() - ready to read %u bytes from register 0x%02x,\n", rx_buf.len, (spi_tx_buffer[0] & 0x3f));
#endif

    rstatus = spi_transceive(
                             cfg->spi.bus,
                             &cfg->spi.config,
                             &tx_set,
                             &rx_set
                            );

#if 0
printk("- read_registers() - after spi_transceive() call rx_buf holds { 0x%02X, 0x%02X }\n", spi_rx_buffer[0], spi_rx_buffer[1]);
#endif

    memcpy(data, &spi_rx_buffer[1], (size_t)len);

#ifdef DEV_SHOW_FIRST_BYTES_SPI_RX_BUFFER
    printk("- DEV 1113 - first few bytes read back via spi_transceive():  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
      spi_rx_buffer[0], spi_rx_buffer[1], spi_rx_buffer[2], spi_rx_buffer[3], spi_rx_buffer[4]);
#endif

    return rstatus;
}



static uint32_t write_registers(const struct device *dev,       // Zephyr pointer to peripheral or sensor device
                               const uint8_t* device_register,  //
                               uint8_t* data,                   // data to write to peripheral
                               uint8_t len,                     // count of bytes to write
                               uint8_t option)                  // in SPI used to select auto-inc periph addr or not
{
    uint8_t reg = device_register[0];
    const struct kx132_device_config *cfg = dev->config;
    uint32_t rstatus = 0;


#if 0
     spi_tx_buffer[0] = device_register[0];
     spi_tx_buffer[1] = data[0];
 
     tx_buf.len = 2; rx_buf.len = 2;

// struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };
    rx_set.buffers = 0;
    rx_set.count = 1;

printk("- write_registers() - ready to write %u bytes to register 0x%02x,\n", tx_buf.len, (spi_tx_buffer[0] & 0x3f));
printk("- write_registers() - tx_buf holds { 0x%02X, 0x%02X }\n", spi_tx_buffer[0], spi_tx_buffer[1]);

    rstatus = spi_transceive(
                             cfg->spi.bus,
                             &cfg->spi.config,
                             &tx_set,
                             &rx_set
                            );

#else
#define KX132_SPI_WRITEM  (1 << 6) /* 0x40 */

//	uint8_t buffer_tx[1] = { reg | KX132_SPI_WRITEM };  // <-- this OR'ing of 0b01000000 may break comm's with KX132 sensor
	uint8_t buffer_tx[1] = { reg };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};

printk("- write_register() - set up SPI tx buffers IIS2DH way,\n");
//printk("- write_register() - tx_buf[0].buf[0] holds register addr 0x%02x,\n", tx_buf[0].((uint8_t *)(buf[0])) );
//                                                                                       ^^^^^^^^^
printk("- write_register() - only one of buffer_tx holds { 0x%02x },\n", buffer_tx[0]);
printk("- write_register() - first two of data hold { 0x%02x, 0x%02x },\n", data[0], data[1]);
printk("\n");


//	if (spi_write_dt(&config->spi, &tx)) {
//		return -EIO;
//	}
	rstatus = spi_write_dt(&cfg->spi, &tx);
#endif

    return rstatus;
}



static uint32_t update_output_data_rate(const struct device *dev)
{
    uint32_t rstatus = 0;
// These two local arrays get copied to file-scoped spi_tx_buff and spi_rx_buff arrays:
    uint8_t data_to_write[2] = { 0, 0 };
    uint8_t data_to_read[2] = { 0, 0 };


    printk("- 1205 update_output_data_rate() - about to read KX132 register with addr 0x%02x . . .\n", KX132_ODCNTL);
    data_to_read[0] = KX132_ODCNTL;
    read_registers(dev, data_to_read, &data_to_read[1], 1, SPI_MSBIT_CLEARED);
    printk("- 1205 update_output_data_rate() - ODR register 0x%02x holds %u\n", data_to_read[0], data_to_read[1]);
    printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    printk("- 1205 update_output_data_rate() - updating ODR register with value of 8 . . .\n");
    data_to_write[0] = KX132_ODCNTL;
    data_to_write[1] = 0x08;
    write_registers(dev, data_to_write, &data_to_write[1], 1, SPI_MSBIT_CLEARED);

    read_registers(dev, data_to_read, &data_to_read[1], 1, SPI_MSBIT_CLEARED);
    printk("- 1205 update_output_data_rate() - ODR register 0x%02x now holds %u\n", data_to_write[0], data_to_read[1]);
    printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    printk("- 1205 update_output_data_rate() - updating ODR register with value of 6, ** MSBIT set ** . . .\n");
    data_to_write[0] = KX132_ODCNTL;
    data_to_write[1] = 0x06;
    write_registers(dev, data_to_write, &data_to_write[1], 1, SPI_MSBIT_SET); // SPI_MSBIT_CLEARED);

//
// 2022-12-05 Note, writes succeed whether MSBIT set or cleared - TMH
//

    read_registers(dev, data_to_read, &data_to_read[1], 1, SPI_MSBIT_CLEARED);
    printk("- 1205 update_output_data_rate() - ODR register 0x%02x now holds %u\n", data_to_write[0], data_to_read[1]);
    printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

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
	if ( 1 ) {
            printk("- main.c - calling sensor_trigger_set() . . .\n");
            trig.type = SENSOR_TRIG_DATA_READY;
            trig.chan = SENSOR_CHAN_ACCEL_XYZ;
            rstatus = sensor_trigger_set(dev_kx132_1, &trig, trigger_handler);
            printk("- main.c - call to sensor_trigger_set() returns status value of %d\n", rstatus);

            rstatus |= kx132_trigger_set(dev_kx132_1, &trig, trigger_handler);
            printk("- main.c - call to kx132_trigger_set() returns status value of %d\n", rstatus);
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
    if ( 0 )
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


// - DEV 1205 --
    printk("\n\n- DEV 1205 - main.c, testing stub for local SPI writes . . .\n");
    rstatus = update_output_data_rate(dev_kx132_1);
    printk("\nFurther tests:\n");

    uint8_t data_to_read[2] = { 0, 0 };

    data_to_read[0] = KX132_INC1;
    read_registers(dev_kx132_1, data_to_read, &data_to_read[1], 1, SPI_MSBIT_CLEARED);
    printk("- main() - INC1 register 0x%02x holds %u\n", data_to_read[0], data_to_read[1]);

    data_to_read[0] = KX132_CNTL1;
    read_registers(dev_kx132_1, data_to_read, &data_to_read[1], 1, SPI_MSBIT_CLEARED);
    printk("- main() - CNTL1 register 0x%02x holds %u\n\n\n", data_to_read[0], data_to_read[1]);

#if 0
    printk("dev app exiting early . . .\n\n\n");
    return;
// ZZZZZ the end
#endif


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
#if 0
                printk("\n");
#else
                printk("  \"");
                for ( i = 0; i < sizeof(int); i++ )
                {
                    snprintf(lbuf, sizeof(lbuf), " %c ", data_from_sensor.as_bytes[i]);
                    printk("%s", lbuf);
                }
                snprintf(lbuf, sizeof(lbuf), "\"\n\n");
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
                char data[40] = {0, 0, 0, 0, 0};
                char *data_ptr = data;
#define FOUR_BYTES (4)  // length of KX132 manufacturer id string 'Kion'
#define SIX_BYTES (6)   // count of bytes to hold 16 bit x, y, z accel readings

                printk("- INFO (demo app) - testing SPI read registers routine:\n");
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, (FOUR_BYTES + 1), SPI_MSBIT_CLEARED);
                printk("- INFO (demo app) - over SPI bus read back manufacturer id string '%s'\n\n", data);

                per_addr[0] = 8;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, (SIX_BYTES + 1), SPI_MSBIT_CLEARED);
                snprintf(lbuf, sizeof(lbuf), "main.c - x,y,z readings from local SPI read dev routine:  0x%04x, 0x%04x, 0x%04x\n",
                  (data[0]*16+data[1]), (data[2]*16+data[3]), (data[4]*16+data[5]));
                printk("%s", lbuf);

                uint8_t readings[6] = {0,0,0,0,0,0};
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[0] = data[0];

                per_addr[0] = 9;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[1] = data[0];

                per_addr[0] = 10;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[2] = data[0];

                per_addr[0] = 11;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[3] = data[0];

                per_addr[0] = 12;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[4] = data[0];

                per_addr[0] = 13;
                rstatus = read_registers(dev_kx132_1, peripheral_reg_addr_ptr, data_ptr, 1, SPI_MSBIT_CLEARED);
                readings[5] = data[0];

                snprintf(lbuf, sizeof(lbuf), "main.c - x,y,z readings via six SPI transactions: . . . . 0x%04x, 0x%04x, 0x%04x\n\n\n",
                  (readings[0]*16+readings[1]), (readings[2]*16+readings[3]), (readings[4]*16+readings[5]));
                printk("%s", lbuf);
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



