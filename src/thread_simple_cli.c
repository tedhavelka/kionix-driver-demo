//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/tedhavelka/kionix-driver-demo
//
//      File:  thread_simple_cli.c
//
//----------------------------------------------------------------------

/*
 *  @Brief:  Zephyr thread to support a very simple command line
 *     line interface via host board UART.
 *
 *  @References:
 *
 *  +  https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#spawning-a-thread
 */



//----------------------------------------------------------------------
// - SECTION - pound includes
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

// 2021-10-05 - CLI incorporation work, see Zephyr v2.6.0 file "zephyr/subsys/console/tty.c"
#include <drivers/uart.h>   // to provide uart_poll_in()

#include "diagnostic.h"



//----------------------------------------------------------------------
// - SECTION - pound defines
//----------------------------------------------------------------------

// defines thread related:
#define SIMPLE_CLI_THREAD_STACK_SIZE 1024
#define SIMPLE_CLI_THREAD_PRIORITY 8   // NEED to implement project enum of project thread priorities - TMH

// defines for application or task implemented by this thread:
#define SLEEP_TIME__SIMPLE_CLI__MS (10)

#define SIZE_COMMAND_TOKEN (32)    // in bytes, and we needn't support long paths, filenams or other identifiers

#define SIZE_COMMAND_HISTORY (20)  // 2021-10-25 - not yet implemented



//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL



//----------------------------------------------------------------------
// - SECTION - routine prototypes
//----------------------------------------------------------------------

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3);


//----------------------------------------------------------------------
// - SECTION - File scoped or global variables
//----------------------------------------------------------------------

static char latest_command[SIZE_COMMAND_TOKEN];



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(simple_cli_thread_stack_area, SIMPLE_CLI_THREAD_STACK_SIZE);
struct k_thread simple_cli_thread_thread_data;

int initialize_thread_simple_cli_task(void)
{
    k_tid_t simple_cli_task_tid = k_thread_create(&simple_cli_thread_thread_data, simple_cli_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(simple_cli_thread_stack_area),
                                            simple_cli_thread_entry_point,
                                            NULL, NULL, NULL,
                                            SIMPLE_CLI_THREAD_PRIORITY,
                                            0,
                                            K_MSEC(1000)); // K_NO_WAIT);
    return (int)simple_cli_task_tid;
}


//
//----------------------------------------------------------------------
// - SECTION - int main or entry point
//----------------------------------------------------------------------
//

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3)
{
// --- VAR BEGIN ---
//    int loop_count = 0;
    char dev_msg[256];   // printk(), printf() development messages string
// --- VAR END ---


// 2021-10-05
// REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
    const struct device *uart_for_cli;
//    uart_for_cli = device_get_binding(DT_LABEL(UART_2));
//    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));

    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
    if ( uart_for_cli == NULL )
    {   
        dmsg("Failed to assign pointer to UART2 device!\n", PROJECT_DIAG_LEVEL);
    } 

    while (1)
    {
        if ( uart_for_cli != NULL )
        {
            char lbuf[160];
            memset(lbuf, 0, sizeof(lbuf));
            unsigned char* msg = lbuf;
            uart_poll_in(uart_for_cli, msg);

            if ( strlen(msg) > 0 )
            {
                snprintf(dev_msg, sizeof(dev_msg), "zzz - %s - zzz\n", msg);
                dmsg(dev_msg, DIAG_NORMAL);
            }
        }

        k_msleep(SLEEP_TIME__SIMPLE_CLI__MS);
    }

} // end of thread entry point routine



// --- EOF ---
