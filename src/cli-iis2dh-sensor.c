/*
 *  part of Kionix Driver Demo
 */



//---------------------------------.------------------------------------
// - SECTION - pound includes
//---------------------------------^------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                // to provide memset()
#include <stdint.h>                // to provide define of uint32_t

// app and development supporting:
#include "diagnostic.h"
#include "module-ids.h"
#include "return-values.h"
#include "kionix-demo-errors.h"

// sensor specific (this CLI command deals with STMicro IIS2DH):
#include "thread-iis2dh.h"

// this source file part of 'simple CLI' module in app:
#include "thread-simple-cli.h"     // to provide prototype for printk_cli()



//----------------------------------------------------------------------
// - SECTION - global variables
//----------------------------------------------------------------------

// extern uint32_t argument_count;



//---------------------------------.------------------------------------
// - SECTION - routines
//---------------------------------^------------------------------------

uint32_t iis2dh_sensor_handler(const char* args)
{

// --- VAR BEGIN ---
    uint32_t rstatus = ROUTINE_OK;
    char lbuf[DEFAULT_MESSAGE_SIZE];
    uint32_t argument_count = argument_count_from_cli_module();

// for generalized command parsing:
    uint32_t match = 0;
    char argument[SUPPORTED_ARG_LENGTH];
    uint32_t placeholder_decimal_value = 0;

// for iis2dh command to read or write register, fixed at one byte read or sent:
    uint32_t type_of_arg = 0;
    uint32_t control_register = 0;
    uint32_t config_value = 0;

    uint8_t value_of_register = 0;

// --- VAR END ---


    printk_cli("\n\r\n\r- DEV 1117b - iis2dh stub function (now factored to separate source file)\n\r");

// - NOTE 1 - <--dev code removed from this point

//        rstatus = arg_n(0, argument);
//        match += strncmp(argument, "read", SUPPORTED_ARG_LENGTH);

#define STRNCMP_ARGUMENT(index_to_argument, pattern, up_to_byte_length) \
rstatus = arg_n(index_to_argument, argument); \
match += strncmp(argument, pattern, up_to_byte_length);

#define CHECK_IF_DECIMAL_AT_ARG_INDEX(n) \
match += arg_is_decimal(n, &placeholder_decimal_value);

// 2021-11-18 - check for input of the form "iis2dh read n from n":

    if ( argument_count == 4 )
    {
        STRNCMP_ARGUMENT(0, "read", SUPPORTED_ARG_LENGTH);
//        match += arg_is_decimal(1, &placeholder_decimal_value);
        CHECK_IF_DECIMAL_AT_ARG_INDEX(1);
        STRNCMP_ARGUMENT(2, "from", SUPPORTED_ARG_LENGTH);
        CHECK_IF_DECIMAL_AT_ARG_INDEX(3);

        snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "checking arguments for `read n from reg` command, match = %u\n\r",
          match);
        printk_cli(lbuf);
        if ( match == 0 )
        {
            printk_cli("match == 0, ready to proceed with command,\n\r");
        }

    }    

// --- CMD FORM BEGIN - read register one byte fixed ---

// Prepare for register read, fixed at reading back one byte:

    type_of_arg = arg_is_decimal(0, &control_register);
    if ( type_of_arg == RESULT_ARG_IS_DECIMAL )
    {
        snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "parsed sensor register addr %u\n\r", control_register);
        printk_cli(lbuf);
    }
    else
    {
        rstatus = ERROR_UNEXPECTED_ARGUMENT_TYPE;
        printk_cli("- INPUT ERROR - got invalid register address!\n\r");
    }

    type_of_arg = arg_is_decimal(1, &config_value);
    if ( type_of_arg == RESULT_ARG_IS_DECIMAL )
    {
        snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "parsed configuration value of %u\n\r", config_value);
        printk_cli(lbuf);
    }
    else
    {
        rstatus = ERROR_UNEXPECTED_ARGUMENT_TYPE;
        printk_cli("- INPUT ERROR - got invalid configuration register value!\n\r");
    }


    if ( rstatus == ROUTINE_OK )
    {
        printk_cli("- DEV SUMMARY - reading to send updating config value to sensor.\n\r");

        if ( argument_count == 1 )
        {
            dev__thread_iis2dh__set_one_shot_message_flag();
            rstatus = wrapper_iis2dh_register_read(control_register, &value_of_register);
            snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "register 0x%02X holds %u,\n\r",
              (control_register & 0xFF), (value_of_register & 0xFF));
            printk_cli(lbuf);
        }
    }
    else
    {
        printk_cli("- DEV SUMMARY - SKIPPING READ OF IIS2DH REGISTER!\n\r");
    }
// --- CMD FORM END - read register one byte fixed ---


    printk_cli("- 1117b DEVELOPMENT UNDERWAY -- returning early . . .\n\r");
    return rstatus;

} // end routine 




//----------------------------------------------------------------------
// - SECTION - notes
//----------------------------------------------------------------------

#if 0 // NOTE 1

    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "- DEV 1117b - working with args '%s',\n\r", args);
    printk_cli(lbuf);

    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "present argument count is '%u',\n\r", argument_count);
    printk_cli(lbuf);

    rstatus = arg_n(0, argument);
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "argument 1 is '%s'\n\r", argument);
    printk_cli(lbuf);

    rstatus = arg_n(1, argument);
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "argument 2 is '%s'\n\r", argument);
    printk_cli(lbuf);

#endif



// --- EOF ---
