##----------------------------------------------------------------------
##
##  Kionix driver demo
##  Copyright (c) <team name here>
##  PDX-License-Identifier: Apache-2.0
##
##  References:
##    *  https://stackoverflow.com/questions/31343813/displaying-cmake-variables
##
##----------------------------------------------------------------------


cmake_minimum_required(VERSION 3.13.1)

# Determine the build type
if (NOT BUILD_TYPE)
  set(BUILD_TYPE debug)
endif()

# Print out build type
message(STATUS "Build type: ${BUILD_TYPE}")



## 2021-11-11 Note following two vars select `crosstool-ng` as
##  cross-compiler part of toolchain, as opposed to 'gnuarmemb':

# Set environment variables
set(CROSS_COMPILE arm-zephyr-eabi-)

# Note following toolchain variant setting selects crosstool-ng compiler, not gcc gnuarmemb.
# To build this project in Nordic Semiversion of Segger gcc cross compiler
# must be installed and this variable set to 'gnuarmemb':
set(ZEPHYR_TOOLCHAIN_VARIANT zephyr)



#set(ZEPHYR_SDK_INSTALL_DIR ${HOME}/zephyrproject/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(kionix-driver-demo)

## This zephyr-include-directories stanza makes work pound inclusion of iis2dh.h using < > braces:
## (Unsure how zephyr/samples/sensor/lis2dh finds its corresponding driver sources - TMH)
zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/iis2dh)

zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/lis2dh)

# 2021-11-11 - trying to bring in thread_analyzer.h:
##zephyr_include_directories($ENV{ZEPHYR_BASE}/include/debug)   // <-- not sufficient, or rather not a correct path - TMH
include_directories(../zephyr/include/debug)

### 2021-11-15 - Unexpected need to add this stanza when today `west update` no longer pulls in kionix-drivers dependency:
##zephyr_include_directories(../kionix-drivers/drivers/kionix/kx132-1211)

# 2021-11-16 - attempt to utilize Zephyr kernel timing reporting:
include_directories(../zephyr/include)


target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/diagnostic.c)
target_sources(app PRIVATE src/thread-iis2dh.c)
target_sources(app PRIVATE src/thread-lis2dh.c)

target_sources(app PRIVATE src/scoreboard.c)
target_sources(app PRIVATE src/banner.c)

target_sources(app PRIVATE src/thread-simple-cli.c)
target_sources(app PRIVATE src/cli-zephyr-stack-info.c)
target_sources(app PRIVATE src/cli-zephyr-kernel-timing.c)
target_sources(app PRIVATE src/cli-iis2dh-sensor.c)



## 2021-11-11
## https://stackoverflow.com/questions/31343813/displaying-cmake-variables

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

print_all_variables()


# --- end of CMakeLists.txt file ---
