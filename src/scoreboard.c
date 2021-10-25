

#include <stdint.h>         // to provide define of uint32_t

#include <sys/printk.h>     // to provide printk() function

#include "diagnostic.h"

#include "scoreboard.h"



static uint32_t flag__cmd_a = 0;
static uint32_t global_test_value = 0;
static enum nn_diagnostic_levels global_diag_messaging_level = DIAG_NORMAL;



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

uint32_t set_global_test_value(const uint32_t passed_value)
{
    printk("- scoreboard - in setter function test val holds %u,\n", global_test_value);

    global_test_value = passed_value;

    printk("- scoreboard - setting test val to  %u,\n", global_test_value);

    return 0;
}


uint32_t get_global_test_value(uint32_t* value_to_return)
{
    *value_to_return = global_test_value;
    return 0;
}



uint32_t update_diag_messaging_level(const enum nn_diagnostic_levels passed_value)
{
    global_diag_messaging_level = passed_value;
    return 0;
}


uint32_t get_diag_messaging_level(enum nn_diagnostic_levels* value_to_return)
{
    *value_to_return = global_diag_messaging_level;
    return 0;
}






// --- EOF ---
