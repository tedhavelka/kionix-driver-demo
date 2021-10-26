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
#include <drivers/uart.h>        // to provide uart_poll_in()

#include "diagnostic.h"

//#include "thread_simple_cli.h"   // to provide experimental global vars for simple data sharing
#include "scoreboard.h"   // to provide experimental global vars for simple data sharing



//----------------------------------------------------------------------
// - SECTION - pound defines
//----------------------------------------------------------------------

// defines thread related:
#define SIMPLE_CLI_THREAD_STACK_SIZE 1024
#define SIMPLE_CLI_THREAD_PRIORITY 8   // NEED to implement project enum of project thread priorities - TMH

// defines for application or task implemented by this thread:
#define SLEEP_TIME__SIMPLE_CLI__MS (10)

#define SIZE_COMMAND_TOKEN (32)    // in bytes, and we needn't support long paths, filenams or other identifiers

#define SIZE_COMMAND_HISTORY (10)  // 2021-10-25 - not yet implemented



//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL



//----------------------------------------------------------------------
// - SECTION - routine prototypes
//----------------------------------------------------------------------

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3);
void show_prompt(void);


//----------------------------------------------------------------------
// - SECTION - File scoped or global variables
//----------------------------------------------------------------------

static char latest_command[SIZE_COMMAND_TOKEN];
static char command_history[SIZE_COMMAND_HISTORY][SIZE_COMMAND_TOKEN];
static uint32_t index_to_cmd_history;
static uint32_t index_within_cmd_token;

static char newline_string[] = { '\n' };

static const struct device *uart_for_cli;



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



void initialize_command_handler(void)
{
    memset(latest_command, 0, sizeof(latest_command));
    for ( int i = 0; i < SIZE_COMMAND_HISTORY; i++ )
    {
        memset(command_history[i], 0, sizeof(command_history[i]));
    }

    index_to_cmd_history = 0;
    index_within_cmd_token = 0;
    show_prompt();
}



uint32_t printk_cli(const char* output)
{
    uint32_t rstatus = 0;
    uint32_t output_byte_count = strlen(output);

    if ( uart_for_cli == NULL )
    {
        return 1;
    }

    for ( int i = 0; i < output_byte_count; i++ )
    {
        uart_poll_out(uart_for_cli, output[i]);
    }
    return rstatus;
} 



void clear_latest_command_string(void)
{
    memset(latest_command, 0, sizeof(latest_command));
    index_within_cmd_token = 0;
    show_prompt();
}



#define PS1 "\n\rkd-demo > "

void show_prompt(void)
{
//    printk("%s", PS1);
    printk_cli(PS1);
}



static uint32_t command_handler(const char* input)
{
    uint32_t test_value = 0;
    uint32_t rstatus = 0;

    printk("command_handler received '%s'\n", input);

    if ( input[0] == 'a' )
    {
        rstatus = get_global_test_value(&test_value);
        printk("- cmd a - global setting for data sharing test holds %u,\n", test_value);
    }

    if ( input[0] == 'b' )
    {
        rstatus = set_global_test_value(3);
    }

    if ( input[0] == 'c' )
    {
        rstatus = set_global_test_value(256);
    }

//    clear_latest_command_string();

    return rstatus;
}



uint32_t build_command_string(const char* latest_input, const struct device* callers_uart)
{
    uint32_t rstatus = 0;

#if 0
#endif

// If latest character printable and not '\r' . . . append latest command string:
    if ( (latest_input[0] >= 0x20) && (latest_input[0] < 0x7F) && (latest_input[0] != '\r') )
//       ~~~~~~~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~~
    {
        if ( index_within_cmd_token >= SIZE_COMMAND_TOKEN )
        {
            printk("Supported commnd length exceeded!\n");
            printk("Press <ENTER> to process or <ESC> to start over.\n");
        }
        else
        {
            latest_command[index_within_cmd_token] = latest_input[0];
            ++index_within_cmd_token;
            uart_poll_out(callers_uart, latest_input[0]);
        }
    }

// If latest character is <ESC> then clear the latest captured command string:
    if ( latest_input[0] == 0x1B )
    {
        clear_latest_command_string();
    }

// If latest character is <ENTER> then call the command handler:
    if ( latest_input[0] == '\r' )
    {
// Following two lines an effective "\n\r" test:
        uart_poll_out(callers_uart, newline_string[0]);
        uart_poll_out(callers_uart, latest_input[0]);
        rstatus = command_handler(latest_command);
        clear_latest_command_string();
    }

    return rstatus;
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
//    char dev_msg[256];   // printk(), printf() development messages string
// --- VAR END ---


// 2021-10-05
// REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
//// Moving to file-scoped static pointer:
//    const struct device *uart_for_cli;
//// Following two lines fail to compile:
//    uart_for_cli = device_get_binding(DT_LABEL(UART_2));
//    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));

    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
    if ( uart_for_cli == NULL )
    {   
        dmsg("Failed to assign pointer to UART2 device!\n", PROJECT_DIAG_LEVEL);
    } 

    initialize_command_handler();

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
#if 0
                snprintf(dev_msg, sizeof(dev_msg), "zzz - %s - zzz\n", msg);
                dmsg(dev_msg, DIAG_NORMAL);
                if ( ( msg[0] == '\n' ) ||  ( msg[0] == '\r' ) )
                {
                    printk("simple cli - got a command!\n");
                }
                else
                {
                    command_handler(msg);
                }
#endif

                build_command_string(msg, uart_for_cli);
            }
        }

        k_msleep(SLEEP_TIME__SIMPLE_CLI__MS);
    }

} // end of thread entry point routine



// --- EOF ---
