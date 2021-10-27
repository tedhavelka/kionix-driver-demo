

#include <stdint.h>         // to provide define of uint32_t

#include <sys/printk.h>     // to provide printk() function

#include "diagnostic.h"
#include "scoreboard.h"
//#include "iis2dh-registers.h"



//static uint32_t flag__cmd_a = 0;
static uint32_t global_test_value = 0;
static enum nn_diagnostic_levels global_diag_messaging_level = DIAG_NORMAL;
//static enum iis2dh_output_data_rates_e user_iis2dh_output_data_rate = ODR_100_HZ;



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




static uint32_t odr_value_has_changed = 0;
static enum iis2dh_output_data_rates_e requested_odr_present = ODR_0_POWERED_DOWN;

uint32_t set_requested_iis2dh_odr(const enum iis2dh_output_data_rates_e passed_rate)
{

    if ( requested_odr_present != passed_rate ) // Output Data Rate changed since last read of this value from scoreboard
    {
        requested_odr_present = passed_rate;
        odr_value_has_changed = 1;
    }

    return odr_value_has_changed;
}


uint32_t get_requested_iis2dh_odr(enum iis2dh_output_data_rates_e* value_to_return)
{
    *value_to_return = requested_odr_present;
    odr_value_has_changed = 0;
    return 0;
}




// --- EOF ---
