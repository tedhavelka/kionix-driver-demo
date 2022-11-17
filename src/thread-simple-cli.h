#ifndef _THREAD_SIMPLE_CLI
#define _THREAD_SIMPLE_CLI

/**
 *  Function to initialize Zephyr thread for STMicro IIS2DH driver code tests.
 */

int initialize_thread_simple_cli(void);


// routine to send character stream to arbitrary, non-Zephyr-chosen UART:
uint32_t printk_cli(const char* output);

// routine to share firmware's latest run-time argument count with commands factored into dedicated source files:
uint32_t argument_count_from_cli_module(void);

uint32_t arg_n(const uint32_t requested_arg, char* return_arg);

uint32_t arg_is_decimal(const uint32_t index_to_arg, int* value_to_return);

uint32_t dec_value_at_arg_index(const uint32_t index_to_arg);



// 2021-11-18 - command factoring work:
#define SIZE_COMMAND_TOKEN   (128)
#define SUPPORTED_ARG_LENGTH (16)



#endif // _THREAD_SIMPLE_CLI
