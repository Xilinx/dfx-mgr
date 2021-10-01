# only for cmake --version >= 3.5.1
cmake_minimum_required(VERSION 3.2.1)

set (CMAKE_BUILD_TYPE "Debug")
MESSAGE( STATUS "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}" )

# project name
project(opendfx-graph)

# creates the variable EXEC and sets it to hello
set(LIB dfxgraph)

# set the C++14 standard
set(CMAKE_CXX_STANDARD 14)

# I../includes
include_directories(include)
#include_directories(../src)

collector_list  (_list PROJECT_INC_DIRS)
include_directories (${_list} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${_list} ${CMAKE_CURRENT_SOURCE_DIR}/../include)


# puts all .cpp files inside src to the SOURCES variable
file(GLOB SOURCES src/*.cpp)

# compiles the files defined by SOURCES to generante the executable defined by EXEC
#add_executable(${EXEC} ${SOURCES})

#add_library(${LIB}_static STATIC ${SOURCES})
add_library(${LIB} SHARED ${SOURCES})

#target_link_libraries(${LIB}_shared dl pthread)
target_link_libraries(${LIB} dfx-mgr-shared dl pthread opencv_core opencv_highgui )
#target_link_libraries(${LIB}_static dfx-mgr-static dl pthread)

set_target_properties (${LIB} PROPERTIES
    OUTPUT_NAME       "${LIB}"
    VERSION           "${PROJECT_VER}"
    SOVERSION         "${PROJECT_VER_MAJOR}"
)

install (TARGETS ${LIB} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install (TARGETS ${LIB}_shared ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

#target_link_libraries(${EXEC} dl)

add_subdirectory (test)
add_subdirectory (drivers)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

#include_directories (${_inc_dirs})
#add_definitions (-DACAPD_INTERNAL)

collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")

collect (PROJECT_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
collect (PROJECT_INC_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")

add_subdirectory (include)


collector_list (_inc_dirs PROJECT_INC_DIRS)
collector_list (_sources PROJECT_LIB_SOURCES)
collector_list (_headers PROJECT_LIB_HEADERS)

foreach (f ${_headers})
  configure_file (${f} include/${PROJECT_NAME}/${f} @ONLY)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${f}
           DESTINATION include RENAME ${PROJECT_NAME}/${f})
  if (${f} MATCHES "^[^/]*\\.h")
    collect (PROJECT_HDR_TESTS "acapd/${f}")
  endif (${f} MATCHES "^[^/]*\\.h")
endforeach (f)

