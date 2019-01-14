find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/resources/triple-resources.c
                    COMMAND ${RESOURCE_COMPILER} --generate-source
                            --target ${CMAKE_BINARY_DIR}/resources/triple-resources.c
                            triple-resources.xml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    DEPENDS triple.ui common.ui
                    VERBATIM)

set (TRIPLE_LIBS ${GLIB_LIBRARIES}
                 ${GTK3_LIBRARIES}
                 ${HYSCAN_LIBRARIES}
                 ${HYSCAN_AME_LIBRARY}
                 ${MATH_LIBRARIES})

add_executable (triple
                ame-ui.c
                triple.c
                ${CMAKE_BINARY_DIR}/resources/triple-resources.c)

target_link_libraries (triple ${TRIPLE_LIBS})
