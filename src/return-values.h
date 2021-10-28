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
    LAST_ITEM_IN_RETURN_VALUES_ENUM
};

#endif