//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/kionix-driver-demo/tree/iis2dh-driver-integration-work-001
//
//      File:  thread_iis2dh.c
//
//----------------------------------------------------------------------

/*
 *  @Brief:  Zephyr thread to obtain and to store acceleration readings
 *     taken from STMicro IIS2DH sensor, using Zephyr in-tree driver.
 *
 *  @References:
 *
 *   +  https://docs.zephyrproject.org/2.6.0/reference/timing_functions/index.html?highlight=kernel%20timing
 *
 *   +  z4-sandbox-kionix-work/zephyr/samples/sensor/lis2dh/src/main.c
 *
 *   +  https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.7.0/kconfig/CONFIG_IIS2DH_ODR.html
 *
 *   +  https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
 */



//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

#define DT_DRV_COMPAT st_iis2dh

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // to provide memset(),

// Zephyr RTOS headers . . .
#include <zephyr.h>

// to provide Zephyr's device_get_binding():
#include <device.h>

#include <devicetree.h>
// NOTE:  following two headers are located in ${ZEPHYR_BASE}/include/drivers
#include <drivers/gpio.h>
#include <drivers/sensor.h>

// 2021-10-08 FRI - measurement timing implementation work
#include <timing/timing.h>
// 2021-10-08 FRI - 

// Work to utilize IIS2DH driver structures and routines from STMicro:
// ( Note https://github.com/STMicroelectronics/iis2dh )
#include <iis2dh.h>  // to include iis2dh_reg.h, to provide stmdev_ctx_t and related

// For tests of hand-rolled IIS2DH register config routines:
#include <drivers/i2c.h>



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

// defines thread related:
#define IIS2DH_THREAD_STACK_SIZE 1024
#define IIS2DH_THREAD_PRIORITY 7

// defines for application or task implemented by this thread:
#define SLEEP_TIME__IIS2DH_TASK__MS (6000)

// defines to connect with STMicro IIS2DH out-of-tree driver API:
//#define KX132_1211 DT_INST(0, kionix_kx132_1211)
#define ST_IIS2DH DT_INST(0, st_iis2dh)

//#define CONFIG_KX132_1211_DEV_NAME DT_LABEL(KX132_1211)
#define CONFIG_ST_IIS2DH_DEV_NAME DT_LABEL(IIS2DH)

// https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
#define IIS2DH_ACCELEROMETER DT_NODELABEL(stmicro_sensor)


//----------------------------------------------------------------------
// - SECTION - prototypes
//----------------------------------------------------------------------

void iis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3);


//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(iis2dh_thread_stack_area, IIS2DH_THREAD_STACK_SIZE);
struct k_thread iis2dh_thread_thread_data;



int initialize_thread_iis2dh_task(void)
{
    k_tid_t iis2dh_task_tid = k_thread_create(&iis2dh_thread_thread_data, iis2dh_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(iis2dh_thread_stack_area),
                                            iis2dh_thread_entry_point,
                                            NULL, NULL, NULL,
                                            IIS2DH_THREAD_PRIORITY,
                                            0,
                                            K_MSEC(1300)); // K_NO_WAIT);
    return (int)iis2dh_task_tid;
}


#define ROUTINE_OK 0
static uint32_t read_of_iis2dh_whoami_register(const struct device *dev, struct sensor_value value)
{
    int status = ROUTINE_OK;
    uint8_t cmd[] = { 0x0F };
    struct iis2dh_data *data_struc_ptr = (struct iis2dh_data *)dev->data;
    uint8_t scratch_pad_byte = 0;
//    uint8_t odcntl_as_found = 0;

    status = i2c_write_read(data_struc_ptr->bus,   // data_struc_ptr->i2c_dev,
                            DT_INST_REG_ADDR(0),
                            cmd, sizeof(cmd),
                            &scratch_pad_byte, sizeof(scratch_pad_byte));

    value.val1 = scratch_pad_byte;

    return status;
}

static uint32_t read_of_iis2dh_temperature_registers(const struct device *dev, struct sensor_value value)
{
    int status = ROUTINE_OK;
    uint8_t cmd[] = { 0x0C };
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;
    uint8_t scratch_pad_bytes[] = {0, 0};

    status = i2c_write_read(device_data_ptr->bus,   // data_struc_ptr->i2c_dev,
                            DT_INST_REG_ADDR(0),
                            cmd, sizeof(cmd),
                            &scratch_pad_bytes, sizeof(scratch_pad_bytes));

    value.val1 = ((scratch_pad_bytes[0] << 8) | scratch_pad_bytes[0]);

    return status;
}



// 2021-10-19 - Adding general Kionix driver 'write register' routine:

/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  @Description   Routine to write one eight-bit register per call,
 *                 in peripheral passed as 'device *dev'.
 *
 *  @Note          Peripheral device I2C adderss obtained from Zephyr
 *                 project macro at build time.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

static uint32_t kd_write_peripheral_register(const struct device *dev, const uint8_t* device_register_and_data)
{
    int status = ROUTINE_OK;
    int len = sizeof(device_register_and_data);
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;

    status = i2c_write(device_data_ptr->bus,
                       device_register_and_data,
                       len,
                       DT_INST_REG_ADDR(0)
                      );
    return status;
}



static uint8_t iis2dh_ctrl_reg1 = 0;
#define ODR_0_POWERED_DOWN             ( 0 << 4 )
#define ODR_1_HZ                       ( 1 << 4 )
#define ODR_10_HZ                      ( 2 << 4 )
#define ODR_25_HZ                      ( 3 << 4 )
#define ODR_50_HZ                      ( 4 << 4 )
#define ODR_100_HZ                     ( 5 << 4 )
#define ODR_200_HZ                     ( 6 << 4 )
#define ODR_400_HZ                     ( 7 << 4 )

#define ODR_5p376_HZ_IN_LOW_POWER_MODE ( 9 << 4 )
#define LOW_POWER_ENABLE               ( 1 << 3 )
#define AXIS_Z_ENABLE                  ( 1 << 2 )
#define AXIS_Y_ENABLE                  ( 1 << 1 )
#define AXIS_X_ENABLE                  ( 1 << 0 )

// Details of IIS2DH register 0x24 in iis2dh.pdf page 36 of 49, DocID027668 Rev 2 . . .
static uint8_t iis2dh_ctrl_reg4 = 0;
#define BLOCK_DATA_UPDATE_NON_CONTINUOUS        ( 1 << 7 )
#define BLE_LSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE  ( 0 << 6 )
#define BLE_MSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE  ( 1 << 6 )
#define ACC_FULL_SCALE_2G                       ( 0 << 4 ) 
#define ACC_FULL_SCALE_4G                       ( 1 << 4 )
#define ACC_FULL_SCALE_8G                       ( 2 << 4 )
#define ACC_FULL_SCALE_16G                      ( 3 << 4 )
// page 16 of 49:  low power means 8-bit readings, normal power means 10-bit readings, high-resolution means 12-bit readings
#define ACC_OPERATING_MODE_NORMAL                  ( 0 )
#define ACC_OPERATING_MODE_HIGH_RES                ( 1 )
#define ACC_OPERATING_MODE_LOW_POWER               ( 2 )
#define KX132_1211_SELF_TEST_NORMAL_MODE           ( 0 )
#define KX132_1211_SELF_TEST_0                     ( 1 )
#define KX132_1211_SELF_TEST_1                     ( 2 )
#define SPI_MODE_FOUR_WIRE                         ( 0 )
#define SPI_MODE_THREE_WIRE                        ( 1 )

static uint8_t iis2dh_ctrl_reg5 = 0;
#define FIFO_ENABLE ( 1 << 6 )

static uint8_t iis2dh_fifo_ctrl_reg = 0;
#define FIFO_CTRL_FM_BYPASS ( 0 << 6 )

static uint32_t kd_initialize_sensor_kx132_1211(const struct device *dev)
{

// Disable KX132-1211 FIFO:
    uint8_t cmd[] = { IIS2DH_CTRL_REG5, (iis2dh_ctrl_reg5 &= ~FIFO_ENABLE), 0 };
    kd_write_peripheral_register(dev, cmd);
// *** NEED to clear accelerator fifo overrun flag here ***

// Reset FIFO by briefly setting bypass mode:
    cmd[0] = IIS2DH_FIFO_CTRL_REG;  cmd[1] = FIFO_CTRL_FM_BYPASS; 
    kd_write_peripheral_register(dev, cmd);

// Set full scale (+/- 2g, 4g, 8g, 16g), normal versus high resolution, block update mode:
    cmd[0] = IIS2DH_CTRL_REG4;
    cmd[1] = ( 
               BLOCK_DATA_UPDATE_NON_CONTINUOUS
             | BLE_LSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE 
             | ACC_FULL_SCALE_8G
             | ACC_OPERATING_MODE_NORMAL
             | KX132_1211_SELF_TEST_NORMAL_MODE
             | SPI_MODE_THREE_WIRE
             );
    kd_write_peripheral_register(dev, cmd);

// Set data rate, normal or low-power (did we not do this above?), enable all three axes:
    cmd[0] = IIS2DH_CTRL_REG1;
    cmd[1] = (
               ODR_5p376_HZ_IN_LOW_POWER_MODE,
             | LOW_POWER_ENABLE
             | AXIS_Z_ENABLE
             | AXIS_Y_ENABLE
             | AXIS_X_ENABLE
             );
    kd_write_peripheral_register(dev, cmd);




    return 0;
}



void iis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3)
{

// --- VAR BEGIN ---
    int loop_count = 0;

// 2021-10-12 - keep assignment on this declarative line so it gets built at compile time:
    const struct device *sensor = DEVICE_DT_GET_ANY(st_iis2dh);
//    const struct device *sensor = device_get_binding(DT_LABEL(IIS2DH_ACCELEROMETER));

//    static uint16_t iis2ds12_i2c_slave_addr = DT_INST_REG_ADDR(0);   <-- 2021-10-17 not available at build time - TMH

    struct sensor_value acceleration_readings;
//#include <iis2dh_reg.h>

    uint8_t buff[10];
    memset(buff, 0, sizeof(buff));

// From LIS2DH sample app:
    int rc = 0;
    struct sensor_value accel[3];
    const char *overrun = "";

// DEV tests:
//    extern stmdev_ctx_t iis2dh_i2c_ctx;  // Should not need to declare data structure as 'extern'
                                         // now that we include iis2dh.h.
// 2021-10-16 - We're trying to send iis2dh device data parameter
// in the same way we see the lower level STMicro driver
// routines call with a pointer to this sensor type's data
// member element:
    struct iis2dh_data *iis2dh = sensor->data; // dev->data;
    uint32_t whoami_check_status = 0;
// Defined in [WEST_WORKSPACE]/modules/hal/st/sensor/stmemsc/iis2dh_STdC/driver/iis2dh_reg.h:716:} iis2dh_odr_t;
// (an enumeration with elements equal 0..9)
    iis2dh_odr_t sensor_data_rate = 0;

// --- VAR END ---


// Check whether device_get_binding() macro / call succeeded:
    if ( sensor == NULL )
        { printk("No iis2dh device found!\n"); }
    else
        { printk("Success finding iis2dh device,\n"); }

    if (!device_is_ready(sensor)) {
        printf("Device %s is not ready\n", sensor->name);
        return;
    }
    else
    {
        printf("Call to device_is_ready() says %s sensor is ready,\n", sensor->name);
    }

    printf("Device name found to be '%s'\n", sensor->name);

// 2021-10-19 Tuesday work to initialize Kionix sensor:
    rc = kd_initialize_sensor_kx132_1211(sensor);

/*
// From file zephyr/drivers/sensor/iis2dh/iis2dh.c:
222 static const struct sensor_driver_api iis2dh_driver_api = {
223         .attr_set = iis2dh_attr_set,
224 #if CONFIG_IIS2DH_TRIGGER
225         .trigger_set = iis2dh_trigger_set,
226 #endif / * CONFIG_IIS2DH_TRIGGER * /
227         .sample_fetch = iis2dh_sample_fetch,
228         .channel_get = iis2dh_channel_get,
229 };
*/

// Note, looks like IIS2DH sensor is already configured at compmile time,
// the initialization routine in IIS2DH in-tree driver is static and
// not available for us to call directly, nor available through the
// public facing API of this driver.

//    whoami_check_status = iis2dh_device_id_get(&iis2dh_i2c_ctx, buff);
    whoami_check_status = iis2dh_device_id_get(iis2dh->ctx, buff);

// Unsafe, unbounded print string type call:   - TMH
    printk("sensor who-am-i check returns '%s',\n", buff);
    printk("(However this value of three returned even when sensor not attached - need to investigate)\n");

    while (1)
    {
        printk("iis2dh task at loop iteration 0x%08X\n", loop_count);

// DEV TEST WHOAMI
        whoami_check_status = iis2dh_device_id_get(iis2dh->ctx, buff);

// Unsafe, unbounded print string type call:   - TMH
        printk("sensor who-am-i check returns '%s',\n", buff);
// DEV TEST WHOAMI


        struct sensor_value odr = {
                  .val1 = 1,
        };
{
 
        rc = sensor_attr_set(sensor,
                             SENSOR_CHAN_ACCEL_XYZ, //trig.chan,
                             5, // SENSOR_ATTR_SAMPLING_FREQUENCY,
                             &odr
                            );
        if (rc != 0) {
            printf("Failed to set odr: %d\n", rc);
//            return;

        printf("Sampling at %u Hz (assuming so as sensor_attr_set returns 0 status - success\n", odr.val1);
// 10/17 test:
// In [ZEPHYR_WORKSPACE]/modules/hal/st/sensor/stmemsc/iis2dh_STdC/driver/iis2dh_reg.c:
// 371 int32_t iis2dh_data_rate_set(stmdev_ctx_t *ctx, iis2dh_odr_t val)

            printf("- DEV001 - calling iis2dh_data_rate_set() directly...\n");
            uint32_t a = iis2dh_data_rate_set(iis2dh->ctx, 16);
            printf("got return value of %d,\n", a);

            printf("- DEV001 - calling iis2dh_data_rate_get()...\n");
            uint32_t b = 0;
            a = iis2dh_data_rate_get(iis2dh->ctx, (iis2dh_odr_t*)&b);
            printf("got return value of %d,\n", a);
            printf("read back data rate of value %d,\n", b);

            odr.val1 = 0;
            odr.val2 = 0;
            a = read_of_iis2dh_whoami_register(sensor, odr);
            printf("- DEV002 - hand-rolled whoami query returns manu' id of %u,\n", odr.val1);
            printf("- DEV002 - and routine status of %u,\n", a);

            a = read_of_iis2dh_temperature_registers(sensor, odr);
            printf("- DEV002 - local temperature query returns temperature of %u,\n", odr.val1);
            printf("- DEV002 - and routine status of %u,\n", a);

        }
}

        rc = sensor_attr_get(sensor,
                             SENSOR_CHAN_ACCEL_XYZ,
                             SENSOR_ATTR_SAMPLING_FREQUENCY,
                             &odr
                            );
        printf("sampling at %u Hz, per call to sensor_attr_get(),\n", odr.val1);


        rc = sensor_sample_fetch(sensor);

        if (rc == -EBADMSG) {
                /* Sample overrun.  Ignore in polled mode. */
                if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
                        overrun = "[OVERRUN] ";
                }
                rc = 0;
        }
        if (rc == 0) {
                rc = sensor_channel_get(sensor,
                                        SENSOR_CHAN_ACCEL_XYZ,
                                        accel);
        }
        if (rc < 0) {
                printf("ERROR: Update failed: %d\n", rc);
        } else {
                printf("#%u @ %u ms: %sx %f , y %f , z %f\n",
                       loop_count, k_uptime_get_32(), overrun,
                       sensor_value_to_double(&accel[0]),
                       sensor_value_to_double(&accel[1]),
                       sensor_value_to_double(&accel[2]));
        }


// *** review 6cd823d21eeaa begin ******************
        rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, &acceleration_readings);
        if ( rc == 0 )
        {
            printk("fetched and got acceleration readings %u, %u\n",
              acceleration_readings.val1,
              acceleration_readings.val2
            );
        }
        else
        {
            printk("ERROR getting acceleration readings, error is %u\n", rc);
        }
// *** review 6cd823d21eeaa end ******************


// TEST of getting sensor data rate:
//392:int32_t iis2dh_data_rate_get(stmdev_ctx_t *ctx, iis2dh_odr_t *val)

//        rc = iis2dh_data_rate_get(&iis2dh_i2c_ctx, &sensor_data_rate);
        rc = iis2dh_data_rate_get(iis2dh->ctx, &sensor_data_rate);
        if ( rc == 0 )
        {
            printk("sensor reports data rate set at %u,\n", (unsigned int)sensor_data_rate);
        }

        rc = iis2dh_data_rate_set(iis2dh->ctx, 8);
        if ( rc == 0 )
        {
            printk("routine to set output data rate returns %u,\n", rc);
        }

        printk("\n\n");
        k_msleep(SLEEP_TIME__IIS2DH_TASK__MS);
        loop_count++;

    } // end while (1) loop

}




// --- EOF ---
