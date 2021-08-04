
add_executable(testGraph testGraph.cpp)
add_executable(testGraphAPI testGraphAPI.c)
add_executable(testJson testJson.cpp)

target_link_libraries(testGraph dfxgraph_shared dl)
target_link_libraries(testGraphAPI dfxgraph_shared dl)
target_link_libraries(testJson dfxgraph_shared dl)
