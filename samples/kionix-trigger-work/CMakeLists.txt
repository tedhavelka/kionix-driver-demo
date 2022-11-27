# ----------------------------------------------------------------------
# 
#   Project:  Kionix driver demo
# 
#   File:  CMakeLists.txt
# 
#   SPDX-License-Identifier: Apache-2.0
# 
#   Copyright (c) Neela Nurseries 2021 - 2022
# 
# ----------------------------------------------------------------------



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
# - SECTION - project sources
# ----------------------------------------------------------------------

# Originally called Kionix out-of-tree driver code (no new thread yet created to do this):
target_sources(app PRIVATE src/main.c)




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
