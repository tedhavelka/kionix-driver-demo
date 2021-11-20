/*
 * ---------------------------------------------------------------------
 *
 *  @Project   Kionix Driver Demo
 *
 *  @File      scoreboard.c
 *
 *  @Brief     Module which entails a couple of programmming patterns
 *   for sharing data between modules in decoupled way.  Scoreboard
 *   facilities include:
 *
 *   +  setter and getter pairs of routines for integer, float and
 *      string values.
 *
 *   +  table of Boolean flags and associated pointers to optional.
 *      callback routines.
 *
 *   +  Boolean flag update routines to set, clear supported flags.
 *
 *   +  callback orchestrator which flag updaters optionally call
 *      to execute callbacks associated with flag set and clear events.
 *
 * ---------------------------------------------------------------------
 */


//----------------------------------------------------------------------
// - SECION - pound includes
//----------------------------------------------------------------------

/*
 *  @Note While every attempt is made to keep scoreboard programming
 *  patterns generic and general purpose, the scoreboard is necessarily
 *  tightly coupled to all project firmware modules which utilize it.
 *  For modules which use only setter and getter routines, those modules
 *  must #include this scoreboard module's header file.  For modules
 *  which make use of supported Boolean flags with one or more callback
 *  routines enabled, those modules must #include scoreboard.h and
 *  this scoreboard module must #include given external module's header
 *  files that present that given module's public API.
 *
 *  This coupling is largely concentrated in the scoreboard module,
 *  and permits most other modules to be designed in a decoupled way.
 *  That is, most modules do not need to know or directly synchronize
 *  with each other.  The posted values and flags in this scoreboard
 *  module, and select architected callbacks provide needed interaction
 *  while keeping module code more simple and easier to understand
 *  and maintain.  - TMH
 */

#include <stdint.h>                // to provide define of uint32_t

#include <sys/printk.h>            // to provide printk() function

// Scoreboard module's header file:
#include "scoreboard.h"

// General Pulse Stage 1 header files:
#include "common.h"
#include "return-values.h"
#include "routine-options.h"
#include "diagnostic.h"

// Specific modules this scoreboard needs know about:
//#include "data-model-stage1.h"
#include "main.h"

//extern uint32_t on_event__temperature_readings_requested__query_iis2dh(uint32_t event);



//----------------------------------------------------------------------
// - SECION - scoreboard scoped variables used by setters and getters
//----------------------------------------------------------------------

static enum nn_diagnostic_levels global_diag_messaging_level = DIAG_NORMAL;

//static enum iis2dh_output_data_rates_e user_iis2dh_output_data_rate = ODR_100_HZ;

// Note up to calling modules to assure valid bit configurations are
// posted to this scoreboard variable:
static uint8_t iis2dh_full_scale_selection = 0;



//----------------------------------------------------------------------
// - SECION - 2021-11-03 work on 'flags plus callbacks' facility
//----------------------------------------------------------------------

// --- 1103-B DEV BEGIN ---
// QUESTION how can we provide a simple, light weight framework of
//  boolean flags which can optionally exercise one or more callbacks
//  when they go from unset to set, and from set to unset?

// (1) See scoreboard.h for enumeration of supported flags in Pulse Stage 1
//     firmware project.



//
// (2)  Following struct contains two integers each of which are used as
// bit-wise flags to support indications toward up to 32 callback
// functions to exercise when given project-implemented flag is either
// set or unset.  A scoreboard routine named handle_flag_events
// checks a table of flag_callbacks structures and invokes (calls)
// any callback routines which are indicated for the given flag's
// reported event.
//
// Each project implemented flag here in this part of the scoreboard
// gets one of these 'callback present indicator' types.  Upshot:  a
// flag undergoing set or clear events can potentially trigger many
// callback routines to run:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

struct flag_and_callback_indicators
{
    uint32_t flag;                                     // A numeric ID for each supported flag
    uint32_t callbacks_0_to_31_enabled_when_set;       // list of callbacks to run when flag set
    uint32_t callbacks_0_to_31_enabled_when_cleared;   // list of callbacks to run when flag cleared
};

typedef struct flag_and_callback_indicators flag_and_callback_indicators_t;

// REF https://stackoverflow.com/questions/10468128/how-do-you-make-an-array-of-structs-in-c

#if 0 // following is exampmle code which fails build when in .h file, confirming our structs need to be in .c file:
struct body
{
    double p;
};

struct body bodies[3];
#endif


// (3)  An array to serve as a table of flag callback indicators, which
// are themselves bit-wise flags.  A Boolean flag can be set or unset,
// a standard C integer has 32-bits, so for this first draft "flags
// callbacks" implementation a flag can have up to 32 callback functions
// for case where given flag is set, and from zero to 32 callback
// functions for case where flag is cleared.  Note these callback
// function pointers are shared among supported flags.  If more than
// 32 callbacks are needed, additional integer members can be added to
// the flag_callbacks struct.

flag_and_callback_indicators_t table_of_flags[COUNT_FLAGS_SUPPORTED];


// (4)  A function pointer definition and an array of function
// pointers which provide the means to exercise a callback for a given
// flag.  These can be assigned a value in at least two ways, one way
// being at compile time and a second way being at run time.
//
// Per Zephyr philosophy to use fewer resources and know memory use at
// compile time, we assign pointers to callback functions with
// pre-processor directives to enable or disable them:

#define COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE (32)

typedef uint32_t (*flag_callback_t)(const uint32_t event);

// How to understand above typedef and declaration, following article by
// Brian Barto at https://medium.com/@bartobri/untangling-complex-c-declarations-9b6a0cf88c96
//
// Summarizing:
//
//    "Token 'flag_callback_t' is a pointer to a function,
//     a function which expects a const parameter of type uint32_t,
//     and returns a value of type uint32_t."
//
// Breaking down the syntax:
//
//    typedef uint32_t (*flag_callback_t)(const uint32_t event);
//                      ^
//                      |
//    "Token 'flag_callback_t' is a pointer..."
//
//    typedef uint32_t (*flag_callback_t)(const uint32_t event);
//                                       ^                    ^
//                                       |                    |
//    "...to a function..."
//
//    typedef uint32_t (*flag_callback_t)(const uint32_t event);
//                                        ^^^^^^^^^^^^^^^^^^^^
//                                                  |
//    "...which expects a const uint32_t type..."
//
//    typedef uint32_t (*flag_callback_t)(const uint32_t event);
//            ^^^^^^^^
//               |
//    "...and returns a uint32_t type."


//
// 2021-11-07 -
// Here declare two tables of function / routine / callback pointers.
// Note, C is somewhat rigid in that once we declare and typedef a
// function pointer, our choice of parameters and return value types
// are locked in.  Our callbacks in this case expect a const uint32_t
// and return a uint32_t.  This should be sufficient for callbacks to
// get and return everything they need to perform useful, coordinating
// work between threads and app tasks: 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static flag_callback_t table_of_callbacks_for_flag_set[COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE];
static flag_callback_t table_of_callbacks_for_flag_clear[COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE];



// (5)  Routine to check for and execute callbacks:

uint32_t handle_flag_callbacks(enum pulse_stage1_supported_flags_e flag, enum flag_event_e event);

// --- 1103-B DEV END ---



// DEV 1110:  flash resources permitting, provide flag descriptions with
//  array of strings:

char* flag_descriptions[COUNT_FLAGS_SUPPORTED] =
{
    "flag accelerometer readings set ready\0",
    "flag FIFO overrun in latest readings set\0",
    "flag temperature readings requested\0",
    "\0"
};




//----------------------------------------------------------------------
// - SECTION - routines, assign pointers to event callbacks
//----------------------------------------------------------------------

void assign_function_callback_pointers(void)
{
// (1)
// Assign the function pointer for event when readings sets are completely gathered and stored:
    table_of_callbacks_for_flag_set[RS__ACCELEROMETER_READINGS_SET_COMPLETE] = &on_event__readings_done__calculate_vrms;
// Connect scoreboard flag for this event to above callback:
// ( NOTE these bit flags implemented but not yet checked by callback orchestrating routine in first draft scoreboard - TMH )
    table_of_flags[RS__ACCELEROMETER_READINGS_SET_COMPLETE].callbacks_0_to_31_enabled_when_set |= ( 1 << RS__ACCELEROMETER_READINGS_SET_COMPLETE );

// (2)
// Next flag callback assignment here.
    table_of_callbacks_for_flag_set[TI__TEMPERATURE_READING_REQUESTED] = &on_event__temperature_readings_requested__query_iis2dh;
    table_of_callbacks_for_flag_clear[TI__TEMPERATURE_READING_REQUESTED] = NULL;
// Connect scoreboard flag for this event to above callback:
    table_of_flags[TI__TEMPERATURE_READING_REQUESTED].callbacks_0_to_31_enabled_when_set |= ( 1 << TI__TEMPERATURE_READING_REQUESTED );

// (3)
// Third flag callback assignment here.

}



#ifdef KD_DEV__ENABLE_SCOREBOARD_DEVELOPMENT_ROUTINES
void show_summary_of_callback_pointers(uint32_t option)
{
    uint32_t i = 0;

    printk("Summary of callback pointers for 'set flag' events:\n\
1 = defined, 0 = undefined.\n\
There are %u callback pointers presently supported:\n\n", COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE
);

    if ( (option == SUMMARY_CALLBACKS_ENABLED_FOR_FLAG_SET)
      || (option == SUMMARY_ALL_CALLBACKS_ENABLED) )
    {
        for ( i = 0; i < COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE; i++ )
        {
            if ( table_of_callbacks_for_flag_set[i] == NULL )
                { printk(" 0"); }
            else
                { printk(" 1"); }
        }
    }

    if ( (option == SUMMARY_CALLBACKS_ENABLED_FOR_FLAG_CLEAR)
      || (option == SUMMARY_ALL_CALLBACKS_ENABLED) )
    {
        printk("Summary of callback pointers for 'clear flag' events:\n\n");
        for ( i = 0; i < COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE; i++ )
        {
            if ( table_of_callbacks_for_flag_clear[i] == NULL )
                { printk(" 0"); }
            else
                { printk(" 1"); }
        }
    }

    printk("\n\n");
}
#endif



//----------------------------------------------------------------------
// - SECTION - routine definitions
//----------------------------------------------------------------------

uint32_t initialize_scoreboard(void)
{
    int i = 0;
    static uint32_t scoreboard_initialized = 0;
    uint32_t rstatus = ROUTINE_OK;

    if ( scoreboard_initialized == TRUE )
    {
        rstatus = KD__SB_SCOREBOARD_INITIALIZED;
    }
    else
    {
// Clear the two tables of callback function (routine) pointers:
        for ( i = 0; i < COUNT_CALLBACKS_SUPPORTED_PER_EVENT_TYPE; i++ )
        {
            table_of_callbacks_for_flag_set[i] = NULL;
            table_of_callbacks_for_flag_clear[i] = NULL;
        }

// Initialize the sole table of project supported Boolean flags:
        for ( i = 0; i < COUNT_FLAGS_SUPPORTED; i++ )
        {
            table_of_flags[i].flag = FALSE;
            table_of_flags[i].callbacks_0_to_31_enabled_when_set = 0;
            table_of_flags[i].callbacks_0_to_31_enabled_when_cleared = 0;
        }
    }

    assign_function_callback_pointers();

#ifdef KD_DEV__ENABLE_SCOREBOARD_DEVELOPMENT_ROUTINES
    show_summary_of_callback_pointers(SUMMARY_CALLBACKS_ENABLED_FOR_FLAG_SET);
#endif

    return rstatus;
}



//----------------------------------------------------------------------
// - SECTION - routines, setters and getters
//----------------------------------------------------------------------

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

uint32_t scoreboard__set_requested_iis2dh_odr(const enum iis2dh_output_data_rates_e passed_rate)
{
printk("YY\nYYYYY present ODR = %u, requested ODR = %u\nYY\n", requested_odr_present, passed_rate);
    if ( requested_odr_present != passed_rate ) // Output Data Rate changed since last read of this value from scoreboard
    {
        requested_odr_present = passed_rate;
        odr_value_has_changed = 1;
    }

    return odr_value_has_changed;
}


uint32_t scoreboard__get_requested_iis2dh_odr(enum iis2dh_output_data_rates_e* value_to_return)
{
    *value_to_return = requested_odr_present;
    odr_value_has_changed = 0;
    return 0;
}



// Setter and getter for IIS2DH full scale configuration bits:
 
uint32_t scoreboard__set_IIS2DH_CTRL_REG4_full_scale_config_bits(const uint8_t fs_setting)
{
    iis2dh_full_scale_selection = fs_setting;
    return 0;
}

uint32_t scoreboard__get_IIS2DH_CTRL_REG4_full_scale_config_bits(uint8_t *fs_setting)
{
    *fs_setting = iis2dh_full_scale_selection;
    return 0;
}




//----------------------------------------------------------------------
// - SECTION - scoreboard routines to update flags
//----------------------------------------------------------------------

/*
 *  @Note  For each flag a single routine can set or clear the flag.
 *         Each implemented flag has an update routine whose name begins
 *         with 'scoreboard__update_flag_'.  These update routines
 *         may optionally call a general purpose callback orchestrator,
 *         a routine which checks whether a flag event is configured
 *         to execute one or more callbacks when that given event
 *         occurs.
 */

// Following flag gets set when accelerometer readings are ready to
// be transformed and used to calculate VRMS.  This flag is unset
// shortly after VRMS is calculated, so that new accelerometer readings
// can be gathered by other parts of this firmware:

uint32_t scoreboard__update_flag_accelerometer_readings_ready(enum flag_event_e event_or_updating_value)
{
    uint32_t rstatus = ROUTINE_OK;

    if ( table_of_flags[RS__ACCELEROMETER_READINGS_SET_COMPLETE].flag != event_or_updating_value )
    {
        table_of_flags[RS__ACCELEROMETER_READINGS_SET_COMPLETE].flag = event_or_updating_value;
        handle_flag_callbacks(RS__ACCELEROMETER_READINGS_SET_COMPLETE, event_or_updating_value);
    }

    return rstatus;
}



uint32_t handle_flag_callbacks(enum pulse_stage1_supported_flags_e flag_index, enum flag_event_e event)
{
    uint32_t callback_status = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// In this IF-block execute a routine via a C function pointer, when
// the callback pointer is not NULL.
//
// Note, in first draft of this routine the callback pointer is the
// one at the index of the flag's position in the enumeration of
// supported flags.  In Pulse Stage 1 pre-release firmware
// there are ten supported flags, but each flag has two associated integer
// variables to specify associated callbacks.  These integers each
// contain 32 bits which by design indicate up to 32 callbacks a flag
// may trigger when set, and alternately when given flag is cleared.
//
// NOTE:  We are not yet reading those two ints to make multiple callbacks.
//  
// If we have fewer than ten event-driven callbacks needed in Pulse
// Stage 1 firmware, we can hold off implementing the logic to read those
// two integers.  Instead specific flag update routines can call this
// callback orchestrator two or more times as needed.  Less elegant but
// also less and simpler coding.  - TMH
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if ( event == VALUE_OF_FLAG_SET )
    {
        if ( table_of_callbacks_for_flag_set[flag_index] != NULL )
        {
// Note, 'table_of_callbacks_for_flag_set' is an array of function pointers:
            printk("- MARK 3 - executing callback for flag %u set event . . .\n", flag_index);
            callback_status = table_of_callbacks_for_flag_set[flag_index](event);
        }
        else
        {
            printk("- MARK 2 - \n");
            printk("- ERROR:  callback pointer NULL!\n");
        }
    }

// TO-DO:  handle 'flag cleared' event here.

    return callback_status;

} // end routine handle_flag_callbacks()




// --- EOF ---
