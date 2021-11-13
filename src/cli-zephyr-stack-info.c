//----------------------------------------------------------------------
//
//  Project:  Kionix Driver Demo
//
//     File:  cli-zephyr-stack-info.c
//
//----------------------------------------------------------------------


// Standard C includes from newlibc:
#include <stdint.h>         // to provide define of uint32_t
#include <string.h>         // to provide strlen()
#include <stdio.h>          // to provide snprintf() and related

// Zephyr RTOS project includes:
#include <sys/printk.h>

// See project top level CMakeLists.txt file for `include_directory()` to provide relative
// path of Zephyr thread analyzer header file:
//#include <include/debug/thread_analyzer.h>  // <-- described as needed in Zephyr Doc' v2.7.99
#include <thread_analyzer.h>

// Kionix Driver Demo headers:
#include "return-values.h"
#include "diagnostic.h"
#include "thread-simple-cli.h"



// - 1112 DEV - we need to call Zephyr's `thread_analyzer_run()` with a pointer
// - to a routine of the form:
// -
// -    void thread_print_cb(struct thread_analyzer_info *info)

void kd_thread_print_cb(struct thread_analyzer_info *info)
{
    char lbuf[SIZE_OF_MESSAGE_MEDIUM] = { 0 };

    print_cli("------------------------------------------------------------------------\n\r");
    snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "stack size %u\n\rstack used %u\n\r",
      info->stack_size, info->stack_used);
    print_cli(lbuf);
    print_cli("------------------------------------------------------------------------\n\r");
}



uint32_t cli__zephyr_2p6p0_stack_statistics(const char* args)
{
// void thread_analyzer_print(void)
printk("ZZTOP\n\r");

    if ( strlen(args) == 0 ) { } // trivial test to start, to avoid compiler warning - TMH

    thread_analyzer_print();

    return ROUTINE_OK;
}



// --- EOF ---
