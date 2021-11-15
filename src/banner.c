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



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

uint32_t latest_version_string(char* callers_buffer)
{
/*
#define KD_VERSION_NUMBER_MAJOR    1
#define KD_VERSION_NUMBER_MINOR    0
#define KD_VERSION_NUMBER_BRANCH
*/

    if ( callers_buffer != NULL )
    {
        snprintf(callers_buffer, KD_VERSION_STRING_LENGTH, "Kionix Driver Demo v%u.%u.%u",
          KD_VERSION_NUMBER_MAJOR, KD_VERSION_NUMBER_MINOR, KD_VERSION_NUMBER_BRANCH);
    }

    printk_cli(callers_buffer);

    return ROUTINE_OK;
}



uint32_t cli__kd_version(const char* args)
{
    char lbuf[KD_VERSION_STRING_LENGTH] = { 0 };
    uint32_t rstatus = ROUTINE_OK;

    rstatus = latest_version_string(lbuf);
//    printk_cli("Kionix Driver Demo version:  ");
//    printk_cli(lbuf);
    printk_cli("\n\r\n\r");

    return rstatus;
}



// --- EOF ---
