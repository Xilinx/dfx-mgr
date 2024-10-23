#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# SPDX-License-Identifier: MIT
#########################################################################

#PROJECT_VERSION should match with yocto recipe version for SOVERSION
set (PROJECT_VERSION_MAJOR 2)
set (PROJECT_VERSION_MINOR 0)
set (PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
message (STATUS "dfx-mgr version: ${PROJECT_VERSION} (${CMAKE_SOURCE_DIR})")

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)
message ("-- Build type:  ${CMAKE_BUILD_TYPE}")

if (NOT CMAKE_INSTALL_LIBDIR)
  set (CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

if (NOT CMAKE_INSTALL_BINDIR)
  set (CMAKE_INSTALL_BINDIR "bin")
endif (NOT CMAKE_INSTALL_BINDIR)

set (_host "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}")
message ("-- Host:    ${_host}")

set (_target "${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
message ("-- Target:  ${_target}")

if (NOT DEFINED MACHINE)
  set (MACHINE "Generic")
endif (NOT DEFINED MACHINE)
message ("-- Machine: ${MACHINE}")

# handle if '-' in machine name
string (REPLACE "-" "_" MACHINE ${MACHINE})

if (NOT DEFINED PROJECT_SYSTEM)
  string (TOLOWER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM)
  string (TOUPPER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM_UPPER)
endif (NOT DEFINED PROJECT_SYSTEM)

option (WITH_STATIC_LIB "Build with a static library" ON)

if ("${PROJECT_SYSTEM}" STREQUAL "linux")
  option (WITH_SHARED_LIB "Build with a shared library" ON)
endif ("${PROJECT_SYSTEM}" STREQUAL "linux")

set_property (GLOBAL PROPERTY "PROJECT_EC_FLAGS" -Wall -Werror -Wextra)
# vim: expandtab:ts=2:sw=2:smartindent
