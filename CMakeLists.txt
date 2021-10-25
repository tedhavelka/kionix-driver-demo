##----------------------------------------------------------------------
## Kionix driver demo
## Copyright (c) <team name here>
## SPDX-License-Identifier: Apache-2.0
##----------------------------------------------------------------------


cmake_minimum_required(VERSION 3.13.1)

# Determine the build type
if (NOT BUILD_TYPE)
  set(BUILD_TYPE debug)
endif()

# Print out build type
message(STATUS "Build type: ${BUILD_TYPE}")

# Set environment variables
set(CROSS_COMPILE arm-zephyr-eabi-)
set(ZEPHYR_TOOLCHAIN_VARIANT zephyr)
#set(ZEPHYR_SDK_INSTALL_DIR ${HOME}/zephyrproject/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(kionix-driver-demo)

## This zephyr-include-directories stanza makes work pound inclusion of iis2dh.h using < > braces:
## (Unsure how zephyr/samples/sensor/lis2dh finds its corresponding driver sources - TMH)
zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/iis2dh)

zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/sensor/lis2dh)


target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/diagnostic.c)
target_sources(app PRIVATE src/thread_iis2dh.c)
target_sources(app PRIVATE src/thread_lis2dh.c)

target_sources(app PRIVATE src/thread_simple_cli.c)
target_sources(app PRIVATE src/scoreboard.c)


# --- end of CMakeLists.txt file ---
