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
#include "common.h"
#include "conversions.h"
#include "diagnostic.h"
#include "module-ids.h"
#include "return-values.h"
#include "scoreboard.h"

#include "kionix-demo-errors.h"

// sensor specific (this CLI command deals with STMicro IIS2DH):
#include "thread-iis2dh.h"

// this source file part of 'simple CLI' module in app:
#include "thread-simple-cli.h"     // to provide prototype for printk_cli()



//----------------------------------------------------------------------
// - SECTION - file scoped variables
//----------------------------------------------------------------------

// extern uint32_t argument_count;

enum iis2dh_flexible_commands
{
    KD__IIS2DH_CMD__FIRST_ENUM_ELEMENT,
    KD__IIS2DH_CMD__FROM_REG_READ_ONE_BYTE,
    KD__IIS2DH_CMD__FROM_REG_READ_MULTIPLE_BYTES,
    KD__IIS2DH_CMD__TO_REG_WRITE_ONE_BYTE,

    KD__IIS2DH_CMD__COUNT_FLEXIBLE_CMD_FORMS
};



//---------------------------------.------------------------------------
// - SECTION - routines
//---------------------------------^------------------------------------

uint32_t cli__iis2dh_sensor_handler(const char* args)
{

// --- VAR BEGIN ---
    uint32_t rstatus = ROUTINE_OK;
    char lbuf[DEFAULT_MESSAGE_SIZE];
    char binary_rep[BINARY_REPRESENTATION_EIGHT_BITS_AS_STRING];
    uint32_t argument_count = argument_count_from_cli_module();

// for generalized command parsing:
    uint32_t match = 0;
    char argument[SUPPORTED_ARG_LENGTH];
    uint32_t placeholder_decimal_value = 0;
    enum iis2dh_flexible_commands command_to_execute = KD__IIS2DH_CMD__FIRST_ENUM_ELEMENT;

// for iis2dh command to read or write register, fixed at one byte read or sent:
    uint32_t type_of_arg = 0;
    uint32_t control_register = 0;
    uint8_t value_of_register = 0;
    uint32_t config_value = 0;

// --- VAR END ---


// - NOTE 1 - <--dev code removed from this point

// GENERALIZED COMMAND PARSING MACROS - ALLOW FOR CERTAIN ONE-LINE ARGUMENT CHECKS:
// (Note:  these macros depend on local variables in 'generalized command parsing' section top of this routine.)

#define STRNCMP_ARGUMENT(index_to_argument, pattern, up_to_byte_length) \
rstatus = arg_n(index_to_argument, argument); \
match += strncmp(argument, pattern, up_to_byte_length);

#define CHECK_IF_DECIMAL_AT_ARG_INDEX(n) \
match += arg_is_decimal(n, &placeholder_decimal_value);


    if ( argument_count == 1 )
    {
// Check for valid command to read one fixed byte from iis2dh config register
        CHECK_IF_DECIMAL_AT_ARG_INDEX(1);
        if ( match == 0 )
            { command_to_execute = KD__IIS2DH_CMD__FROM_REG_READ_ONE_BYTE; }
    }

    else if ( argument_count == 3 )
    {
//        printk_cli("checking for `iis2dh write <reg> <data>` command...\n\r");
        STRNCMP_ARGUMENT(0, "write", SUPPORTED_ARG_LENGTH);
        CHECK_IF_DECIMAL_AT_ARG_INDEX(1);   // <-- here expect peripheral register address
        CHECK_IF_DECIMAL_AT_ARG_INDEX(2);   // <-- here expect byte value to write to peripheral register
        if ( match == 0 )
            { command_to_execute = KD__IIS2DH_CMD__TO_REG_WRITE_ONE_BYTE; }
//        else
//            { printk_cli("iis2dh write command not found.\n\r"); }
    }

// 2021-11-18 - check for input of the form "iis2dh read n from n":
    else if ( argument_count == 4 )
    {
        STRNCMP_ARGUMENT(0, "read", SUPPORTED_ARG_LENGTH);
        CHECK_IF_DECIMAL_AT_ARG_INDEX(1);
        STRNCMP_ARGUMENT(2, "from", SUPPORTED_ARG_LENGTH);
        CHECK_IF_DECIMAL_AT_ARG_INDEX(3);

        if ( match == 0 )
            { command_to_execute = KD__IIS2DH_CMD__FROM_REG_READ_MULTIPLE_BYTES; }
    }


    switch (command_to_execute)
    {
        case KD__IIS2DH_CMD__FROM_REG_READ_MULTIPLE_BYTES:
        {
#define LOCAL_CAP_ON_VALUES_CAPTURED 24
            uint8_t register_values[LOCAL_CAP_ON_VALUES_CAPTURED];

#if 1
            uint32_t count_bytes_to_read = (
              dec_value_at_arg_index(1) <= LOCAL_CAP_ON_VALUES_CAPTURED ?
              dec_value_at_arg_index(1) :
              LOCAL_CAP_ON_VALUES_CAPTURED
            );

//            dev__thread_iis2dh__set_one_shot_message_flag();

            rstatus = wrapper_iis2dh_register_read_multiple(
                                                             dec_value_at_arg_index(3),
                                                             register_values,
                                                             count_bytes_to_read
                                                           );
#endif
            if ( count_bytes_to_read == 1 )
            {
memset(binary_rep, 0, BINARY_REPRESENTATION_EIGHT_BITS_AS_STRING);
                integer_to_binary_string(register_values[0], binary_rep, BINARY_REPRESENTATION_EIGHT_BITS_AS_STRING);
                snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "\n\rregister 0x%02X holds 0x%02X, equal to %u and 0b%s\n\r",
                  dec_value_at_arg_index(3), register_values[0], register_values[0], binary_rep);
                printk_cli(lbuf);
            }
            else
            {
                snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "\n\rbytes read back from IIS2DH register 0x%02X:\n\r",
                  dec_value_at_arg_index(3));
                printk_cli(lbuf);
                for ( int i = 0; i < count_bytes_to_read; i++ )
                {
// two modulo tests provide white space to highlight accelerometer x,y,z triplet data:
                    if ( ( i % 2 ) == 0 ) { printk_cli(" "); }
                    if ( ( i % 6 ) == 0 ) { printk_cli(" "); }
                    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, " 0x%02X", register_values[i]);
                    printk_cli(lbuf);
                }
            }

            printk_cli(ONE_NEWLINE);

            break;
        }
 

        case KD__IIS2DH_CMD__TO_REG_WRITE_ONE_BYTE:
        {
#if 1
            printk_cli("\n\rcalling wrapper routine to write config register . . .\n\r");
            dev__thread_iis2dh__set_one_shot_message_flag();
#endif
            rstatus = wrapper_iis2dh_register_write(
                                                     dec_value_at_arg_index(1),
                                                     dec_value_at_arg_index(2)
                                                   );
            break;
        }


        default:
            break;
    }    


    return rstatus;

} // end routine cli__iis2dh_sensor_handler()



uint32_t cli__request_temperature_reading(const char* args)
{
// --- VAR BEGIN ---
    uint32_t rstatus = ROUTINE_OK;
    char lbuf[DEFAULT_MESSAGE_SIZE];
// --- VAR END ---

    scoreboard__update_flag__temperature_reading_requested(SET_FLAG);

    return rstatus;
}



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
