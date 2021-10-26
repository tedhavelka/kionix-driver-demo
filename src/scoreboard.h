#ifndef _SCOREBOARD_H
#define _SCOREBOARD_H


#include "diagnostic.h"

// Routine prototypes

uint32_t set_global_test_value(const uint32_t passed_value);
uint32_t get_global_test_value(uint32_t* value_to_return);
uint32_t update_diag_messaging_level(const enum nn_diagnostic_levels passed_value);
uint32_t get_diag_messaging_level(enum nn_diagnostic_levels* value_to_return);



#endif // _SCOREBOARD_H
