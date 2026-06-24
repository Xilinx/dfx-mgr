#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
# SPDX-License-Identifier: MIT
#########################################################################

cmake_minimum_required (VERSION 3.5)
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
include_directories(include)

foreach(_inc_path ${CMAKE_INCLUDE_PATH})
  collect (PROJECT_INC_DIRS "${_inc_path}")
endforeach()


foreach (_lib ${_deps})
  message ("Looking for ${_lib}")
  find_library (_lib_path ${_lib})
  if (NOT _lib_path)
    message ( "external library ${_lib} not found" )
    message ( "hint: you may need to pass -DCMAKE_LIBRARY_PATH=<path>" )
    message ( FATAL_ERROR "library ${_lib} is required to build the examples" )
  endif (NOT _lib_path)
  get_filename_component (_lib_dir_path ${_lib_path} DIRECTORY)
  collect (PROJECT_LIB_DIRS ${_lib_dir_path})
  unset(_lib_path CACHE)
endforeach()

collector_list  (_list PROJECT_LIB_DIRS)
link_directories (${_list})


add_subdirectory (src)
add_subdirectory (example)

# Code-formatting targets for first-party sources under src/ and example/:
#   'format'       : rewrite files in-place with clang-format
#   'format-check' : dry-run validation, non-zero exit if any file needs formatting
# Requires clang-format >= 12 (needed by the .clang-format alignment options).
set(CLANG_FORMAT_MIN_VERSION 12)

set(_cf_major 0)
find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
  execute_process(COMMAND ${CLANG_FORMAT} --version
                  OUTPUT_VARIABLE _cf_ver OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "version ([0-9]+)" _ "${_cf_ver}")
  if(CMAKE_MATCH_1)
    set(_cf_major ${CMAKE_MATCH_1})
  endif()
endif()

if(_cf_major LESS CLANG_FORMAT_MIN_VERSION)
  # Missing or too old: define failing targets instead of breaking configure.
  if(NOT CLANG_FORMAT)
    set(_cf_msg "clang-format not found (>= ${CLANG_FORMAT_MIN_VERSION} required)")
  else()
    set(_cf_msg "clang-format >= ${CLANG_FORMAT_MIN_VERSION} required (found version ${_cf_major}: ${CLANG_FORMAT})")
  endif()
  add_custom_target(format       COMMAND bash -c "echo '${_cf_msg}'; exit 1" VERBATIM)
  add_custom_target(format-check COMMAND bash -c "echo '${_cf_msg}'; exit 1" VERBATIM)
else()
  message(STATUS "Found clang-format ${_cf_major}: ${CLANG_FORMAT}")
  set(_cf_find "find src example -type f \\( -name '*.c' -o -name '*.h' \\) ")
  add_custom_target(format
    COMMAND bash -c "${_cf_find} | xargs -r ${CLANG_FORMAT} -i"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} VERBATIM)
  add_custom_target(format-check
    COMMAND bash -c "${_cf_find} | xargs -r ${CLANG_FORMAT} --dry-run --Werror"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} VERBATIM)
endif()

#if (WITH_EXAMPLE)
#endif (WITH_EXAMPLE)
# vim: expandtab:ts=2:sw=2:smartindent
