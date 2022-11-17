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
 *  @Brief   routine to convert eight-bit two's compliment reading
 *           into a value in units of G, gravitational force at surface
 *           of Earth.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

float reading_in_g(const uint32_t reading_in_twos_comp, const uint32_t full_scale, const uint32_t resolution_in_bits)
{
    uint32_t reading = reading_in_twos_comp;
    uint32_t reading_bounded = (reading & 0xFF);
    float reading_as_float = 0.0;
    float scale_by_value = ( ( 2.0 * APPR_ACCELERATION_OF_GRAVITY ) / 128.0 );
    (void)full_scale;
    (void)resolution_in_bits;

#if 0
#include <sys/printk.h>            // to provide printk() function
//printk("- zzz - converting reading value %u to float value in units of G,\n", reading_in_twos_comp);
printk(" - PASSED VALUE %u -", reading_in_twos_comp);
#endif

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
#define DEV__BINARY_FORMATTING_STRING_INDEXING 0

#if DEV__BINARY_FORMATTING_DEBUG == 1 || DEV__BINARY_FORMATTING_STRING_INDEXING == 1
    char lbuf[DEFAULT_MESSAGE_SIZE] = { 0 };
#endif

//    (str_length > 32) ? (maximum_shift = 32) : (maximum_shift = str_length);
    (str_length > 32) ? (maximum_shift = 32) : (maximum_shift = str_length - 1);

#if DEV__BINARY_FORMATTING_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "\n\rready to convert integer up to %u bits wide,\n\r",
      maximum_shift);
    printk_cli(lbuf);
#endif

// place string terminating zero at end of caller's string memory:
    string[(str_length - 1)] = 0;

#if DEV__BINARY_FORMATTING_STRING_INDEXING == 1
        printk_cli("\n\r");
#endif
    for ( shift_value = 0; shift_value < maximum_shift; shift_value++ )
    {
        masking_bit = ( 1 << shift_value );

#if DEV__BINARY_FORMATTING_STRING_INDEXING == 1
        snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "writing binary string index %u,\n\r",
          (str_length - shift_value - 2) );
        printk_cli(lbuf);
#endif

        if ( integer & masking_bit )
        {
            string[(str_length - shift_value - 2)] = 0x31;
        }
        else
        {
            string[(str_length - shift_value - 2)] = 0x30;
        }
    }

//    string[str_length] = 0;

#if DEV__BINARY_FORMATTING_DEBUG == 1
    snprintf(lbuf, DEFAULT_MESSAGE_SIZE, "integer %u equals '%s'.\n\r",
      integer, string);
    printk_cli(lbuf);
#endif
}



// --- EOF ---
