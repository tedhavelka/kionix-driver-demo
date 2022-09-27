/*
 * Copyright (c) 2021 Neela Nurseries
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <sys/printk.h>

#include "diagnostic.h"

#include "thread-simple-cli.h"



/*
 *----------------------------------------------------------------------
 * @brief     Wrapper function about Zephyr printk().
 * @purpose   To provide easy way to disable messages on chosen UART
 *            peripheral for Zephyr console, shell-uart, uart-mcumgr
 *            and similar.           
 *----------------------------------------------------------------------
 */

void dmsg(const char* message, int option)
{

#if 0
    if ( option | KD_DIAG_OPTION_TO_DEFAULT_UART )
    {
        printk("%s", message);
    }
#endif

//    if ( option | KD_DIAG_OPTION_TO_CLI_UART )
    if ( 1 )
    {
       printk_cli(message);
    }

#if 0
    switch (option)
    {
       case ( option | KD_DIAG_OPTION_TO_DEFAULT_UART ):
           printk("%s", message);
           break;

       case ( option | KD_DIAG_OPTION_TO_CLI_UART ):
           printk_cli(message);
           break;

       default:
    }
#endif

}




// --- EOF ---
