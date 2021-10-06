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

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/diagnostic.c)
