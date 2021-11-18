#ifndef _RETURN_VALUES_H
#define _RETURN_VALUES_H

enum kd_return_values
{
    ROUTINE_OK = 0,

    ERROR_NO_ARGS_PARSED,
    ERROR_TOO_FEW_ARGS_PARSED,
    WARNING_MORE_ARGS_FOUND_THAN_SUPPORTED,
    WARNING_COMMAND_INPUT_LONGER_THAN_SUPPORTED,
    WARNING_FOUND_ARG_LENGTH_LONGER_THAN_SUPPORTED,
    ERROR_CLI_ARGUMENT_INDEX_OUT_OF_RANGE,
    ERROR_UNEXPECTED_ARGUMENT_TYPE,

    KD__DEVICE_POINTER_NULL,

    LAST_ITEM_IN_RETURN_VALUES_ENUM
};



// To aid in documentating tests in app sources:
#define RESULT_ARG_IS_DECIMAL 0
#define RESULT_ARG_NOT_DECIMAL 1



#endif
