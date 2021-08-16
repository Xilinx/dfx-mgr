file(GLOB SOURCES_CPP *.cpp)

add_executable(testGraph testGraph.cpp)
add_executable(testGraphAPI testGraphAPI.c)
add_executable(testJson testJson.cpp)
add_executable(testJsonAPI testJsonAPI.c)
add_executable(testGraphManager testGraphManager.cpp)
add_executable(testGraphClient testGraphClient.cpp)

target_link_libraries(testGraph dfxgraph_shared dl pthread)
target_link_libraries(testGraphAPI dfxgraph_shared dl pthread)
target_link_libraries(testJson dfxgraph_shared dl pthread)
target_link_libraries(testJsonAPI dfxgraph_shared dl pthread)
target_link_libraries(testGraphManager dfxgraph_shared dl pthread)
target_link_libraries(testGraphClient dfxgraph_shared dl pthread)

