find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/resources/hyscan499-resources.c
                    COMMAND ${RESOURCE_COMPILER} --generate-source
                            --target ${CMAKE_BINARY_DIR}/resources/hyscan499-resources.c
                            hyscan499-resources.xml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    DEPENDS common.ui
                    VERBATIM)

set (TRIPLE_LIBS ${GLIB2_LIBRARIES}
                 ${GTK3_LIBRARIES}
                 ${HYSCAN_LIBRARIES}
                 ${HYSCAN_FNN_LIBRARY}
                 ${MATH_LIBRARIES})

add_executable (hyscan499
                app.c
                ${CMAKE_BINARY_DIR}/resources/hyscan499-resources.c)

add_executable (configurator
                configurator.c)

if (${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set_property (TARGET hyscan499 PROPERTY WIN32_EXECUTABLE true)
    set_property (TARGET configurator PROPERTY WIN32_EXECUTABLE true)
  endif()

  target_sources (hyscan499 PRIVATE app.rc icon.ico)
  target_sources (configurator PRIVATE app.rc icon.ico)

  add_executable (launcher launcher.c launcher.rc)
  target_link_libraries (launcher Shlwapi)
  set_property (TARGET launcher PROPERTY WIN32_EXECUTABLE true)
endif()

target_link_libraries (hyscan499 ${TRIPLE_LIBS})
target_link_libraries (configurator ${GLIB2_LIBRARIES}
                                    ${GTK3_LIBRARIES})

