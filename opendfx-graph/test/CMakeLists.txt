set(EXEC testDFX)

file(GLOB SOURCES *.c)

add_executable(${EXEC} ${SOURCES})


target_link_libraries(${EXEC} dfxgraph_shared dl)
