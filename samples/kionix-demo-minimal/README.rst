# Kionix Driver Demo

Small Zephyr based app, to exercise and show use of a Zephyr out-of-tree driver for Kionix accelerometer with part number KX132-1211.


Overview
********

This sample Zephyr app intended to show in short and simple form, C language based means to call and use a Zephyr RTOS driver for Kionix KX132 accelerometer.


How to build
************

Assuming a correct Zephyr toolchain is installed, and project is cloned into a directory 'dirname' that receives the changes from invoking `west init -l dirname` in the parent directory, this small app can be compiled by calling:

$ west build -b lpcxpresso55s69_cpu0

To flash the board:

$ west flash 

Differing programmers will flash an lpcxpresso55s69 development board, but contributors to this project have so far only tested a Segger J-Link programmer / debugger.



Supported boards
****************

 [ ] nrf9160dk_nrf9160

 [ ] sparkfun_thing_plus_nrf9160

 [*] lpcxpresso55s69_cpu0



Wiring for UARTs and sensors
*****************************

( 2022-11-17 - This section not yet started - TMH )



Expected outputs
****************

When a Kionix KX132-1211 or compatible sensor is wired and properly initialized by Zephyr and driver code, the following six lines with varying readings data will scroll every (SLEEP_TIME_MS / 1000) seconds.  If the sensor is not connected or otherwise fails to initialize, an error message will repeat at the same pace:

main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer 1852795211
main.c - sensor_value.val2 holds 0
main.c - value.val1 as bytes:  0x4B 0x69 0x6F 0x6E   " K  i  o  n "
main.c - Kionix sensor reports part ID of 317
main.c - Kionix sensor x,y,z readings encoded:  0xfb6c02c5, 0x0000404a

main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer 1852795211
main.c - sensor_value.val2 holds 0
main.c - value.val1 as bytes:  0x4B 0x69 0x6F 0x6E   " K  i  o  n "
main.c - Kionix sensor reports part ID of 317
main.c - Kionix sensor x,y,z readings encoded:  0xfb6002d1, 0x0000404a

main.c - Kionix sensor reports its manufacturer ID, as 32-bit integer 1852795211
main.c - sensor_value.val2 holds 0
main.c - value.val1 as bytes:  0x4B 0x69 0x6F 0x6E   " K  i  o  n "
main.c - Kionix sensor reports part ID of 317
main.c - Kionix sensor x,y,z readings encoded:  0xfb4102cf, 0x0000404e



