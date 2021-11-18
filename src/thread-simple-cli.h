#ifndef _THREAD_SIMPLE_CLI
#define _THREAD_SIMPLE_CLI

/**
 *  Function to initialize Zephyr thread for STMicro IIS2DH driver code tests.
 */

int initialize_thread_simple_cli_task(void);


// Function to send character stream to arbitrary, non-Zephyr-chosen UART:
uint32_t printk_cli(const char* output);



#endif // _THREAD_SIMPLE_CLI
