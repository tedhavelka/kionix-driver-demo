#ifndef _SCOREBOARD_H
#define _SCOREBOARD_H

/*
 * ---------------------------------------------------------------------
 *
 *  @Project   Kionix Driver Demo
 *
 *  @File      scoreboard.h
 *
 *  @Brief     Module which entails a couple of programmming patterns
 *   for sharing data between modules in decoupled way.  Scoreboard
 *   facilities include:
 *
 *   +  setter and getter pairs of routines
 *
 *   +  table of Boolean flags and associated pointers to optional
 *      callback routines (2021-11-05 not yet fully implemented - TMH)
 *
 * ---------------------------------------------------------------------
 */

//----------------------------------------------------------------------
// For scoreboard module to support custom data types and structures
// in a given project, scoreboard must pound include a number of
// project header files . . . but this runs counter to idea of keeping
// the scoreboard decoupled.  May be reasonable to have calling code
// manage the specialized data type checking and use only standard C
// data types here. - TMH
//----------------------------------------------------------------------

#include "diagnostic.h"

#include "iis2dh-registers.h"    // may not need this include - TMH



//----------------------------------------------------------------------
// - SECTION - symbols and enumerations to share with other modules
//----------------------------------------------------------------------

#define COUNT_FLAGS_SUPPORTED (20)
#define FLAG_DESCRIPTION_LENGTH_IN_BYTES (64)

enum kionix_driver_demo_supported_flags_e
{
    RS__ACCELEROMETER_READINGS_SET_COMPLETE = 0,     // RS = Readings Set module
    TI__FIFO_OVERRUN_IN_LATEST_READINGS_GATHERING,   // TI = Thread IIS2DH
    TI__TEMPERATURE_READING_REQUESTED,               // TI = Thread IIS2DH
    MARKER_END_OF_IMPLEMENTED_FLAGS,
    MARKER_END_OF_SUPPORTED_FLAGS = COUNT_FLAGS_SUPPORTED
};

enum flag_event_e
{
    FLAG_CLEARED = 0,
    FLAG_SET
};



//----------------------------------------------------------------------
// - SECTION - routine prototypes
//----------------------------------------------------------------------

uint32_t initialize_scoreboard(void);

uint32_t set_diag_messaging_level(const enum nn_diagnostic_levels passed_value);
uint32_t get_diag_messaging_level(enum nn_diagnostic_levels* value_to_return);


// . . .
uint32_t scoreboard__set_requested_iis2dh_odr(const enum iis2dh_output_data_rates_e data_rate);
uint32_t scoreboard__get_requested_iis2dh_odr(enum iis2dh_output_data_rates_e *data_rate);

// setter and getter for IIS2DH full scale configuration bits (needed by VRMS calculation code):
uint32_t scoreboard__set_IIS2DH_CTRL_REG4_full_scale_config_bits(const uint8_t fs_setting);
uint32_t scoreboard__get_IIS2DH_CTRL_REG4_full_scale_config_bits(uint8_t *fs_setting);



// - SECTION - Boolean flag routines:

uint32_t scoreboard__update_flag_accelerometer_readings_ready(enum flag_event_e event_or_updating_value);

uint32_t scoreboard__update_flag__temperature_reading_requested(enum flag_event_e event_or_updating_value);



#endif // _SCOREBOARD_H
