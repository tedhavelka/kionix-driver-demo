#ifndef _DIAGNOSTIC_H
#define _DIAGNOSTIC_H

/*
 * Copyright (c) 2021 Neela Nurseries
 *
 * SPDX-License-Identifier: Apache-2.0
 */



enum nn_diagnostic_levels
{
    _NN_DIAG_SILENT,
    _NN_DIAG_ERRORS,
    _NN_DIAG_ERRORS_WARNINGS,
    _NN_DIAG_ERRORS_WARNINGS_INFO
};

#define DIAG_OFF (_NN_DIAG_SILENT)
#define DIAG_NORMAL (_NN_DIAG_ERRORS_WARNINGS)
#define DIAG_VERBOSE (_NN_DIAG_ERRORS_WARNINGS_INFO)

#define SIZE_OF_MESSAGE_SHORT (80)
#define SIZE_OF_MESSAGE_MEDIUM (160)
#define DEFAULT_MESSAGE_SIZE (256)


// Function prototypes:

void dmsg(const char* message, int option);



#endif // _DIAGNOSTIC_H
