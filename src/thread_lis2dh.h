#ifndef _THREAD_IIS2DH_ACCELEROMETER
#define _THREAD_IIS2DH_ACCELEROMETER

/**
 *  Function to initialize Zephyr thread for STMicro LIS2DH driver code tests.
 *  Can we get this out-of-tree STMicro driver to work with the
 *  accelerometer that is on board sparkfun_thing_plus_nrf9160?
 */

int initialize_thread_lis2dh_task(void);

#endif // _THREAD_IIS2DH_ACCELEROMETER
