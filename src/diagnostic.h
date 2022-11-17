#ifndef _DIAGNOSTIC_H
#define _DIAGNOSTIC_H

/*
 * Copyright (c) 2021 Neela Nurseries
 *
 * SPDX-License-Identifier: Apache-2.0
 */



// NOTE:  developers make sure last entry does not exceed integer value of 31 - TMH
enum nn_diagnostic_levels
{
    _NN_DIAG_SILENT,
    _NN_DIAG_ERRORS,
    _NN_DIAG_WARNINGS,
    _NN_DIAG_INFO,
    _NN_DIAG_LEVEL__LAST_ENTRY
};

#define KD_DIAG_SILENT   ( 1 << _NN_DIAG_SILENT )
#define KD_DIAG_ERRORS   ( 1 << _NN_DIAG_ERRORS )
#define KD_DIAG_WARNINGS ( 1 << _NN_DIAG_WARNINGS )
#define KD_DIAG_INFO     ( 1 << _NN_DIAG_INFO )

#define DIAG_OFF       KD_DIAG_SILENT
#define DIAG_MINIMAL   KD_DIAG_ERRORS
#define DIAG_NORMAL  ( KD_DIAG_ERRORS | KD_DIAG_WARNINGS )
#define DIAG_VERBOSE ( KD_DIAG_ERRORS | KD_DIAG_WARNINGS | KD_DIAG_INFO )


// NOTE:  developers make sure last entry does not exceed integer value of 31 - TMH
enum nn_diagnostic_message_options
{
    _NN_DIAG_TO_DEFAULT_UART = _NN_DIAG_LEVEL__LAST_ENTRY,
    _NN_DIAG_TO_CLI_UART,
    _NN_DIAG_OPTION__LAST_ENTRY
};

#define KD_DIAG_OPTION_TO_DEFAULT_UART ( 1 << _NN_DIAG_TO_DEFAULT_UART )
#define KD_DIAG_OPTION_TO_CLI_UART     ( 1 << _NN_DIAG_TO_CLI_UART )


// "Project level diagnostics" is project wide default diagnostics setting:

//#define PROJECT_DIAG_LEVEL DIAG_NORMAL // DIAG_OFF
//#define PROJECT_DIAG_LEVEL DIAG_OFF // DIAG_NORMAL
#define PROJECT_DIAG_LEVEL ( DIAG_NORMAL | KD_DIAG_OPTION_TO_CLI_UART )





//----------------------------------------------------------------------
// - SECTION - buffer sizes
//----------------------------------------------------------------------

#define SIZE_OF_MESSAGE_SHORT (80)
#define SIZE_OF_MESSAGE_MEDIUM (160)
#define DEFAULT_MESSAGE_SIZE (256)



//----------------------------------------------------------------------
// - SECTION - Function prototypes
//----------------------------------------------------------------------

void dmsg(const char* message, int option);



#endif // _DIAGNOSTIC_H
