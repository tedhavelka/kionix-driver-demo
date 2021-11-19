/*
 *  Project:  Kionix Driver Demo (Zephyr RTOS based app)
 *
 *  File:  conversions.c
 *
 *  Based on 2016 Intel Corporation sample Zephyr app, "basic/blinky"
 *
 *  Copyright (c) 2021 Neela Nurseries
 *
 *  SPDX-License-Identifier: Apache-2.0
 */



//---------------------------------.------------------------------------
// - SECTION - pound includes
//---------------------------------^------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                // to provide memset()
#include <stdint.h>                // to provide define of uint32_t


// App specific includes . . .
#include "conversions.h"
#include "diagnostic.h"

// this source file part of 'simple CLI' module in app:
#include "thread-simple-cli.h"     // to provide prototype for printk_cli()



/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *  @brief  Routine to convert integer up to 32-bits in length to
 *          string in binary format.  Caller sends pointer to memory
 *          of formatting string, and length of that memory.
 *
 *  @param  integer
 *  @param  string pointer
 *  @param  str_length
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

void integer_to_binary_string(const uint32_t integer, char* string, const uint32_t str_length)
{

    uint32_t masking_bit = 1;
    uint32_t shift_value = 0;
    uint32_t maximum_shift = 0;
#define DEV__BINARY_FORMATTING_DEBUG 0
#if DEV__BINARY_FORMATTING_DEBUG == 1
    char lbuf[DEFAULT_MESSAGE_SIZE] = { 0 };
#endif

    (str_length > 32) ? (maximum_shift = 32) : (maximum_shift = str_length);

#if DEV__BINARY_FORMATTING_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "\n\rready to convert integer up to %u bits wide,\n\r",
      maximum_shift);
    printk_cli(lbuf);
#endif
    for ( shift_value = 0; shift_value < maximum_shift; shift_value++ )
    {
        masking_bit = ( 1 << shift_value );
        if ( integer & masking_bit )
        {
            string[(str_length - shift_value - 1)] = 0x31;
        }
        else
        {
            string[(str_length - shift_value - 1)] = 0x30;
        }
    }

    string[str_length] = 0;

#if DEV__BINARY_FORMATTING_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "integer %u equals '%s'.\n\r",
      integer, string);
    printk_cli(lbuf);
#endif
}



// --- EOF ---
