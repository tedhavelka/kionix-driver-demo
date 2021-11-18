/*
 *  @Project Kionxi Driver Demo
 *
 *  @File
 *
 *  @Copyright (c) 2021 Neela Nurseries
 *
 *  @SPDX-License-Identifier: Apache-2.0
 */


#include <stdint.h>         // to provide define of uint32_t
#include <stdio.h>          // to provide snprintf() and related

#include "version.h"
#include "thread-simple-cli.h"
#include "return-values.h"
#include "routine-options.h"



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

uint32_t latest_version_string(char* callers_buffer, uint32_t option)
{
    if ( callers_buffer != NULL )
    {
        snprintf(callers_buffer, KD_VERSION_STRING_LENGTH, "KD Demo v%u.%u.%u",
          KD_VERSION_NUMBER_MAJOR, KD_VERSION_NUMBER_MINOR, KD_VERSION_NUMBER_BRANCH);
    }

    if ( option == KD_ROUTINE_OPTION__RETURN_AND_DISPLAY_APP_VERSION )
    {
        printk_cli(callers_buffer);
    }

    return ROUTINE_OK;
}



uint32_t cli__kd_version(const char* args)
{
    char lbuf[KD_VERSION_STRING_LENGTH] = { 0 };
    uint32_t rstatus = ROUTINE_OK;

    rstatus = latest_version_string(lbuf, KD_ROUTINE_OPTION__RETURN_AND_DISPLAY_APP_VERSION);
    printk_cli("\n\r\n\r");

    return rstatus;
}



uint32_t cli__banner_message(const char* args)
{
    uint32_t rstatus = ROUTINE_OK;
    char lbuf[KD_VERSION_STRING_LENGTH] = { 0 };

    printk_cli("\r\n**------------------------------------------------------------------------------------------------**\n\r");
    printk_cli("**  Kionix Driver Demo\n\r**  ");
    rstatus = latest_version_string(lbuf, KD_ROUTINE_OPTION__RETURN_APP_VERSION);
    printk_cli(lbuf);
    printk_cli("\n\r**  A small Zephyr RTOS 2.6.0 based app to exercise Kionix KX132-1211 accelerometer\n\r");
    printk_cli("**------------------------------------------------------------------------------------------------**\n\r\r\n");

    return rstatus;
}




// --- EOF ---
