/*
 *  Project:  Kionix Driver Demo (Zephyr RTOS based app)
 *
 *  File:  cli-zephyr-kernel-timing.c
 *
 *  SPDX-License-Identifier: Apache-2.0
 */


//----------------------------------------------------------------------
// - SECTION - pound includes
//---------------------------------^------------------------------------

#include <stdint.h>                // to provide define of uint32_t
#include <stdio.h>                 // to provide snprintf() and related

//#include <include/kernel.h>        // to provide k_cycle_get_32()
// 2021-11-16 Stanza added to CMakeLists.txt file "include_directories(../zephyr/include)" enables:
#include <kernel.h>                // to provide k_cycle_get_32()

#include "common.h"                //
#include "diagnostic.h"            //
#include "return-values.h"         //
#include "thread-simple-cli.h"     // to provide prototype for printk_cli()



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

uint32_t return_zephyr_kernel_runtime_cycle_count(uint32_t* kernel_cycle_count)
{
    *kernel_cycle_count = k_cycle_get_32();
    return ROUTINE_OK;
};


uint32_t cli__show_zephyr_kernel_runtime_cycle_count(const char* args)
{
    char lbuf[SIZE_OF_MESSAGE_MEDIUM] = { 0 };
    uint32_t cycle_count = 0;

    cycle_count = k_cycle_get_32();
    snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "%sZephyr kernel at %u cycles\n\r",
      ONE_NEWLINE, cycle_count);
    printk_cli(lbuf);

    return ROUTINE_OK;
};



// --- EOF ---
