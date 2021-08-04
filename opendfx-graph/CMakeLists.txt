# only for cmake --version >= 3.5.1
cmake_minimum_required(VERSION 3.2.1)

# project name
project(opendfx-graph)

# creates the variable EXEC and sets it to hello
set(LIB dfxgraph)

# set the C++14 standard
set(CMAKE_CXX_STANDARD 14)

# I../includes
include_directories(include)


# puts all .cpp files inside src to the SOURCES variable
file(GLOB SOURCES src/*.cpp)

# compiles the files defined by SOURCES to generante the executable defined by EXEC
#add_executable(${EXEC} ${SOURCES})

add_library(${LIB}_static STATIC ${SOURCES})
add_library(${LIB}_shared SHARED ${SOURCES})

#target_link_libraries(${EXEC} dl)

add_subdirectory (drivers)
add_subdirectory (test)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
