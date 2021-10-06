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



// Function prototypes:

void dmsg(const char* message, int option);



// --- EOF ---
