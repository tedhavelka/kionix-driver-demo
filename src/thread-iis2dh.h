#ifndef _THREAD_IIS2DH_ACCELEROMETER_H
#define _THREAD_IIS2DH_ACCELEROMETER_H



/**
 *  Function to initialize Zephyr thread for STMicro IIS2DH driver code tests.
 */

int initialize_thread_iis2dh_task(void);

uint32_t wrapper_iis2dh_register_read(const uint8_t register_addr, uint8_t* register_value);

uint32_t wrapper_iis2dh_register_write(const uint8_t register_addr, uint8_t* register_value);



#endif // _THREAD_IIS2DH_ACCELEROMETER_H
