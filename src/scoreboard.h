#ifndef _SCOREBOARD_H
#define _SCOREBOARD_H


//----------------------------------------------------------------------
// For scoreboard module to support custom data types and structures
// in a given project, scoreboard must pound include a number of
// project header files:
//----------------------------------------------------------------------

#include "diagnostic.h"
#include "iis2dh-registers.h"




// Routine prototypes

uint32_t set_global_test_value(const uint32_t passed_value);
uint32_t get_global_test_value(uint32_t* value_to_return);
uint32_t update_diag_messaging_level(const enum nn_diagnostic_levels passed_value);
uint32_t get_diag_messaging_level(enum nn_diagnostic_levels* value_to_return);

uint32_t scoreboard_set_requested_iis2dh_odr(const enum iis2dh_output_data_rates_e data_rate);

/*
 *  @brief  Always returns iis2dh output data rate in parameter,
 *          return value is:
 *      0   for rate unchanged since last check
 *      1   when rate has changed since last check
 */
uint32_t scoreboard_get_requested_iis2dh_odr(enum iis2dh_output_data_rates_e *data_rate);



#endif // _SCOREBOARD_H
