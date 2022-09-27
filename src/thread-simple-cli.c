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
 *     +  REF https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#spawning-a-thread
 *
 *    2021-10-05
 *     +  REF https://lists.zephyrproject.org/g/devel/topic/help_required_on_reading_uart/16760425
 *
 *    2021-11-18
 *     +  REF https://www.asciihex.com/character/control/8/0x08/bs-backspace
 *
 *
 *  @Key routines:
 *
 *     +  build_command_string(...)
 *
 *     +  command_handler(const char* latest_input)
 *
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
// - SECTION - TO DO
//----------------------------------------------------------------------

/*

   [x] TO DO 2021-11-17 - Implement simple backspace character-wise deletion on input line
        DONE 2021-11-18

        DONE 2021-11-18 - '/' echoes present input on new prompt and continues to 'listen'

   [ ] TO DO 2021-11-17 - Implement command history

*/



//----------------------------------------------------------------------
// - SECTION - pound includes
//---------------------------------^------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                // to provide memset()

// Zephyr RTOS headers . . .
#include <zephyr.h>

// to provide Zephyr's device_get_binding():
#include <device.h>

#include <devicetree.h>
// NOTE:  following two headers are located in ${ZEPHYR_BASE}/include/drivers
#include <drivers/gpio.h>
#include <drivers/sensor.h>

// 2021-10-05 - CLI incorporation work, see Zephyr v2.6.0 file "zephyr/subsys/console/tty.c"
#include <drivers/uart.h>          // to provide uart_poll_in()


//
// Project specific includes:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "common.h"
#include "diagnostic.h"
#include "module-ids.h"
#include "return-values.h"
#include "kionix-demo-errors.h"
#include "banner.h"

// Thread and module data sharing module:
#include "scoreboard.h"            // to provide experimental global vars for simple data sharing

// Individual and small command set routine definitions:
#include "cli-zephyr-stack-info.h"
#include "cli-zephyr-kernel-timing.h"

#include "thread-simple-cli.h"     // to provide prototype for printk_cli(),
                                   // ( called earlier than defined in this source file. )



//----------------------------------------------------------------------
// - SECTION - pound defines
//----------------------------------------------------------------------

// defines thread related:
#define SIMPLE_CLI_THREAD_STACK_SIZE 3072 // 2048 // 1536 // 3072 // 1024
#define SIMPLE_CLI_THREAD_PRIORITY 8   // NEED to implement project enum of project thread priorities - TMH

// defines for application or task implemented by this thread:
#define SLEEP_TIME__SIMPLE_CLI__MS (50)

// Note, for now we limit a token or character strings sans white space
// to 32 characters, subject to change as needed:
//#define SIZE_COMMAND_TOKEN (32)

// Note, space permitting we'll store up to ten user commands in a ring buffer:
#define SIZE_COMMAND_HISTORY (10)     // 2021-10-25 - not yet implemented

// Note, for now we support command line up to 256 characters:
#define SIZE_COMMAND_INPUT_SUPPORTED (256)

// Note, we start with support for passing up to ten args to a CLI command herein:
#define MAX_COUNT_SUPPORTED_ARGS (10)
//#define SUPPORTED_ARG_LENGTH     (16)



//----------------------------------------------------------------------
// - SECTION - DEVELOPMENT FLAGS
//----------------------------------------------------------------------

#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL


// Latest parsed arguments:
static char argument_array[MAX_COUNT_SUPPORTED_ARGS][SUPPORTED_ARG_LENGTH];

// Count of latest parsed arguments, tokens following latest command:
static uint32_t argument_count;

//// Flag to indicate present input character is first backspace in latest series of backspaces:
//static uint32_t flag_fresh_backspace_keypress = TRUE;



//----------------------------------------------------------------------
// - SECTION - routine prototypes
//----------------------------------------------------------------------

void simple_cli_thread_entry_point(void* arg1, void* arg2, void* arg3);
void show_prompt(void);

uint32_t store_args_from(const char* string);
uint32_t dev_show_args(void);

uint32_t arg_n(const uint32_t requested_arg, char* return_arg);
uint32_t arg_is_decimal(const uint32_t index_to_arg, int* value);
uint32_t arg_is_hex(const uint32_t index_to_arg, int* value);



// *************************************************************************************************
// - ADD COMMANDS - STEP 1:  add a prototype for CLI routine here
// *************************************************************************************************

// command handler prototypes:
uint32_t output_data_rate_handler(const char* args);
uint32_t cli__help_message(const char* args);
//uint32_t banner_message(const char* args);

// cli-zephyr-stack-info.h . . .
extern uint32_t cli__zephyr_2p6p0_stack_statistics(const char* args);
// banner.h . . .
extern uint32_t cli__kd_version(const char* args);
// banner.h . . .
extern uint32_t cli__banner_message(const char* args);
// cli-zephyr-kernel-timing.h . . .

// . . .
extern uint32_t iis2dh_sensor_handler(const char* args);



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
    { "help", "show supported Kionix demo CLI commands.", &cli__help_message },
    { "?", "show supported Kionix demo CLI commands.", &cli__help_message },
    { "banner", "show brief project identifier string for this Zephyr based app.", &cli__banner_message },
    { "version", "show this Kionix Driver Demo version info", &cli__kd_version },

    { "odr", "IIS2DH Output Data Rate (ODR) set and get command.", &output_data_rate_handler },
    { "iis2dh", "IMPLEMENTATION UNDERWAY - general purpose iis2dh configuration command.", &iis2dh_sensor_handler },
//    { "stacks", "show Zephyr RTOS thread stack statistics", &cli__zephyr_2p6p0_stack_statistics },
    { "st", "show Zephyr RTOS thread stack statistics", &cli__zephyr_2p6p0_stack_statistics },

    { "cyc", "show Zephyr kernel run time cycles count", &cli__show_zephyr_kernel_runtime_cycle_count }
};




//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

// https://docs.zephyrproject.org/latest/reference/kernel/threads/index.html#c.K_THREAD_STACK_DEFINE

K_THREAD_STACK_DEFINE(simple_cli_thread_stack_area, SIMPLE_CLI_THREAD_STACK_SIZE);
struct k_thread simple_cli_thread_thread_data;

int initialize_thread_simple_cli_task(void)
{
    uint32_t rstatus = ROUTINE_OK;

    k_tid_t simple_cli_task_tid = k_thread_create(&simple_cli_thread_thread_data, simple_cli_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(simple_cli_thread_stack_area),
                                            simple_cli_thread_entry_point,
                                            NULL, NULL, NULL,
                                            SIMPLE_CLI_THREAD_PRIORITY,
                                            0,
                                            K_MSEC(1000)); // K_NO_WAIT);

    rstatus = k_thread_name_set(simple_cli_task_tid, MODULE_ID__THREAD_SIMPLE_CLI);
    if ( rstatus == 0 ) { } // avoid compiler warning about unused variable - TMH

    return (int)simple_cli_task_tid;
}



uint32_t printk_cli(const char* message)
{
    uint32_t rstatus = 0;
    uint32_t message_byte_count = strlen(message);

    if ( uart_for_cli == NULL )
    {
        return KD_ERROR__HANDLE_TO_ALTERNATE_UART_NULL;
    }

    for ( int i = 0; i < message_byte_count; i++ )
    {
        uart_poll_out(uart_for_cli, message[i]);
    }
    return rstatus;
} 



void clear_argument_array(void)
{
    int i = 0;

    for ( i = 0; i < SIZE_COMMAND_HISTORY; i++ )
    {
        memset(argument_array[i], 0, sizeof(argument_array[i]));
    }

    argument_count = 0;
}



uint32_t argument_count_from_cli_module(void)
{
    return argument_count;
}


/*
 *----------------------------------------------------------------------
 *  @Description:  routine to initialize simple command line interface,
 *     carries out following tasks:
 *
 *     +  clears command history (history not yet implemented 2021-11-15)
 *     +  clears index to current command in history (planned to be a ring buffer history)
 *     +  clears a global index used to traverse bytes in command token
 *     +  clears array of latest parsed command line arguments (tokens following command)
 *     +  displays given app banner message
 *     +  displays first instance of command line prompt
 *
 *  @Note  
 *----------------------------------------------------------------------
 */

void initialize_command_handler(void)
{
    int i = 0;
    uint32_t rstatus = ROUTINE_OK;

    memset(latest_command, 0, sizeof(latest_command));
    for ( i = 0; i < SIZE_COMMAND_HISTORY; i++ )
    {
        memset(command_history[i], 0, sizeof(command_history[i]));
    }

// Count of implemented CLI commands known at build time, used couple times at run time:
    implemented_command_count = (sizeof(kd_command_set) / sizeof(struct cli_command_writers_api));

    index_to_cmd_history = 0;
    index_within_cmd_token = 0;

// Clear global argument array and argument count variable:
    clear_argument_array();

// First stuff out the CLI UART:
    printk_cli("\n\n\n\n\n");
    rstatus = cli__banner_message("placeholder_args");
    show_prompt();
}



void clear_latest_command_string(void)
{
    memset(latest_command, 0, sizeof(latest_command));
    index_within_cmd_token = 0;
    show_prompt();
}



// --- 1118 b DEV BEGIN ---
void delete_one_char_from_latest_command_string(void)
{
    char lbuf[DEFAULT_MESSAGE_SIZE];
    uint32_t command_length = strlen(latest_command);

#define BACKSPACE_DEBUG 0
#if BACKSPACE_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "- before backspace - present command length is %u\n\r", command_length);
    printk_cli(lbuf);
#endif
#if 0
//    if ( flag_fresh_backspace_keypress == TRUE )
#endif

    if ( command_length > 0 )
    {
#if BACKSPACE_DEBUG == 1
        printk_cli("deleting a character,\n\r");
#endif
        --index_within_cmd_token;
        latest_command[index_within_cmd_token] = 0;
        command_length = strlen(latest_command);
    }

#if BACKSPACE_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "- after backspace - present command length is %u\n\r", command_length);
    printk_cli(lbuf);
#endif
    show_prompt();
    printk_cli(latest_command);
}
// --- 1118 b DEV END ---





#define PS1 "\n\rkd-demo > "

void show_prompt(void)
{
    printk_cli(PS1);
}



void show_prompt_and_latest_command(void)
{
    printk_cli(PS1);
    printk_cli(latest_command);
}



uint32_t command_and_args_from_input(const char* latest_input,
                                     char* command,
                                     char* args)
{
    uint32_t input_byte_count = strlen(latest_input);
    uint32_t i = 0;
    uint32_t flag_white_space_found = 0;
    uint32_t rstatus = 0;

//printk("In 'command and args' about to parse %u bytes,\n", input_byte_count);

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
//    uint32_t test_value = 0;
    char command[SIZE_COMMAND_TOKEN];
    char args[SIZE_COMMAND_INPUT_SUPPORTED];
    uint32_t rstatus = 0;
#if 0 // DEV BLOCK 1 START
    char lbuf[SIZE_OF_MESSAGE_SHORT] = { 0 };
#endif // DEV BLOCK 1 END
// --- VAR END ---

    memset(command, 0, sizeof(command));
    memset(args, 0, sizeof(args));
    rstatus = command_and_args_from_input(latest_input, command, args);

// NEED TO CHECK return status of previous routine call here:
    rstatus = store_args_from(args);

#if 0 // // DEV BLOCK 1 START
    snprintf(lbuf, sizeof(lbuf), "parsed %u args from present input,\n\r", (argument_count + 0));
    printk_cli(lbuf);

    rstatus |= dev_show_args();
#endif // DEV BLOCK 1 END

    for ( int i = 0; i < ( sizeof(kd_command_set) / sizeof(struct cli_command_writers_api) ); i++ )
    {
        if ( strncmp(command, kd_command_set[i].token_to_represent_command, SIZE_COMMAND_TOKEN) == 0 )
        {
            rstatus = kd_command_set[i].handler(args);
        }
    }

    return rstatus;
}



uint32_t build_command_string(const char* latest_input, const struct device* callers_uart)
{
    uint32_t rstatus = 0;


// --- 1118 b DEV BEGIN ---
    if ( latest_input[0] == 0x08 )    // . . . <BACKSPACE> key 
    {
        delete_one_char_from_latest_command_string();
    }
//    else
//    {
//        flag_fresh_backspace_keypress = TRUE;
//    }


    if ( latest_input[0] == 0x2F )    // . . . '/' forward slash character
    {
        show_prompt_and_latest_command();
    }

// --- 1118 b DEV END ---


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - STEP - store and echo latest CLI character wise input
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// If latest character printable and not '\r' . . . append latest command string:
//    if ( (latest_input[0] >= 0x20) && (latest_input[0] < 0x7F) && (latest_input[0] != '\r') 
    else if ( (latest_input[0] >= 0x20) && (latest_input[0] < 0x7F) && (latest_input[0] != '\r') 
           && (latest_input[0] != 0x08) )
//       ~~~~~~~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~~
    {
        if ( index_within_cmd_token >= SIZE_COMMAND_TOKEN )
        {
            printk("Supported command length exceeded!\n");
            printk("Press <ENTER> to process or <ESC> to start over.\n");
        }
        else
        {
            latest_command[index_within_cmd_token] = latest_input[0];
            ++index_within_cmd_token;
            uart_poll_out(callers_uart, latest_input[0]);
        }
    }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - STEP - respond to supported control characters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// If latest character is <ESC> then clear the latest captured command string:

    else if ( latest_input[0] == 0x1B )    // . . . <ESCAPE> key 
    {
        clear_latest_command_string();
    }

#if 0
// --- 1118 b DEV BEGIN ---
    else if ( latest_input[0] == 0x08 )    // . . . <BACKSPACE> key 
    {
        delete_one_char_from_latest_command_string();
    }
// --- 1118 b DEV END ---
#endif

// If latest character is <ENTER> then call the command handler:

    else if ( latest_input[0] == '\r' )
    {
// --- 1115 DEV BEGIN ---
// bug fix, attempting to correct overwriting of CLI prompt and latest command:
//        uart_poll_out(callers_uart, latest_input[0]);
        if ( strlen(latest_input) > 1 )
        {
            printk_cli("\n\r");
        }
        else
        {
            uart_poll_out(callers_uart, latest_input[0]);
        }
// --- 1115 DEV END ---

        rstatus = command_handler(latest_command);
        clear_latest_command_string();
    }

    return rstatus;
} 


//----------------------------------------------------------------------
// - SECTION - string parsing routines
//----------------------------------------------------------------------

uint32_t store_args_from(const char* input)
{
    uint32_t rstatus = 0;
    uint32_t i = 0;          // 'i' indexes the length of passed input string
    uint32_t present_arg_size = 0;
    uint32_t present_arg_start = 0;
    uint32_t input_length = strlen(input);
// Note global 'argument_count' serves as our present argument index.

#ifdef DIAG_COMMAND_PARSING
#endif
#ifdef DIAG_STORE_ARGS
printk("- store_args_from - starting to parse and store args . . .\n");
#endif

    clear_argument_array();

#ifdef DIAG_STORE_ARGS
printk("- store_args_from - args array cleared, input has length %u,\n", input_length);
#endif

    while(
           ( i < input_length ) &&
           ( argument_count < MAX_COUNT_SUPPORTED_ARGS ) &&
           ( i < SIZE_COMMAND_INPUT_SUPPORTED ) &&
           ( present_arg_size < SUPPORTED_ARG_LENGTH )
         )
    {
// Skip leading white space at start of input string and after each token parsed:
        if ( (present_arg_size == 0) && (input[i] == 0x20) )
        {
//            i++;
        }
// Note the starting position of each successive argument:
        else if ( (present_arg_size == 0) && (input[i] != 0x20) )
        {
#ifdef DIAG_STORE_ARGS
printk("- store_args_from - found arg at %u . . .\n", i);
#endif
            present_arg_start = i;
            present_arg_size++;
        }

        else if ( (present_arg_size > 0) && (input[i] == 0x20) )
        {
#if 1
            for ( int j = present_arg_start; j < i; j++ )
            {
                 argument_array[argument_count][j - present_arg_start] = input[j];
            }
            argument_count++;
            present_arg_size = 0;
#else
int j = present_arg_start;
printk("- store_args_from - found arg from %u to %u in input,\n", j, i);
argument_count++;
present_arg_size = 0;
#endif
        }
        i++;
    } // end processing while loop

#ifdef DIAG_STORE_ARGS
printk("- store_args_from - reached end of processing loop,\n");
#endif

// If end of input not white space then we have a final arg to store after prior while construct:
    if ( present_arg_size > 0 )
    {
        for ( int j = present_arg_start; j < i; j++ )
        {
             argument_array[argument_count][j - present_arg_start] = input[j];
        }
        argument_count++;
    }


    if ( argument_count >= MAX_COUNT_SUPPORTED_ARGS )
        { rstatus = WARNING_MORE_ARGS_FOUND_THAN_SUPPORTED; }

    if ( i >= SIZE_COMMAND_INPUT_SUPPORTED )
        { rstatus = WARNING_COMMAND_INPUT_LONGER_THAN_SUPPORTED; }

    if ( present_arg_size >= SUPPORTED_ARG_LENGTH )
        { rstatus = WARNING_FOUND_ARG_LENGTH_LONGER_THAN_SUPPORTED; }

    return rstatus;
}



uint32_t arg_n(const uint32_t requested_arg, char* return_arg)
{
    uint32_t rstatus = ROUTINE_OK;

    if ( requested_arg <= argument_count )
    {
        strncpy(return_arg, argument_array[requested_arg], sizeof(argument_array[requested_arg]));
    }
    else
    {
        if ( argument_count == 0 )
            { rstatus = ERROR_NO_ARGS_PARSED; }
        else
            { rstatus = ERROR_TOO_FEW_ARGS_PARSED; }
    }

    return rstatus;
}



/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  @brief    Test whether argument contains only characters [0-9],
 *             when numeric convert to integer value.
 *
 *  @param    index_to_arg . . . index to tokenized argument from latest command line input
 *  @param    pointer to calling code memory space for return integer value
 *
 *  @note     Honoring shell return value convention, and with ease of summing series of test results:
 *  @return   0 when true
 *  @return   1 when false
 *
 *  https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */
uint32_t arg_is_decimal(const uint32_t index_to_arg, int* value_to_return)
{
    uint32_t tstatus = 0;  // test status
    uint32_t arg_len = strlen(argument_array[index_to_arg]);
    uint32_t multiplier = 1;

// Bounds checking:
    if (( index_to_arg >= 0 ) && ( index_to_arg < MAX_COUNT_SUPPORTED_ARGS ))
        { }
    else
        { return ERROR_CLI_ARGUMENT_INDEX_OUT_OF_RANGE; }

    for ( int i = 0; i < arg_len; i++ )
    {
        if ( ( argument_array[index_to_arg][i] < 0x30 ) || ( argument_array[index_to_arg][i] > 0x39 ) )
        {
            tstatus = RESULT_ARG_NOT_DECIMAL /* false result */;  i = arg_len /* kick out */;
        }
    }

// Note 0x30 is the ASCII value for the character zero '0':
    if ( tstatus == RESULT_ARG_IS_DECIMAL )
    {
        *value_to_return = 0;
        for ( int i = (arg_len - 1); i >= 0; i-- )
        {
            *value_to_return += ( (argument_array[index_to_arg][i] - 0x30) * multiplier );
            multiplier *= 10;
        }
    }

    return tstatus;
}



uint32_t arg_is_hex(const uint32_t arg, int* value_to_return)
{
    return 0;
}



uint32_t dec_value_at_arg_index(const uint32_t index_to_arg)
{
    uint32_t multiplier = 1;
    uint32_t value_to_return = 0;

// Bounds checking:
    if (( index_to_arg >= 0 ) && ( index_to_arg < MAX_COUNT_SUPPORTED_ARGS ))
        { }
    else
        { return ERROR_CLI_ARGUMENT_INDEX_OUT_OF_RANGE; }

// Note 0x30 is the ASCII value for the character zero '0':
    uint32_t arg_len = strlen(argument_array[index_to_arg]);
    {
        value_to_return = 0;
        for ( int i = (arg_len - 1); i >= 0; i-- )
        {
            value_to_return += ( (argument_array[index_to_arg][i] - 0x30) * multiplier );
            multiplier *= 10;
        }
    }

    return value_to_return;
}



uint32_t dev_show_args(void)
{

    if ( argument_count < 1 )
    {
        printk("- dev-show-args - no arguments parsed from latest command input.\n");
    }
    else
    {
        printk("- dev-show-args -\nPresent args:  ");
        for ( int i = 0; i < argument_count; i++ )
        {
            printk("%s, ", argument_array[i]);
        }
        printk("\n");
    }

    return 0;
}




//----------------------------------------------------------------------
// - SECTION - command handlers
//----------------------------------------------------------------------

// *************************************************************************************************
// - ADD COMMANDS - STEP 3:  add CLI routine definitions here
// *************************************************************************************************

/*
 *  @Brief   Accept integer values 0-9 and map those to iis2dh Output
 *           Data Rate bit-wise flags, expressed in iis2dh-registers.h.
 */

uint32_t output_data_rate_handler(const char* args)
{
    uint32_t rstatus = 0;
    uint32_t new_data_rate = 0;
    char lbuf[SIZE_OF_MESSAGE_MEDIUM] = { 0 };
    enum iis2dh_output_data_rates_e new_rate = ODR_0_POWERED_DOWN;

//    printk_cli("2021-10-25 ODR stub function\n\r");

    if ( argument_count > 0 )
    {
        rstatus = arg_is_decimal(0, &new_data_rate);

#if 0 // DEV BLOCK BEGIN
        snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "- output_data_rate_handler - test first arg decimal yields %u,\n\r", rstatus);
        printk_cli(lbuf);
        snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "- output_data_rate_handler - arg1 as integer = %u,\n\r", new_data_rate);
        printk_cli(lbuf);
#endif // DEV BLOCK END

        if ( rstatus == RESULT_ARG_IS_DECIMAL )
        {
            if ( (new_data_rate >= LOWEST_DATA_RATE_INDEX) && (new_data_rate <= HIGHEST_DATA_RATE_INDEX) )
            {
                switch (new_data_rate)
                {
                    case 0: new_rate = ODR_0_POWERED_DOWN; break;
                    case 1: new_rate = ODR_1_HZ; break;
                    case 2: new_rate = ODR_10_HZ; break;
                    case 3: new_rate = ODR_25_HZ; break;
                    case 4: new_rate = ODR_50_HZ; break;

                    case 5: new_rate = ODR_100_HZ; break;
                    case 6: new_rate = ODR_200_HZ; break;
                    case 7: new_rate = ODR_400_HZ; break;
                    case 8: new_rate = ODR_1620_HZ_IN_LOW_POWER_MODE; break;
                    case 9: new_rate = ODR_5376_HZ_IN_LOW_POWER_MODE; break;

                    default: 
                            new_rate = ODR_0_POWERED_DOWN;
                }

                printk("- output_data_rate_handler - calling scoreboard to post new requested data rate...\n");
                rstatus = scoreboard_set_requested_iis2dh_odr(new_rate);
            }
            else
            {
                printk("- output_data_rate_handler - WARNING! requested output data rate out of range,\n");
            }

        } // end scope to handle when first arg is integer value

    } // end scope to handle when there are one or more arguments
    else
    {
        enum iis2dh_output_data_rates_e present_odr_flags;
        rstatus = scoreboard_get_requested_iis2dh_odr(&present_odr_flags);
        snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "\n\rpresent ODR flags = %u,\n\r",
          (uint32_t)present_odr_flags);
        printk_cli(lbuf);
    }

    return rstatus;
}



uint32_t cli__help_message(const char* args)
{
#define WIDTH_OF_BULLET_POINT (3)
#define WIDTH_OF_COMMAND_TOKEN_OR_NAME (12)
#define WIDTH_OF_COMMAND_DESCRIPTION (80)

    char lbuf[SIZE_OF_MESSAGE_MEDIUM];

//    printk_cli("Kionix demo CLI commands:\n\r\n\r");
    printk_cli("\n\rKionix demo CLI commands:\n\r\n\r");

    for ( int i = 0; i < implemented_command_count; i++ )
    {
        snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, " %*u)  %s%*s   . . . %s\n\r",
                   WIDTH_OF_BULLET_POINT,
                   (i + 1),
                   kd_command_set[i].token_to_represent_command,
                   (WIDTH_OF_COMMAND_TOKEN_OR_NAME - strlen(kd_command_set[i].token_to_represent_command)),
                   " ",
                   kd_command_set[i].description
                 );

        printk_cli(lbuf);
    }
    printk_cli("\n\r");
    return 0;
} 



#if 0
uint32_t banner_message(const char* args)
{
    printk_cli("\n--\n\r");
    printk_cli("-- Kionix Driver Demo\n\r");
    printk_cli("-- A small Zephyr RTOS 2.6.0 based app to exercise Kionix KX132-1211 accelerometer\n\r");
    printk_cli("--\n\r");
    return 0;
}
#endif



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

    unsigned int i = 0;
// --- VAR END ---


// 'uart2' is a DTS node for Thingy91 and Sparkfun nRF9160 dev boards,
// but is a node alias for lpc55x69 eval board per DTS overlay by TMH:
//    uart_for_cli = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
    uart_for_cli = device_get_binding(DT_LABEL(DT_ALIAS(uart2)));

    if ( uart_for_cli == NULL )
    {   
        dmsg("Failed to assign pointer to UART2 device!\n", PROJECT_DIAG_LEVEL);
    }
    else
    {   
        dmsg("succeeded in getting device binding for UART2!\n", PROJECT_DIAG_LEVEL);
    }

    initialize_command_handler();

    while (1)
    {
        if ( uart_for_cli != NULL )
        {
            memset(lbuf, 0, sizeof(lbuf));
            msg = lbuf;
            uart_poll_in(uart_for_cli, msg);

            if ( strlen(msg) > 0 )
            {
                build_command_string(msg, uart_for_cli);
            }
        }

        k_msleep(SLEEP_TIME__SIMPLE_CLI__MS);

#if 0 // 2022-09-26 MON SANITY CHECK . . .
        i += 1;
        if ( i > 149 )
        {
            i = 0;
            printk_cli("--- MARK 1 ---\n\r");
        }
#endif
    }

} // end of thread entry point routine



//----------------------------------------------------------------------
// - SECTION - set aside
//----------------------------------------------------------------------


// From command_handler():

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
#endif

#if 0
    printk("parsed command '%s',\n", command);
    printk("and arguments '%s'\n", args);
    printk("checking %u implemented Kionix demo commands...\n", ((sizeof(kd_command_set) / sizeof(struct cli_command_writers_api)) - 0));
#endif




#if 0
// Formatting example from cli__help_message routine:

    printk_cli("Kionix demo CLI commands:\n\r\n\r");
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
#endif



// --- EOF ---
