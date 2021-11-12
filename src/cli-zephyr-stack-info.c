//----------------------------------------------------------------------
//
//  Project:  Kionix Driver Demo
//
//     File:  cli-zephyr-stack-info.c
//
//----------------------------------------------------------------------


#include <zephyr.h>         // . . .

#include <stdint.h>         // to provide define of uint32_t

// See project top level CMakeLists.txt file for `include_directory()` to provide relative
// path of Zephyr thread analyzer header file:
//#include <include/debug/thread_analyzer.h>  // <-- described as needed in Zephyr Doc' v2.7.99
#include <thread_analyzer.h>

#include "return-values.h"



uint32_t cli__zephyr_2p6p0_stack_statistics(const char* args)
{
// void thread_analyzer_print 	( 	void  		)

    thread_analyzer_print();

    return ROUTINE_OK;
}



// --- EOF ---
