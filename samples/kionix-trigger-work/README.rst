# Kionix Driver Demo

Small Zephyr based app, to do two things:

(1)  to exercise and show use of a Zephyr out-of-tree driver for Kionix accelerometer KX132-1211.

(2)  to implement and debug "data read" interrupt support for this sensor.


Overview
********

This sample Zephyr app intended to show in short and simple form, C language based means to call and use a Zephyr RTOS driver for Kionix KX132 accelerometer.  As of 2022 Q4 this sample queries over I2C a correctly wired KX132-1211 accelerometer, with main board being an NXP lpcxpresso55s69 dev kit.  This sample app also has code works underway to enable and debug a Zephyr trigger, which is Zephyr RTOS terminology for an interrupt.  The interrupt which the sample enables is the Kionix' data ready interrupt.  The sample configures this interrupt to be active high, and to appear on KX132-1211 interrupt pin 1.  There are two hardware interrupt lines on the KX132.

On an oscilloscope display the KX132 interrupt appears to behave as expected, but on the firmware side there's an unidentified problem keeping the firmware from successfully configuring a GPIO port and pin to receive these interrupts at run time.

As of 2022-11-27 there's a bug in the driver code -- not this demo code -- in which a part of the driver's data structures is not getting a correct assignment of microcontroller GPIO port to a sensor structure data member of type `gpio_dt_spec`.  For details of this specification, see Zephyr documentation page https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html#c.gpio_dt_spec, and source file https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/drivers/gpio.h#L271.

The specific bug manifests in a hard crash of firmware app, when the driver code attempts to make the following call to a Zephyr device API:

   if (!device_is_ready(cfg->int_gpio.port))

The port structure is itself of type `const struct device`, and has a name element.  Normally for a GPIO port that name element would be assigned a value from the project board's device tree source, a name which in the case of lpcxpresso55s69 board turns out to be `gpio1`.  Instead we observe a name value of `=&`.  This just looks wrong.

The KX132 out-of-tree driver code at this same time 2022 Q4 follows heavily the driver design patterns of STMicro IIS2DH driver.  See files in https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/sensor/iis2dh, plus the files in https://github.com/zephyrproject-rtos/hal_st/tree/master/sensor/stmemsc/iis2dh_STdC/driver for details here.

The big change which KX132 driver adopted in 2022 November is the use of Zephyr device tree macros `DT_INST_FOREACH_STATUS_OKAY()`, `DEVICE_DT_INST_DEFINE()` and some smaller, instance based device tree macros to support the sensor structures and instance definition.  Within macro DEVICE_DT_INST_DEFINE() there are two stanzas to optionally set up a GPIO port and pin for an interrupt:

::
   IF_ENABLED(CONFIG_KX132_TRIGGER,                                              \
              (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 }),))  \   

To our understanding this is where the device pointer is assigned the value that's needed for firmware to configure and interact with this given GPIO interrupt pin.

Interestingly, a copy of this assignment from driver to demo, to a line where a local gpio_dt_spec type device is declared, makes an assignment which gives the expected port name.  One small change in the parameters to GPIO_DT_SPEC_INST_GET_OR() is needed.  This change is to pass the fixed value 0 in place of 'inst', as follows from an excerpt from this sample app's main.c:


::
 210 const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(0, drdy_gpios, { 0 }); // hmm, this results in correct name `&gpio1`
 211 ////const struct gpio_dt_spec int_gpio_for_diag = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, { 0 });
 212 
 213     printk("- MARK 4 - about to test local gpio_dt_spec int_gpio.port->name in diag statement . . .\n");
 215     if ( int_gpio_for_diag.port != NULL )
 216     {
 217         printk("- MARK 5 - in demo main.c, interrupt GPIO port name holds '%s',\n", int_gpio_for_diag.port->name);
 218     }


Debugging work underway on this problem 2022-11-27.



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

::
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



