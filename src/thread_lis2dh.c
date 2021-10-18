//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/kionix-driver-demo/tree/iis2dh-driver-integration-work-001
//
//      File:  thread_lis2dh.c
//
//----------------------------------------------------------------------

/*
 *  @Brief:  Zephyr thread to obtain and to store acceleration readings
 *     taken from STMicro LIS2DH sensor, using STMicro driver API in
 *     Nordic sdk-nrf modules/hal/st/sensor/stmemsc/lis2dh_STdC/driver.
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

// Work to utilize LIS2DH driver structures and routines from STMicro, and compare these with IIS2DH API:
// ( Note https://github.com/STMicroelectronics/iis2dh )
#include <lis2dh.h>  // to include iis2dh_reg.h, to provide stmdev_ctx_t and related



//----------------------------------------------------------------------
// - SECTION - defines
//----------------------------------------------------------------------

// defines thread related:
#define LIS2DH_THREAD_STACK_SIZE 1024
#define LIS2DH_THREAD_PRIORITY 6

// defines for application or task implemented by this thread:
#define SLEEP_TIME__LIS2DH_TASK__MS (3000)

// defines to connect with STMicro IIS2DH out-of-tree driver API:
//#define KX132_1211 DT_INST(0, kionix_kx132_1211)
#define ST_LIS2DH DT_INST(0, st_lis2dh)

//#define CONFIG_KX132_1211_DEV_NAME DT_LABEL(KX132_1211)
#define CONFIG_ST_LIS2DH_DEV_NAME DT_LABEL(LIS2DH)

// https://docs.zephyrproject.org/latest/guides/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
#define LIS2DH_ACCELEROMETER DT_NODELABEL(stmicro_sensor)


//----------------------------------------------------------------------
// - SECTION - prototypes
//----------------------------------------------------------------------

void lis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3);


//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(lis2dh_thread_stack_area, LIS2DH_THREAD_STACK_SIZE);
struct k_thread lis2dh_thread_thread_data;



int initialize_thread_lis2dh_task(void)
{
    k_tid_t lis2dh_task_tid = k_thread_create(&lis2dh_thread_thread_data, lis2dh_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(lis2dh_thread_stack_area),
                                            lis2dh_thread_entry_point,
                                            NULL, NULL, NULL,
                                            LIS2DH_THREAD_PRIORITY,
                                            0,
                                            K_MSEC(800));  // can also be 'K_NO_WAIT'
    return (int)lis2dh_task_tid;
}



void lis2dh_thread_entry_point(void* arg1, void* arg2, void* arg3)
{

// --- VAR BEGIN ---
    int loop_count = 0;

// 2021-10-12 - keep assignment on this declarative line so it gets built at compile time:
    const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);
//    const struct device *sensor = device_get_binding(DT_LABEL(IIS2DH_ACCELEROMETER));

    struct sensor_value acceleration_readings;
//#include <lis2dh_reg.h>

    uint8_t buff[10];
    memset(buff, 0, sizeof(buff));

// From LIS2DH sample app:
    int rc = 0;
    struct sensor_value accel[3];
    const char *overrun = "";

// DEV tests:
    struct lis2dh_data *lis2dh = sensor->data; // dev->data;
    uint32_t whoami_check_status = 0;
// Per zephyr/drivers/sensor/lis2dh/lis2dh.c:119
    uint16_t sensor_data_rate = 0;
// --- VAR END ---


// Check whether device_get_binding() macro / call succeeded:
    if ( sensor == NULL )
        { printk("No lis2dh device found!\n"); }
    else
        { printk("Success finding lis2dh device,\n"); }

    if (!device_is_ready(sensor)) {
        printf("Device %s is not ready\n", sensor->name);
        return;
    }
    else
    {
        printf("Call to device_is_ready() says %s sensor is ready,\n", sensor->name);
    }

    printf("Device name found to be '%s'\n", sensor->name);


/*
// From file zephyr/drivers/sensor/iis2dh/iis2dh.c:   . . . NEED TO CHECK IF SAME FUNCTION POINTERS PRESENT IN LIS2DH API - TMH
222 static const struct sensor_driver_api iis2dh_driver_api = {
223         .attr_set = iis2dh_attr_set,
224 #if CONFIG_IIS2DH_TRIGGER
225         .trigger_set = iis2dh_trigger_set,
226 #endif / * CONFIG_IIS2DH_TRIGGER * /
227         .sample_fetch = iis2dh_sample_fetch,
228         .channel_get = iis2dh_channel_get,
229 };
*/

//    whoami_check_status = lis2dh_device_id_get(lis2dh->ctx, buff);

// Unsafe, unbounded print string type call:   - TMH
//    printk("sensor who-am-i check returns '%s',\n", buff);
//    printk("(However this value of three returned even when sensor not attached - need to investigate)\n");

    while (1)
    {
        printk("lis2dh task at loop iteration 0x%08X\n", loop_count);

        struct sensor_value odr = { .val1 = 1, };

        {
            rc = sensor_attr_set(sensor,
                                 SENSOR_CHAN_ACCEL_XYZ, //trig.chan,
                                 5, // SENSOR_ATTR_SAMPLING_FREQUENCY,
                                 &odr
                                );
            if (rc != 0) {
                printf("Failed to set odr: %d\n", rc);
//                return;
            }
            printf("Sampling at %u Hz (asuuming so as sensor_attr_set returns 0 status - success\n", odr.val1);
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


        printk("\n");
        k_msleep(SLEEP_TIME__LIS2DH_TASK__MS);
        loop_count++;
    }

}




// --- EOF ---
