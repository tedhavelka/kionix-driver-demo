//
// File:  cli-zephyr-stack-info.c
//
//
//


#include <zephyr.h>         // . . .

#include <stdint.h>         // to provide define of uint32_t

//#include <include/debug/thread_analyzer.h>
//#include <thread_analyzer.h>


#include "../../zephyr/include/debug/thread_analyzer.h"

#include "return-values.h"


uint32_t cli__zephyr_2p6p0_stack_statistics(const char* args)
{
// void thread_analyzer_print 	( 	void  		)

    thread_analyzer_print();

    return ROUTINE_OK;
}
