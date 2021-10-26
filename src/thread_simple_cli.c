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
 *     +  https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#spawning-a-thread
 *
 *    2021-10-05 . . .
 *     +  REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
 *
 *
 *  @Implementation:
 *
 *    (1)  To add new CLI commands first search in this source file for
 *    text pattern 'ADD COMMANDS - STEP n:' where 'n' is one of 1, 2
 *    of 3.  As of 2021 Q4 there are three places in this project file
 *    where a supported command has parts of itself expressed.  These
 *    places are,
 *    
 *      +  CLI routine prototypes section
 *      +  array of CLI routine structure instances (function pts plus couple other members)
 *      +  CLI routine definition
 *
 *    CLI routines have a particular signature and form.  They accept
 *    an invariant string as their argument, and return a uint32_t 
 *    value.  The invariant string in a CLI routine signature can be
 *    further parsed into multiple arguments by the given routine.
 *
 *    Over time, a few simple parsing utility functions are planned to
 *    provide this very tokenizing, and some simple string-to-number
 *    conversions.
 *
 *
 *  @Notes:
 *
 *    (1)  At commit 0a6d0e6f67330cc0a0656ca9f5792287719a8ea2 there is not
 *    yet a short description field in the "CLI command instance structure".
 *    In the nRF9160 ARM core we have 1MB of FLASH memory so we should
 *    be ok resource wise to add this.  Doing so in Kionix Demo project,
 *    git branch 'cli-dev-work-003' - TMH
 *
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
#define SLEEP_TIME__SIMPLE_CLI__MS (50)

#define SIZE_COMMAND_TOKEN (32)       // in bytes, and we needn't support long paths, filenams or other identifiers

#define SIZE_COMMAND_HISTORY (10)     // 2021-10-25 - not yet implemented

#define SIZE_OF_ARGS_SUPPORTED (256)



//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL



//----------------------------------------------------------------------
// - SECTION - routine prototypes
//----------------------------------------------------------------------

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3);
uint32_t printk_cli(const char* output);
void show_prompt(void);


// *************************************************************************************************
// - ADD COMMANDS - STEP 1:  add a prototype for CLI routine here
// *************************************************************************************************

// command handler prototypes:
uint32_t output_data_rate_handler(const char* args);
uint32_t iis2dh_sensor_handler(const char* args);
uint32_t help_message(const char* args);
uint32_t banner_message(const char* args);
// uint32_t (const char* args);


//----------------------------------------------------------------------
// - SECTION - File scoped or global variables
//----------------------------------------------------------------------

static char latest_command[SIZE_COMMAND_TOKEN];
static char command_history[SIZE_COMMAND_HISTORY][SIZE_COMMAND_TOKEN];  // <-- 2021-10-26 not yet used
static uint32_t index_to_cmd_history;
static uint32_t index_within_cmd_token;

//static char newline_string[] = { '\n' };

static const struct device *uart_for_cli;


// work in project git branch 'cli-dev-work-003':

static uint32_t implemented_command_count;

typedef uint32_t (*command_handler_t)(const char* argument_string);

struct cli_command_writers_api
{
    char* token_to_represent_command;
    char* description;
    command_handler_t handler;
};

// *************************************************************************************************
// - ADD COMMANDS - STEP 2:  add entry in array of CLI routine structures here
// *************************************************************************************************

struct cli_command_writers_api kd_command_set[] =
{
    { "odr", "IN PROGRESS - to be iis2dh Output Data Rate (ODR) set and get command.", &output_data_rate_handler },
    { "iis2dh", "NOT YET IMPLEMENTED - to be general purpose iis2dh configurations command.", &iis2dh_sensor_handler },
    { "help", "show supported Kionix demo CLI commands.", &help_message },
    { "?", "show supported Kionix demo CLI commands.", &help_message },
    { "banner", "show brief project identifier string for this Zephyr based app.", &banner_message }
};




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

// Count of implemented CLI commands known at build time, used couple times at run time:
    implemented_command_count = (sizeof(kd_command_set) / sizeof(struct cli_command_writers_api));

    index_to_cmd_history = 0;
    index_within_cmd_token = 0;

// First stuff out the CLI UART:
    printk_cli("\n\n\n\n\n");
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
    printk_cli(PS1);
}



uint32_t command_and_args_from_input(const char* latest_input,
                                     char* command,
                                     char* args)
{
    uint32_t input_byte_count = strlen(latest_input);
    uint32_t i = 0;
    uint32_t flag_white_space_found = 0;
    uint32_t rstatus = 0;

printk("In 'command and args' about to parse %u bytes,\n", input_byte_count);

    for ( i = 0; (i < input_byte_count); i++ )
    {
        if ( latest_input[i] == 0x20 )
        {
            flag_white_space_found = 1;
            strncpy(command, latest_input, (i - 0));
            strncpy(args, (latest_input + i + 1), (input_byte_count - i));
//printk("In 'command and args' found white space at string index %u\n", i);
            i = input_byte_count;
        }

//        if ( latest_input[i] == '\r' )
        if ( ( latest_input[i] == '\r' ) || ( latest_input[i] == '\n' ) )
        {
            strncpy(command, latest_input, (i - 0));
            strncpy(args, "", 1);
//printk("In 'command and args' found <CR> at string index %u\n", i);
            i = input_byte_count;
        }
    }

    if ( !flag_white_space_found )
    {
        strncpy(command, latest_input, input_byte_count);
    }

//printk("After parsing loop index i = %u,\nreturning . . .\n", i);

    return rstatus;
}



static uint32_t command_handler(const char* latest_input)
{
// --- VAR BEGIN ---
    uint32_t test_value = 0;
    char command[SIZE_COMMAND_TOKEN];
    char args[SIZE_OF_ARGS_SUPPORTED];
    uint32_t rstatus = 0;
// --- VAR END ---

    memset(command, 0, sizeof(command));
    memset(args, 0, sizeof(args));
    rstatus = command_and_args_from_input(latest_input, command, args);

#if 0
    printk("command_handler received '%s'\n", latest_input);

    if ( latest_input[0] == 'a' )
    {
        rstatus = get_global_test_value(&test_value);
        printk("- cmd a - global setting for data sharing test holds %u,\n", test_value);
    }

    if ( latest_input[0] == 'b' )
    {
        rstatus = set_global_test_value(3);
    }

    if ( latest_input[0] == 'c' )
    {
        rstatus = set_global_test_value(256);
    }

#else

#if 0
    printk("parsed command '%s',\n", command);
    printk("and arguments '%s'\n", args);
    printk("checking %u implemented Kionix demo commands...\n", ((sizeof(kd_command_set) / sizeof(struct cli_command_writers_api)) - 0));
#endif
    
    for ( int i = 0; i < ( sizeof(kd_command_set) / sizeof(struct cli_command_writers_api) ); i++ )
    {
        if ( strncmp(command, kd_command_set[i].token_to_represent_command, SIZE_COMMAND_TOKEN) == 0 )
        {
            rstatus = kd_command_set[i].handler(args);
        }
    }

#endif

    return rstatus;
}



uint32_t build_command_string(const char* latest_input, const struct device* callers_uart)
{
    uint32_t rstatus = 0;

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
// When both active, following two lines an effective "\n\r" test:
//        uart_poll_out(callers_uart, newline_string[0]);
        uart_poll_out(callers_uart, latest_input[0]);
        rstatus = command_handler(latest_command);
        clear_latest_command_string();
    }

    return rstatus;
}



//----------------------------------------------------------------------
// - SECTION - command handlers
//----------------------------------------------------------------------

// *************************************************************************************************
// - ADD COMMANDS - STEP 3:  add CLI routine definitions here
// *************************************************************************************************

uint32_t output_data_rate_handler(const char* args)
{
    printk_cli("2021-10-25 ODR stub function\n");
    return 0;
} 


uint32_t iis2dh_sensor_handler(const char* args)
{
    printk_cli("iis2dh stub function\n");
    return 0;
} 



//    { "help", &help_message },
//    { "?", &help_message },
//    { "banner", &banner_message }


uint32_t help_message(const char* args)
{
#define SIZE_OF_MESSAGE_SHORT (80)
#define SIZE_OF_MESSAGE_MEDIUM (160)
#define WIDTH_OF_BULLET_POINT (3)
#define WIDTH_OF_COMMAND_TOKEN_OR_NAME (12)
#define WIDTH_OF_COMMAND_DESCRIPTION (80)

    char lbuf[SIZE_OF_MESSAGE_MEDIUM];

    printk_cli("Kionix demo CLI commands:\n\r");
    for ( int i = 0; i < implemented_command_count; i++ )
    {
//        snprintf(lbuf, SIZE_OF_SHORT_MESSAGE, "  %u)  %s\n\r", i, kd_command_set[i].token_to_represent_command);
        snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "  %*u)  %s%*s   . . . %s\n\r",
                   WIDTH_OF_BULLET_POINT,
                   i,
                   kd_command_set[i].token_to_represent_command,
//                   WIDTH_OF_COMMAND_TOKEN_OR_NAME,
                   (WIDTH_OF_COMMAND_TOKEN_OR_NAME - strlen(kd_command_set[i].token_to_represent_command)),
                   " ",
//                   WIDTH_OF_COMMAND_DESCRIPTION,
                   kd_command_set[i].description
                 );

        printk_cli(lbuf);
    }
    printk_cli("\n\r");
    return 0;
} 



uint32_t banner_message(const char* args)
{
    printk_cli("\n--\n\r");
    printk_cli("-- Kionix Driver Demo\n\r");
    printk_cli("-- A small Zephyr RTOS 2.6.0 based app to exercise Kionix KX132-1211 accelerometer\n\r");
    printk_cli("--\n\r");
    return 0;
}




//
//----------------------------------------------------------------------
// - SECTION - int main or entry point
//----------------------------------------------------------------------
//

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3)
{
// --- VAR BEGIN ---
    char lbuf[160];
    memset(lbuf, 0, sizeof(lbuf));
    unsigned char* msg = lbuf;
// --- VAR END ---

//// Moving to file-scoped static pointer to ease routine implementations like 'show_prompt()':
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
#if 0
            char lbuf[160];
            memset(lbuf, 0, sizeof(lbuf));
            unsigned char* msg = lbuf;
#endif
            memset(lbuf, 0, sizeof(lbuf));
            msg = lbuf;
            uart_poll_in(uart_for_cli, msg);

            if ( strlen(msg) > 0 )
            {
                build_command_string(msg, uart_for_cli);
            }
        }

        k_msleep(SLEEP_TIME__SIMPLE_CLI__MS);
    }

} // end of thread entry point routine



// --- EOF ---
