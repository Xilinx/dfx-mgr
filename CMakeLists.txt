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

#if (WITH_EXAMPLE)
#endif (WITH_EXAMPLE)
# vim: expandtab:ts=2:sw=2:smartindent
