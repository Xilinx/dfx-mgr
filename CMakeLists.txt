#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# SPDX-License-Identifier: MIT
#########################################################################

cmake_minimum_required (VERSION 2.6)
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms")

project (dfx-mgr C ASM)

include (CheckIncludeFiles)
include (CheckCSourceCompiles)
include (collect)
include (options)

foreach(_inc_path ${CMAKE_INCLUDE_PATH})
  collect (PROJECT_INC_DIRS "${_inc_path}")
endforeach()

add_subdirectory (src)

#if (WITH_EXAMPLE)
add_subdirectory (example)
#endif (WITH_EXAMPLE)
# vim: expandtab:ts=2:sw=2:smartindent
