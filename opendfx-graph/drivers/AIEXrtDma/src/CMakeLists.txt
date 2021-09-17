file(GLOB SOURCES aie_control_xrt.cpp aieMatMul.cpp)
#file(GLOB SOURCES aieMatMul.cpp)



set (_deps "dl" "pthread" "xaiengine" "adf_api_xrt" "xrt_core" "xrt_coreutil")

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

add_library(AIEXrtDma_shared SHARED ${SOURCES})

target_link_libraries(AIEXrtDma_shared PUBLIC dfx-mgr-shared dl pthread xaiengine adf_api_xrt xrt_core xrt_coreutil)

target_compile_options(AIEXrtDma_shared PRIVATE -Wno-unknown-pragmas -Wno-deprecated-copy -Wno-missing-field-initializers -Wno-unused-parameter)
#set_target_properties(AIEXrtDma_shared PROPERTIES LINK_FLAGS "/ignore:deprecated-copy")
#add_definitions(-D__PS_ENABLE_AIE__)
include_directories(kernels)
