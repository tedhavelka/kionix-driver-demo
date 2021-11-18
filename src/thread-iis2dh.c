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
// - SECTION - TO DO
//----------------------------------------------------------------------

/*

   [ ] TO DO 2021-11-17 - Bring default Output Data Rate value out as pound define.

   [ ] TO DO 2021-11-17 - Replace calls to printk() with call to diagnostic wrapper function dmsg() where appropriate.

   [ ] TO DO 2021-11-17 - Code clean up in '_thread_entry_point' routine.

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

// Local-to-project headers:
#include "kd-app-config.h"
#include "return-values.h"
#include "module-ids.h"
#include "development-flags.h"

#include "conversions.h"
#include "scoreboard.h"
#include "iis2dh-registers.h"

#if KD_DEV__CLI_DIAG_ON_IN_IIS2DH_TASK
#include "thread-simple-cli.h"
#endif



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

//
// Zephyr Device Tree related:
// 

// To provide for device specific Zephyr macros, some or all of which
// generated at project build time:
#define DT_DRV_COMPAT st_iis2dh

// defines to connect with STMicro IIS2DH out-of-tree driver API:
//#define KX132_1211 DT_INST(0, kionix_kx132_1211)
#define ST_IIS2DH DT_INST(0, st_iis2dh)

//#define CONFIG_KX132_1211_DEV_NAME DT_LABEL(KX132_1211)
#define CONFIG_ST_IIS2DH_DEV_NAME DT_LABEL(IIS2DH)

// https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
#define IIS2DH_ACCELEROMETER DT_NODELABEL(stmicro_sensor)


//
// defines thread related:
//

#define IIS2DH_THREAD_STACK_SIZE 1024
#define IIS2DH_THREAD_PRIORITY 7

// defines for application or task implemented by this thread:
#define SLEEP_TIME__IIS2DH_TASK__MS (2000)


//
// Sensor related:
//

#define COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER (1)

#ifndef KD_APP_DEFAULT_IIS2DH_OUTPUT_DATA_RATE
#warning "IIS2DH default data rate found first in IIS2DH thread source file,"
#warning "using this start up ODR value of:  " KD_APP_DEFAULT_IIS2DH_OUTPUT_DATA_RATE
#define KD_APP_DEFAULT_IIS2DH_OUTPUT_DATA_RATE ODR_100_HZ
#endif



//----------------------------------------------------------------------
// - SECTION - prototypes
//----------------------------------------------------------------------

void iis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3);



//----------------------------------------------------------------------
// - SECTION - file scoped variables, arrays, structures
//----------------------------------------------------------------------

#define BYTES_PER_XYZ_READINGS_TRIPLET (6)
#define FIFO_READINGS_MAXIMUM_COUNT (32)
static uint8_t readings_data[BYTES_PER_XYZ_READINGS_TRIPLET * (FIFO_READINGS_MAXIMUM_COUNT - 1)];
#define TRIPLETS_TO_FORMAT_PER_LINE (4)


#if 1
// Possible run-time copies of sensor configuration register settings:
static uint8_t iis2dh_temp_cfg_reg = 0;    // 0x1F
// static uint8_t iis2dh_ctrl_reg1 = 0;       // 0x20
// static uint8_t iis2dh_ctrl_reg2 = 0;       // 0x21
static uint8_t iis2dh_ctrl_reg3 = 0;       // 0x22
static uint8_t iis2dh_ctrl_reg4 = 0;       // 0x23
static uint8_t iis2dh_ctrl_reg5 = 0;       // 0x24
static uint8_t iis2dh_acc_status = 0;      // 0x27
static uint8_t iis2dh_fifo_ctrl_reg = 0;   // 0x2F
#endif

//static uint32_t iis2dh_thread_sleep_time_in_ms;

// Not fully implemented, meant to track larger sets of time contiguous reagings from FIFO:
static uint32_t running_total_xyz_readings = 0;


//
// --- FIFO overrun related BEGIN ---
struct fifo_overrun_event
{
    uint32_t reading_index;
};

#define MAX_OVERRUNS_TRACKED (50)
struct fifo_overrun_event fifo_overrun_events[MAX_OVERRUNS_TRACKED];

static uint32_t fifo_overrun_count_fsv = 0;
// --- FIFO overrun related END ---
//


// 2021-11-17 - *sensor needed at file scope for new public API routines:
const struct device *sensor = DEVICE_DT_GET_ANY(st_iis2dh);


#if KD_DEV__CLI_ONE_SHOT_MESSAGE_FLAG == 1
static uint32_t flag_one_shot_diag_message_enabled = 0;
#endif



//----------------------------------------------------------------------
// - SECTION - routines development
//----------------------------------------------------------------------

// Note, showing FIFO overrun event counts and details is development in
//  nature, but tracking and responding to them at run time is most
//  likely real life use case, e.g. production use:

void show_fifo_overruns_summary(void)
{
    uint32_t i = 0;

    printk("FIFO overrun events noted from latest x,y,z readings set:\n\n");

    for ( i = 0; i < fifo_overrun_count_fsv; i++ )
    {   
        printk("  FIFO overrun %u @ reading %u\n", i, fifo_overrun_events->reading_index);
    }   
    printk("\n");
}


#if KD_DEV__CLI_ONE_SHOT_MESSAGE_FLAG == 1
void dev__thread_iis2dh__set_one_shot_message_flag(void)
{
    flag_one_shot_diag_message_enabled = 1;
}
#endif




//----------------------------------------------------------------------
// - SECTION - routines production this module
//----------------------------------------------------------------------

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(iis2dh_thread_stack_area, IIS2DH_THREAD_STACK_SIZE);
struct k_thread iis2dh_thread_data;

int initialize_thread_iis2dh_task(void)
{
    int rstatus = 0;

    k_tid_t iis2dh_task_tid = k_thread_create(&iis2dh_thread_data, iis2dh_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(iis2dh_thread_stack_area),
                                            iis2dh_thread_entry_point,
                                            NULL, NULL, NULL,
                                            IIS2DH_THREAD_PRIORITY,
                                            0,
                                            K_MSEC(1300)); // K_NO_WAIT);

// REF https://docs.zephyrproject.org/2.6.0/reference/kernel/threads/index.html?highlight=k_thread_create#c.k_thread_name_set
// int k_thread_name_set(k_tid_t thread, const char *str)
//    rstatus = k_thread_name_set(iis2dh_task_tid, "kd_thread_iis2dh");
    rstatus = k_thread_name_set(iis2dh_task_tid, MODULE_ID__THREAD_IIS2DH);
    if ( rstatus == 0 ) { } // avoid compiler warning about unused variable - TMH

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

    status = i2c_write_read(
                         device_data_ptr->bus,
                         DT_INST_REG_ADDR(0),
                         device_register, sizeof(device_register),
                         data,
                         count_bytes_to_read
                        );

#if KD_DEV__CLI_DIAG_ON_REGISTER_READS == 1
    if ( flag_one_shot_diag_message_enabled == 1 )
    {
        flag_one_shot_diag_message_enabled = 0;
        char lbuf[SIZE_OF_MESSAGE_SHORT];
        snprintf(lbuf, SIZE_OF_MESSAGE_SHORT, "-\n\r- DEV 1118 %s - called to read %u bytes from reg 0x%02X\n\r",
          MODULE_ID__THREAD_IIS2DH, count_bytes_to_read, *device_register);
        printk_cli(lbuf);
        snprintf(lbuf, SIZE_OF_MESSAGE_SHORT, "- DEV 1118 %s - reg 0x%02X holds %u\n\r-\n\r",
//          MODULE_ID__THREAD_IIS2DH, (uint32_t)device_register, (uint32_t)data);
//          MODULE_ID__THREAD_IIS2DH, (uint32_t)(*device_register & 0xFF), (uint32_t)(*data & 0xFF));
          MODULE_ID__THREAD_IIS2DH, (*device_register & 0xFF), (*data & 0xFF));
        printk_cli(lbuf);
    }
#endif

    return status;
}



#if 0
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
#endif


static uint32_t configure_iis2dh_temperature_enable(const struct device *dev)
{
    uint32_t rstatus = ROUTINE_OK;
    uint8_t cmd[] = { 0, 0, 0 };
//    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;

#define DEV_TEMPERATURE_READINGS 1
#if DEV_TEMPERATURE_READINGS == 1
// iis2dh_temp_cfg_reg
// iis2dh_ctrl_reg4
    cmd[0] = TEMP_CONFIG_REGISTER;
    rstatus |= kd_read_peripheral_register(dev, cmd, &iis2dh_temp_cfg_reg, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);
    printk("- DEV 1117 - top of routine before config TEMP_CONFIG_REGISTER holds %u,\n",
      iis2dh_temp_cfg_reg);
#endif

    cmd[0] = TEMP_CONFIG_REGISTER;
    iis2dh_temp_cfg_reg = ( TEMP_ENABLE_1 | TEMP_ENABLE_0 );
    cmd[1] = iis2dh_temp_cfg_reg;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);
#if DEV_TEMPERATURE_READINGS == 1
    printk("- DEV 1117 - after config TEMP_CONFIG_REGISTER holds %u,\n",
      iis2dh_temp_cfg_reg);
#endif

    cmd[0] = IIS2DH_CTRL_REG4;
// NEED to read control register 4 . . .
    rstatus |= kd_read_peripheral_register(dev, cmd, &iis2dh_ctrl_reg4, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);
#if DEV_TEMPERATURE_READINGS == 1
    printk("- DEV 1117 - before config IIS2DH_CTRL_REG4 holds %u,\n",
      iis2dh_ctrl_reg4);
#endif
    iis2dh_ctrl_reg4 |= BLOCK_DATA_UPDATE_NON_CONTINUOUS;
    cmd[1] = iis2dh_ctrl_reg4;
#if DEV_TEMPERATURE_READINGS == 1
    printk("- DEV 1117 - after config IIS2DH_CTRL_REG4 holds %u,\n",
      iis2dh_ctrl_reg4);
#endif
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

    return rstatus;
}



static uint32_t read_of_iis2dh_temperature_registers(const struct device *dev, struct sensor_value *value)
{
    uint32_t rstatus = ROUTINE_OK;
//    uint8_t cmd[] = { OUT_TEMP_L };
    uint8_t cmd[] = { 0, 0 };
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;
    uint8_t scratch_pad_bytes[] = {0, 0};

    char lbuf[DEFAULT_MESSAGE_SIZE];

#if KD_DEV__SET_BDU_BEFORE_TEMP_READING_THEN_UNSET == 1
    cmd[0] = IIS2DH_CTRL_REG4;
    printk("- DEV 1117 - reading iis2dh control register %u . . .\n", IIS2DH_CTRL_REG4);
    rstatus |= kd_read_peripheral_register(dev, cmd, &iis2dh_ctrl_reg4, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);

    iis2dh_ctrl_reg4 |= BLOCK_DATA_UPDATE_NON_CONTINUOUS;
    printk("- DEV 1117 - setting iis2dh control register 4 to %u,\n", iis2dh_ctrl_reg4);
    cmd[1] = iis2dh_ctrl_reg4;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);  // magic number '2' here refers to reg' addr and value to write
#endif

    rstatus = i2c_write_read(device_data_ptr->bus,   // data_struc_ptr->i2c_dev,
                             DT_INST_REG_ADDR(0),
                             cmd, 1,
                             &scratch_pad_bytes, 2
                            );

    value->val1 = ((scratch_pad_bytes[0] << 8) | scratch_pad_bytes[0]);

    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "-\n- %s - IIS2DH reports temperature of %u C\n-\n",
      MODULE_ID__THREAD_IIS2DH, value->val1);
    dmsg(lbuf, DIAG_NORMAL);

#if KD_DEV__SET_BDU_BEFORE_TEMP_READING_THEN_UNSET == 1
    iis2dh_ctrl_reg4 &= ~(BLOCK_DATA_UPDATE_NON_CONTINUOUS);
    printk("- DEV 1117 - setting iis2dh control register 4 to %u after temperature reading,\n", iis2dh_ctrl_reg4);
    cmd[1] = iis2dh_ctrl_reg4;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);  // magic number '2' here refers to reg' addr and value to write
#endif

    return rstatus;
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


static uint8_t read_of_iis2dh_acc_fifo_src_register(const struct device *dev)
{
    int status = ROUTINE_OK;
    uint8_t cmd[] = { IIS2DH_FIFO_SRC_REG };
    struct iis2dh_data *device_data_ptr = (struct iis2dh_data *)dev->data;
    uint8_t acc_fifo_src_register_value = 0;

    status = i2c_write_read(device_data_ptr->bus,
                            DT_INST_REG_ADDR(0),
                            cmd, sizeof(cmd),
                            &acc_fifo_src_register_value, 1
                           );

    return acc_fifo_src_register_value;
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

// Note, we could configure high pass filter here.
// Note, we could configure FIFO operation mode here.
// Note, we could configure one or more interrupts here.

// Whether using interrupts or not we'll clear them here per example code:
    cmd[0] = IIS2DH_CTRL_REG3;
    cmd[1] = 0;
    rstatus |= kd_read_peripheral_register(dev, cmd, &register_value, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);
    printk("Clearing any interrupts we read from CTRL_REG3:  %u\n", register_value);

// In another IIS2DH configuration that utilizes FIFO we would enable FIFO here.

    return rstatus;

} // end routine accelerator_start_acquisition_no_fifo()



static uint32_t accelerator_start_acquisition_with_fifo(const struct device* dev, const uint8_t output_data_rate)
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

// Note, we could configure high pass filter here.

// (6) Set FIFO mode to stream, and FIFO trigger threshhold:
    cmd[0] = IIS2DH_FIFO_CTRL_REG;
    cmd[1] = ( 
               FIFO_MODE_STREAM                         // see iis2dh.pdf table 48
             | FIFO_TRIGGER_ON_INT_1 
             | FIFO_TRIGGER_THRESHHOLD
             );
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);


// Note, we could configure one or more interrupts here.

// Whether using interrupts or not we'll clear them here per example code:
    cmd[0] = IIS2DH_CTRL_REG3;
    cmd[1] = 0;
    rstatus |= kd_read_peripheral_register(dev, cmd, &register_value, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);
    printk("Clearing any interrupts we read from CTRL_REG3:  %u\n", register_value);

// (9) Enable FIFO:
    cmd[0] = IIS2DH_CTRL_REG5;
    iis2dh_ctrl_reg5 |= FIFO_ENABLE;
//printk("-- DEV006 -- control register 5 shadow holds 0x%02X, should be decimal 64.\n", iis2dh_ctrl_reg5);
    cmd[1] = iis2dh_ctrl_reg5;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

    return rstatus;

} // end routine accelerator_start_acquisition_with_fifo()



static uint32_t ii_accelerometer_stop_acquisition(const struct device *dev)
{
    uint8_t cmd[] = { 0, 0, 0 };
    uint8_t register_value = 0;
    uint32_t rstatus = 0;  // Routine status, combined status of register writes and reads

// (1) Disable IIS2DH FIFO:
    cmd[0] = IIS2DH_CTRL_REG5;
    iis2dh_ctrl_reg5 &= ~(FIFO_ENABLE);
    cmd[1] = iis2dh_ctrl_reg5;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

// (2) Reset IIS2DH FIFO by briefly setting bypass mode:
    cmd[0] = IIS2DH_FIFO_CTRL_REG;
    cmd[1] = FIFO_MODE_BYPASS; 
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

// (3) Disable FIFO watermark reached and FIFO overrun interrupts:
    cmd[0] = IIS2DH_CTRL_REG3;
    iis2dh_ctrl_reg3 &= ~(FIFO_WATERMARK_INTERRUPT_ON_INT1_ENABLE);
    iis2dh_ctrl_reg3 &= ~(FIFO_OVERRUN_INTERRUPT_ON_INT1_ENABLE);
    cmd[1] = iis2dh_ctrl_reg3;
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

// (4) Clear interrupt by reading interrupt status register:
    cmd[0] = IIS2DH_CTRL_REG3;
    cmd[1] = 0;
//    printk("333 - \n");
    rstatus |= kd_read_peripheral_register(dev, cmd, &register_value, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER);
//    printk("333 - from IIS2DH register CTRL3 read back %u\n", register_value);

// (5) Disable data rate (power down mode)
    cmd[0] = IIS2DH_CTRL_REG1;
    cmd[1] = ( (ODR_0_POWERED_DOWN | LOW_POWER_ENABLE) & (~(AXIS_Z_ENABLE)) & (~(AXIS_Y_ENABLE)) & (~(AXIS_X_ENABLE)));
    rstatus |= kd_write_peripheral_register(dev, cmd, 2);

//    printk("333 - combined status of config reg writes and read is %u,\n", rstatus);
//    printk("333 - returning to caller . . .\n");
    return rstatus;

} // end routine ii_accelerometer_stop_acquisition




float reading_in_g(const uint32_t reading_in_twos_comp, const uint32_t full_scale, const uint32_t res_in_bits)
{
    uint32_t reading = reading_in_twos_comp;
    uint32_t reading_bounded = (reading & 0xFF);
    float reading_as_float = 0.0;
    float scale_by_value = ( ( 2.0 * APPR_ACCELERATION_OF_GRAVITY ) / 128.0 );
    (void)full_scale;
    (void)res_in_bits;

// Later add check for values > 255, 1023, 4095, e.g. 8- 10- 12-bit readings
    if ( reading & 0x80 )
    {
        reading_bounded = ~(reading_bounded);
        reading_bounded &= 0xFF;   // keep 8-bit bounded reading bounded,
        reading_bounded++;
    }

    reading_as_float = ( (float)reading_bounded * scale_by_value );

    if ( reading & 0x80 )
        { reading_as_float *= -1.0; }

    return reading_as_float;
}




static uint32_t ii_accelerometer_read_xyz(const struct device *dev)
{
// -- VAR BEGIN ---
    uint8_t cmd[] = { 0, 0, 0 };
    uint8_t register_value = 0;
    uint32_t rstatus = 0;  // Routine status, combined status of register writes and reads

// QUESTION why necessary to logical or this register with 0x80? - TMH
    uint8_t iis2dh_x_axis_low_byte_reg = (0x80 | IIS2DH_OUT_X_L);
    memset(readings_data, 0, sizeof(readings_data));
    uint8_t source = 0;
    uint8_t count = 0;
    int i = 0;
// -- VAR END ---

// IIS2DH_FIFO_SRC_REG
#define COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER_FIFO_SRC (1)

// (1) Query for present FIFO level and overrun status flag:
    cmd[0] = IIS2DH_FIFO_SRC_REG;
    cmd[1] = 0;
    rstatus |= kd_read_peripheral_register(dev, cmd, &register_value, COUNT_BYTES_IN_IIS2DH_CONTROL_REGISTER_FIFO_SRC);
    count = (register_value & FIFO_SRC_FSS_MASK);
    printk("222 - IIS2DH FIFO source register holds %u, buffered reading count is %u,\n",
      register_value, count);

// Check for FIFO overrun, indicating missed readings:
    if (( source & FIFO_SOURCE_OVERRUN) != 0 )
    {
#if 1 // TO-DO create development define for this message:
        printk("- FIFO overrun detected!\n");
#endif
        fifo_overrun_events[fifo_overrun_count_fsv].reading_index = running_total_xyz_readings;
        fifo_overrun_count_fsv++;
    }

// Check whether FIFO is empty:
    if ( count == 0 )
    {
        printk("222 - no readings indicated in buffer, but showing 25 readings anyway:\n\n");
        count = 25;
    }

#if 1
    rstatus |= kd_read_peripheral_register(dev,
                                           &iis2dh_x_axis_low_byte_reg,
                                           readings_data,
                                           (BYTES_PER_XYZ_READINGS_TRIPLET * count)
                                          );
#else
    for ( i = 0; i < count; i += BYTES_PER_XYZ_READINGS_TRIPLET )
    {
        rstatus |= kd_read_peripheral_register(dev,
                                           &iis2dh_x_axis_low_byte_reg,
                                           &readings_data[(i * BYTES_PER_XYZ_READINGS_TRIPLET)],
                                           BYTES_PER_XYZ_READINGS_TRIPLET
                                          );
    }
#endif

    printk("data from %u readings:\n", count);
    for ( i = 0; i < count; i += BYTES_PER_XYZ_READINGS_TRIPLET )
    {
        printk(" %02X %02X  %02X %02X  %02X %02X  --  x,y,z in G = %2.3fG, %2.3fG, %2.3fG",
          readings_data[i],
          readings_data[i + 1],
          readings_data[i + 2],
          readings_data[i + 3],
          readings_data[i + 4],
          readings_data[i + 5],
          reading_in_g((uint32_t)readings_data[i], 0, 0),
          reading_in_g((uint32_t)readings_data[i + 2], 0, 0),
          reading_in_g((uint32_t)readings_data[i + 4], 0, 0)
        );

// ---** CODING MARK **---

//        if ( ( (i+1) % TRIPLETS_TO_FORMAT_PER_LINE ) == 0 )

#define EVERY_SIX_LINES (6)
        if ( ( (i+1) % EVERY_SIX_LINES ) == 0 )
        {
            printk("   <-- mark\n");
        }
        else
        {
            printk("\n");
        }
    }
    printk("\n");
 
    return rstatus;

} // end routine ii_accelerometer_read_xyz



void make_references(void)
{
#if 0
    (void)iis2dh_ctrl_reg1;
    (void)iis2dh_ctrl_reg2;
    (void)iis2dh_ctrl_reg4;
    (void)iis2dh_ctrl_reg5;
    (void)iis2dh_acc_status;
    (void)iis2dh_fifo_ctrl_reg;
#endif
}




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
// 2021-11-17 - moving *sensor to file scoped vars:
//    const struct device *sensor = DEVICE_DT_GET_ANY(st_iis2dh);
// 2021-11-17 - Ted unsure whether following line compiles:
//    const struct device *sensor = device_get_binding(DT_LABEL(IIS2DH_ACCELEROMETER));

    static uint16_t iis2ds12_i2c_periph_addr = DT_INST_REG_ADDR(0);   //<-- 2021-10-17 not available at build time - TMH

// Early development variable of type Zephyr sensor reading structure:
//    struct sensor_value acceleration_readings;
// 2021-11-17:
    struct sensor_value iis2dh_temperature_reading;

    uint8_t buff[10];
    memset(buff, 0, sizeof(buff));

// From LIS2DH sample app:
    int rc = 0;

//    uint8_t accelerometer_status = 0;

    enum iis2dh_output_data_rates_e odr_to_set = ODR_0_POWERED_DOWN;

// --- VAR END ---


// Check whether device_get_binding() macro / call succeeded:
    if ( sensor == NULL )
        { printk("No iis2dh device found!\n"); }
    else
        { printk("Success finding iis2dh device,\n"); }

    if (!device_is_ready(sensor)) {
        printk("- %s - Device %s is not ready\n", MODULE_ID__THREAD_IIS2DH, sensor->name);
        printk("- %s - Returning from this thread's entry point . . .\n", MODULE_ID__THREAD_IIS2DH);
        return;
    }
    else
    {
        printk("- %s - Call to device_is_ready() says %s sensor is ready,\n",
          MODULE_ID__THREAD_IIS2DH, sensor->name);
    }

    printk("- %s - INFO:  device name (Zephyr compile time setting) found to be '%s'\n",
      MODULE_ID__THREAD_IIS2DH, sensor->name);

// 2021-11-17 - Effective initialization, see 'TO DO' section for note on further required study here:
    rc = scoreboard_set_requested_iis2dh_odr(KD_APP_DEFAULT_IIS2DH_OUTPUT_DATA_RATE);

#if KD_DEV__ENABLE_IIS2DH_TEMPERATURE_READINGS == 1
    printk("- %s - INFO:  enabling iis2dh temperature readings . . .'\n",
      MODULE_ID__THREAD_IIS2DH);
    rc = configure_iis2dh_temperature_enable(sensor);
#endif

//    accelerator_start_acquisition_with_fifo(sensor, ODR_10_HZ);
    accelerator_start_acquisition_with_fifo(sensor, KD_APP_DEFAULT_IIS2DH_OUTPUT_DATA_RATE);

    while (1)
    {
        printk("iis2dh task at loop iteration 0x%08X\n IIS2DH sensor at I2C addr %02X\n",
          loop_count, iis2ds12_i2c_periph_addr);

        k_msleep(1);

        iis2dh_fifo_ctrl_reg = read_of_iis2dh_acc_fifo_src_register(sensor);
        printk("iis2dh FIFO SRC register 0x%02X holds %u,\n", IIS2DH_FIFO_SRC_REG, iis2dh_fifo_ctrl_reg);

        rc = ii_accelerometer_read_xyz(sensor);

        rc = ii_accelerometer_stop_acquisition(sensor);

        rc = scoreboard_get_requested_iis2dh_odr(&odr_to_set);
        printk("- iis2dh thread - using scoreboard ODR equal to %u,\n", odr_to_set);



// - 2021-11-17 -
// Initial test of IIS2DH temperature read:
#if KD_DEV__ENABLE_IIS2DH_TEMPERATURE_READINGS == 1
        rc = read_of_iis2dh_temperature_registers(sensor, &iis2dh_temperature_reading);
#endif


        accelerator_start_acquisition_with_fifo(sensor, odr_to_set);  // ODR_10_HZ);

        printk("\n\n");



// 2021-10-27 New static variable this module:  iis2dh_thread_sleep_time_in_ms
        k_msleep(SLEEP_TIME__IIS2DH_TASK__MS);
        loop_count++;

    } // end while (1) loop

} // end routine iis2dh_thread_entry_point



//----------------------------------------------------------------------
// - SECTION - routines public API
//----------------------------------------------------------------------

uint32_t wrapper_iis2dh_register_read(const uint8_t register_addr, uint8_t* register_value)
{
    uint32_t rstatus = ROUTINE_OK;

    if ( sensor == NULL )
    {
        rstatus = KD__DEVICE_POINTER_NULL;
    }
    else
    {
        rstatus = kd_read_peripheral_register(sensor,
                                              &register_addr,
                                              register_value,
                                              1
                                             );
    }

    return rstatus;
}


uint32_t wrapper_iis2dh_register_write(const uint8_t register_addr, uint8_t* register_value)
{
    uint32_t rstatus = ROUTINE_OK;

    if ( sensor == NULL )
    {
        rstatus = KD__DEVICE_POINTER_NULL;
    }

    return rstatus;
}



uint32_t wrapper_iis2dh_register_read_multiple(const uint8_t register_addr, uint8_t* register_value, const uint32_t byte_count)
{
    uint32_t rstatus = ROUTINE_OK;

    if ( sensor == NULL )
    {
        rstatus = KD__DEVICE_POINTER_NULL;
    }
    else
    {
        rstatus = kd_read_peripheral_register(sensor,
                                              &register_addr,
                                              register_value,
                                              byte_count
                                             );
    }

    return rstatus;
}




//----------------------------------------------------------------------
// - SECTION - notes and tests ( non-active code )
//----------------------------------------------------------------------

// BLOCK BEGIN
#if 0
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
#endif
// BLOCK END



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



// *** review 6cd823d21eeaa begin ******************
#if 0
        k_msleep(20);
        printk("- DEV004 - fetching readings just after 20ms delay...\n");
        rc = sensor_sample_fetch(sensor);

        rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, &accel[0]);
        if ( rc == 0 )
        {
            printk("x, y, z accelerations in two's compliment:  %02X %02X   %02X %02X   %02X %02X\n",
              accel[0].val1, accel[0].val2,
              accel[1].val1, accel[1].val2,
              accel[2].val1, accel[2].val2
            );
        }
        else
        {
            printk("ERROR getting acceleration readings, error is %u\n", rc);
        }
#endif
// *** review 6cd823d21eeaa end ******************



// --- EOF ---
