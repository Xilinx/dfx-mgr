collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")

collect (PROJECT_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
collect (PROJECT_INC_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")

collect (PROJECT_LIB_HEADERS accel.h)
collect (PROJECT_LIB_HEADERS assert.h)
collect (PROJECT_LIB_HEADERS dma.h)
collect (PROJECT_LIB_HEADERS device.h)
collect (PROJECT_LIB_HEADERS helper.h)
collect (PROJECT_LIB_HEADERS print.h)
collect (PROJECT_LIB_HEADERS shell.h)
collect (PROJECT_LIB_HEADERS shm.h)
collect (PROJECT_LIB_HEADERS client_helper.h)
collect (PROJECT_LIB_HEADERS model.h)
collect (PROJECT_LIB_HEADERS json-config.h)
collect (PROJECT_LIB_HEADERS daemon_helper.h)

collect (PROJECT_LIB_SOURCES accel.c)
collect (PROJECT_LIB_SOURCES assert.c)
collect (PROJECT_LIB_SOURCES dma.c)
collect (PROJECT_LIB_SOURCES device.c)
collect (PROJECT_LIB_SOURCES shell.c)
collect (PROJECT_LIB_SOURCES shm.c)
collect (PROJECT_LIB_SOURCES client_helper.c)
collect (PROJECT_LIB_SOURCES daemon_helper.c)

add_subdirectory (sys)

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

include_directories (${_inc_dirs})
add_definitions (-DACAPD_INTERNAL)

get_property (_ec_flags GLOBAL PROPERTY "PROJECT_EC_FLAGS")
set (_deps "dfx" "websockets")

if (WITH_SHARED_LIB)
  set (_lib ${PROJECT_NAME}-shared)
  add_library (${_lib} SHARED ${_sources})
  target_link_libraries (${_lib} ${_deps})
  install (TARGETS ${_lib} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  target_compile_options (${_lib} PUBLIC ${_ec_flags})
  set_target_properties (${_lib} PROPERTIES
    OUTPUT_NAME       "${PROJECT_NAME}"
    VERSION           "${PROJECT_VER}"
    SOVERSION         "${PROJECT_VER_MAJOR}"
  )
endif (WITH_SHARED_LIB)

# Build a static library if so configured.
if (WITH_STATIC_LIB)
  set (_lib ${PROJECT_NAME}-static)
  add_library (${_lib} STATIC ${_sources})
  install (TARGETS ${_lib} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  target_compile_options (${_lib} PUBLIC ${_ec_flags})
  set_target_properties (${_lib} PROPERTIES
    OUTPUT_NAME       "${PROJECT_NAME}"
  )
endif (WITH_STATIC_LIB)

# vim: expandtab:ts=2:sw=2:smartindent
