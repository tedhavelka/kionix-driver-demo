//----------------------------------------------------------------------
//
//  Project:  Kionix Driver Demo
//
//     File:  cli-zephyr-stack-info.c
//
//
//  Note struct thread_analyzer_info defined in file
//  './include/debug/thread_analyzer.h'
//
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// - SECTION - includes
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
#include "common.h"
#include "return-values.h"
#include "diagnostic.h"
#include "thread-simple-cli.h"


//----------------------------------------------------------------------
// - SECTION - includes
//----------------------------------------------------------------------

#define WIDTH_THREAD_NAME (24)



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

// - 1112 DEV - we need to call Zephyr's `thread_analyzer_run()` with a pointer
// - to a routine of the form:
// -
// -    void thread_print_cb(struct thread_analyzer_info *info)

void kd_thread_print_cb(struct thread_analyzer_info *info)
{
    char lbuf[SIZE_OF_MESSAGE_MEDIUM] = { 0 };
    static uint32_t exec_count = 1;
#if 0
    snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "%u)  '%s' stack size %u bytes, stack used %u bytes\n\r",
      exec_count, info->name, info->stack_size, info->stack_used);
    printk_cli(lbuf);
#else
    snprintf(lbuf, SIZE_OF_MESSAGE_MEDIUM, "   %u) '%s' %*sstack size %u bytes, stack used %u bytes, %u%%\n\r",
      (exec_count + 0),
      info->name,
      (WIDTH_THREAD_NAME - strlen(info->name)),  // <-- this arg and next used with %*s format specifier
      " ",
      info->stack_size,
      info->stack_used,
      ((100 * info->stack_used) / info->stack_size)
    );
    printk_cli(lbuf);
#endif
    exec_count++;
}



uint32_t cli__zephyr_2p6p0_stack_statistics(const char* args)
{
    if ( strlen(args) == 0 ) { } // trivial test to start, to avoid compiler warning - TMH

    printk_cli(TWO_NEWLINES);
//    thread_analyzer_print();  // <-- this routine prints to Zephyr app's preferred console UART
    thread_analyzer_run(kd_thread_print_cb);

    return ROUTINE_OK;
}



// --- EOF ---
