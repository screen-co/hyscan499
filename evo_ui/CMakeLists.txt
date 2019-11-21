set (UI_NAME evo)

find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/resources/${UI_NAME}-resources.c
                    COMMAND ${RESOURCE_COMPILER} --generate-source
                            --target ${CMAKE_BINARY_DIR}/resources/${UI_NAME}-resources.c
                            ${UI_NAME}-resources.xml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    DEPENDS evo.ui
                            record.ui
                    VERBATIM)


add_library (${UI_NAME} SHARED
             hyscan-gtk-map-kit.c
             evo-ui.c
             hyscan-gtk-mark-export.c
             ${CMAKE_BINARY_DIR}/resources/${UI_NAME}-resources.c)

target_link_libraries (${UI_NAME}
                       ${GLIB2_LIBRARIES}
                       ${GTK3_LIBRARIES}
                       ${HYSCAN_LIBRARIES}
                       ${HYSCAN_FNN_LIBRARY}
                       ${MATH_LIBRARIES})

set_target_properties (${UI_NAME} PROPERTIES PREFIX "")
set_target_properties (${UI_NAME} PROPERTIES SUFFIX ".ui")
