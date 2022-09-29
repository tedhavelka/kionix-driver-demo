# ======================================================================
#  Project:  Kionix driver demo
#
#  SPDX-License-Identifier: Apache-2.0
# ======================================================================



# ----------------------------------------------------------------------
# - SECTION - the four stanzas from a Zephyr 'hello_world' simple app
# ----------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

#target_sources(app PRIVATE src/main.c)



# ----------------------------------------------------------------------
# - SECTION - development messages
# ----------------------------------------------------------------------

message(STATUS "- DEV 0923 - variable ZEPHYR_BASE holds '${ZEPHYR_BASE}'")



# ----------------------------------------------------------------------
# - SECTION - additional include paths
# ----------------------------------------------------------------------

zephyr_include_directories($ENV{ZEPHYR_BASE}/include/zephyr/debug)

zephyr_include_directories($ENV{ZEPHYR_BASE}/include/zephyr/kernel)

# Allow cmake and build tools to find 'arch_interface.h':
zephyr_include_directories($ENV{ZEPHYR_BASE}/include/zephyr/sys)


# To include lis2dh.h:
zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/lis2dh)
# To include iis2dh.h:
zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/iis2dh)




# ----------------------------------------------------------------------
# - SECTION - project sources
# ----------------------------------------------------------------------

# Originally called Kionix out-of-tree driver code (no new thread yet created to do this):
target_sources(app PRIVATE src/main.c)

# Identifying modules:
target_sources(app PRIVATE src/banner.c)

# Development modules:
target_sources(app PRIVATE src/diagnostic.c)

# Sensors related:
target_sources(app PRIVATE src/thread-iis2dh.c)
target_sources(app PRIVATE src/thread-lis2dh.c)
target_sources(app PRIVATE src/thread-led.c)

# Command Line Interface related:
target_sources(app PRIVATE src/thread-simple-cli.c)
target_sources(app PRIVATE src/cli-zephyr-stack-info.c)
target_sources(app PRIVATE src/cli-zephyr-kernel-timing.c)
target_sources(app PRIVATE src/cli-iis2dh-sensor.c)

# Internals:
target_sources(app PRIVATE src/conversions.c)
target_sources(app PRIVATE src/scoreboard.c)



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

#print_all_variables()


# --- end of CMakeLists.txt file ---
