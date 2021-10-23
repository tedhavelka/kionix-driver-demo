//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/tedhavelka/kionix-driver-demo
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
 *   +  [west_workspace]/zephyr/samples/sensor/lis2dh/src/main.c
 *
 *   +  https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.7.0/kconfig/CONFIG_IIS2DH_ODR.html
 *
 *   +  https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
 *
 *   +  [west_workspace]/modules/hal/st/sensor/stmemsc/iis2dh_STdC/driver/iis2dh.h, iis2dh.c
 */



//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

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

#include "iis2dh-registers.h"



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

// To provide for device specific Zephyr macros, some or all of which
// generated at project build time:

#define DT_DRV_COMPAT st_iis2dh


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


#define ROUTINE_OK 0  // <-- NEED TO PULL IN LOCAL PROJECT HEADER WITH ENUMERATION OF ROUTINE RETURN VALUES - TMH



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



//----------------------------------------------------------------------
// General I2C multi-byte write and read routines:
//----------------------------------------------------------------------

/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  @Description   Wrapper function around Zephyr RTOS 2.6.0 I2C write
 *                 API function.  In place in part to prepare for
 *                 a more flexible IC2/SPI/other-protocol wrapper
 *                 framework in the future.
 *
 *  @Note          Peripheral device I2C adderss obtained from Zephyr
 *                 project macro at build time.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

static uint32_t kd_write_peripheral_register(const struct device *dev,
                                             const uint8_t* device_register_and_data,
                                             const uint32_t count_bytes_to_write)
{
    int status = ROUTINE_OK;
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;

    status = i2c_write(device_data_ptr->bus,
                       device_register_and_data,
                       count_bytes_to_write,
                       DT_INST_REG_ADDR(0)
                      );
    return status;
}



/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  #Note:  following wrapper to Zephyr RTOS I2C read function
 *   expects caller to provide count of bytes to read from passed
 *   peripheral device.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

static uint32_t kd_read_peripheral_register(const struct device *dev,
                                            const uint8_t* device_register,
                                            uint8_t* data,
                                            const uint8_t count_bytes_to_read)
{
    int status = ROUTINE_OK;
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;

    printk("kd_read - asked to read %u bytes,\n", count_bytes_to_read);

    status = i2c_write_read(
                         device_data_ptr->bus,
                         DT_INST_REG_ADDR(0),
                         device_register, sizeof(device_register),
                         data,
                         count_bytes_to_read
                        );
    return status;
}




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


/*
 *  @Note:  this routine returns an 8-bit register value to caller.
 */

static uint8_t read_of_iis2dh_acc_status_register(const struct device *dev)
{
    int status = ROUTINE_OK;
    uint8_t cmd[] = { IIS2DH_STATUS_REG };
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;
    uint8_t acc_status_register_value = 0;

    status = i2c_write_read(device_data_ptr->bus,
                            DT_INST_REG_ADDR(0),
                            cmd, sizeof(cmd),
                            &acc_status_register_value, 1
                           );

    return acc_status_register_value;
}


/*
 *  @Note:  assign values to several iis2dh registers, for data acquisition.
 */

static uint32_t accelerator_start_acquisition_no_fifo(const struct device* dev, const uint8_t output_data_rate)
{
    uint8_t cmd[] = { 0, 0, 0 };
    uint8_t register_value = 0;
    uint32_t rstatus = 0;  // status of this routine, OR'd sum of register read and write calls



// Prepare accelerometer for data acquisition without FIFO:

// (1) Disable IIS2DH FIFO:
// [ REBOOT | FIFO_EN |   --   |   --   | LIR_INT1 | D4D_INT1 | LIR_INT2 | D4D_INT2 ]  <-- IIS2DH_CTRL_REG5

    cmd[0] = IIS2DH_CTRL_REG5;
    iis2dh_ctrl_reg5 &= ~(FIFO_ENABLE);
    cmd[1] = iis2dh_ctrl_reg5;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);  // magic number '2' here refers to reg' addr and value to write

// (2) Reset FIFO by briefly setting bypass mode:
// [   FM1  |   FM0   |   TR   |  FTH4  |   FTH3   |   FTH2   |   FTH1   |   FTH0   ]  <-- IIS2DH_FIFO_CTRL_REG

    cmd[0] = IIS2DH_FIFO_CTRL_REG;
    cmd[1] = FIFO_MODE_BYPASS; 
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

// (3) Set full scale (+/- 2g, 4g, 8g, 16g), normal versus high resolution, block update mode:
// [   BDU  |   BLE   |   FS1  |   FS0  |    HR    |    ST1   |    ST0   |    SIM   ]  <-- IIS2DH_CTRL_REG4

    cmd[0] = IIS2DH_CTRL_REG4;
    cmd[1] = ( 
               BLOCK_DATA_UPDATE_NON_CONTINUOUS         //
             | BLE_LSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE   // BLE Big | Little Endian high res readings storage
             | ACC_FULL_SCALE_2G                        // 2G, 4G, 8G, 16G
             | POWER_MODE                               // low power, normal, high-resolution modes (see table 9 for cross reg' mut exc setting)
             | IIS2DH_SELF_TEST_NORMAL_MODE             // normal (no test), test 0, test 1
             | SPI_MODE_THREE_WIRE                      // 3-wire | 4-wire
             );
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

// (4) Set data rate, enable accelerometer axes x, y, z:
// [  ODR3  |   ODR2  |  ODR1  |  ODR1  |   LPEN   |    ZEN   |    YEN   |    XEN   ]  <-- IIS2DH_CTRL_REG1

    cmd[0] = IIS2DH_CTRL_REG1;
    cmd[1] = (
               output_data_rate                         //
             | LOW_POWER_ENABLE                         // when low power enabled, high resolution readings not available.  iis2dh.pdf page 16.
             | AXIS_Z_ENABLE
             | AXIS_Y_ENABLE
             | AXIS_X_ENABLE
             );
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);



} // end routine accelerator_start_acquisition_no_fifo()







//
//----------------------------------------------------------------------
// - SECTION - int main or entry point
//----------------------------------------------------------------------
//

void iis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3)
{

// --- VAR BEGIN ---
    int loop_count = 0;

// 2021-10-12 - keep assignment on this declarative line so it gets built at compile time:
    const struct device *sensor = DEVICE_DT_GET_ANY(st_iis2dh);
//    const struct device *sensor = device_get_binding(DT_LABEL(IIS2DH_ACCELEROMETER));

    static uint16_t iis2ds12_i2c_periph_addr = DT_INST_REG_ADDR(0);   //<-- 2021-10-17 not available at build time - TMH

    struct sensor_value acceleration_readings;

    uint8_t buff[10];
    memset(buff, 0, sizeof(buff));

// From LIS2DH sample app:
    int rc = 0;
    struct sensor_value odr;
//    struct sensor_value accel[3];
//    const char *overrun = "";

// DEV tests:
//    extern stmdev_ctx_t iis2dh_i2c_ctx;  // Should not need to declare data structure as 'extern'
                                         // now that we include iis2dh.h.
// 2021-10-16 - We're trying to send iis2dh device data parameter
// in the same way we see the lower level STMicro driver
// routines call with a pointer to this sensor type's data
// member element:
    struct iis2dh_data *iis2dh = sensor->data; // dev->data;
    uint32_t whoami_check_status = 0;
// Defined in [WEST_WORKSPACE]/modules/hal/st/sensor/stmemsc/iis2dh_STdC/driver/iis2dh_reg.h:716: <<closing_curly_brace>> iis2dh_odr_t;
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
//    rc = kd_initialize_sensor_kx132_1211(sensor);

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



    while (1)
    {
        printk("iis2dh task at loop iteration 0x%08X\n IIS2DH sensor at I2C addr %02X\n",
          loop_count, iis2ds12_i2c_periph_addr);

// BLOCK BEGIN
        if ( (loop_count % 2) == 0 )
            { odr.val1 = 5; }
        else
            { odr.val1 = 6; }

// A Zephyr mid-level IIS2DH driver call, effectively a wrapper to STMicro modules/hal driver code:
        rc = sensor_attr_set(sensor,
                             SENSOR_CHAN_ACCEL_XYZ, //trig.chan,
                             SENSOR_ATTR_SAMPLING_FREQUENCY,
                             &odr
                            );
        if (rc != 0) {
            printk("Failed to set odr: %d\n", rc);
        }
        else
        {
            printk("- DEV003 - successfully updated ODR at runtime!\n");
        }
// BLOCK END


// 10/17 test:
// In [ZEPHYR_WORKSPACE]/modules/hal/st/sensor/stmemsc/iis2dh_STdC/driver/iis2dh_reg.c:
// 371 int32_t iis2dh_data_rate_set(stmdev_ctx_t *ctx, iis2dh_odr_t val)


        rc = sensor_attr_get(sensor,
                             SENSOR_CHAN_ACCEL_XYZ,
                             SENSOR_ATTR_SAMPLING_FREQUENCY,
                             &odr
                            );
        printf("sampling at Output Data Rate (ODR) setting %u,\n", odr.val1);


// *** review 6cd823d21eeaa begin ******************
        k_msleep(20);
        printf("- DEV004 - fetching readings just after 20ms delay...\n");
        rc = sensor_sample_fetch(sensor);

        printf("- DEV004 - getting X axis reading only...\n");
        rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_X, &acceleration_readings);
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


        printk("\n\n");
        k_msleep(SLEEP_TIME__IIS2DH_TASK__MS);
        loop_count++;

    } // end while (1) loop

} // end routine iis2dh_thread_entry_point




// --- EOF ---
