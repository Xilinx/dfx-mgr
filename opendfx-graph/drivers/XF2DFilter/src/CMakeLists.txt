file(GLOB SOURCES aie_control_xrt.cpp XF2DFilter.cpp)
#file(GLOB SOURCES aieMatMul.cpp)

include_directories(include/aie)


set (_deps "dl" "pthread" "xaiengine" "adf_api_xrt" "xrt_core" "xrt_coreutil" "opencv_videoio" "opencv_imgcodecs" "opencv_core" "opencv_imgproc" "opencv_features2d" "opencv_flann" "opencv_video" "opencv_calib3d" "opencv_highgui" "xilinxopencl" "smartTilerStitcher")

message ("Looking @ ${_lib_path}")

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

add_library(XF2DFilter_shared SHARED ${SOURCES})

target_link_libraries(XF2DFilter_shared PUBLIC dfx-mgr-shared dl pthread xaiengine adf_api_xrt xrt_core xrt_coreutil smartTilerStitcher opencv_videoio opencv_imgcodecs opencv_core opencv_imgproc opencv_features2d opencv_flann opencv_video opencv_calib3d opencv_highgui xilinxopencl)

target_compile_options(XF2DFilter_shared PRIVATE -Wno-unknown-pragmas -Wno-deprecated-copy -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-pointer-arith)
#set_target_properties(AIEXrtDma_shared PROPERTIES LINK_FLAGS "/ignore:deprecated-copy")
#add_definitions(-D__PS_ENABLE_AIE__)
include_directories(kernels)
