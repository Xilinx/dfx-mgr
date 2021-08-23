file(GLOB SOURCES_CPP *.cpp)

add_executable(testGraph testGraph.cpp)
add_executable(testGraphAPI testGraphAPI.c)
add_executable(testJson testJson.cpp)
add_executable(testJsonAPI testJsonAPI.c)
add_executable(testGraphManager testGraphManager.cpp)
add_executable(testGraphClient testGraphClient.cpp)

target_link_libraries(testGraph dfx-mgr-shared dfxgraph_shared dl pthread)
target_link_libraries(testGraphAPI dfx-mgr-shared dfxgraph_shared dl pthread)
target_link_libraries(testJson dfx-mgr-shared dfxgraph_shared dl pthread)
target_link_libraries(testJsonAPI dfx-mgr-shared dfxgraph_shared dl pthread)
target_link_libraries(testGraphManager dfx-mgr-shared dfxgraph_shared dl pthread)
target_link_libraries(testGraphClient dfx-mgr-shared dfxgraph_shared dl pthread)

