#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# SPDX-License-Identifier: MIT
#########################################################################

find_path (_include_dir xparameters.h)
if (NOT _include_dir)
  message (FATAL_ERROR "xparameter.h not find")
endif (NOT _include_dir)
collect (PROJECT_INC_DIRS ${_include_dir})

collector_list  (_list PROJECT_INC_DIRS)
include_directories (${_list} ${CMAKE_CURRENT_SOURCE_DIR})

set (_lib "xilfpga")
find_library (_lib_path ${_lib})
if (NOT _lib_path)
  message ( "external library ${_lib_path} not found" )
  message ( "hint: you may need to pass -DCMAKE_LIBRARY_PATH=<path>" )
  message ( FATAL_ERROR "library ${_lib} is required to build the examples" )
endif (NOT _lib_path)
list (APPEND _lib "xilmailbox")
list (APPEND _lib "xil")
list (APPEND _lib "c")
list (APPEND _lib "m")
get_filename_component (_lib_path ${_lib_path} DIRECTORY)
collect (PROJECT_LIB_DIRS ${_lib_path})

collector_list  (_list PROJECT_LIB_DIRS)
link_directories (${_list})

set (_linker_script ${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(accel-config.h.in accel-config.h @ONLY)
foreach (_app accel-pr)
  set (_src ${CMAKE_CURRENT_SOURCE_DIR}/${_app}.c accel-config.s)
  list (APPEND _src accel-config.s)
  set_source_files_properties(accel-config.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
  add_executable (${_app}.elf ${_src})
  if (PROJECT_EC_FLAGS)
    string(REPLACE " " ";" _ec_flgs ${PROJECT_EC_FLAGS})
    target_compile_options (${_app}-static PUBLIC ${_ec_flgs})
  endif (PROJECT_EC_FLAGS)
  target_link_libraries(${_app}.elf -Wl,-Map=${_app0}.map -Wl,--gc-sections -T\"${_linker_script}\" -Wl,--start-group acapd ${_lib} -Wl,--end-group)
  install (TARGETS ${_app}.elf RUNTIME DESTINATION bin)
endforeach (_app)
