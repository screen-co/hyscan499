add_definitions (-DGETTEXT_PACKAGE="hyscan-499")

find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

add_custom_command (OUTPUT "${CMAKE_BINARY_DIR}/resources/hyscan-gtk-navigator-resources.c"
                    COMMAND ${RESOURCE_COMPILER} --generate-source
                            --target "${CMAKE_BINARY_DIR}/resources/hyscan-gtk-navigator-resources.c"
                            hyscan-gtk-navigator-resources.xml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    DEPENDS hyscan-gtk-navigator-resources.xml
                    DEPENDS gtk/menus.ui
                    VERBATIM)

add_executable (navigator
                navigator.c
                hyscan-gtk-navigator-app.c
                ${CMAKE_BINARY_DIR}/resources/hyscan-gtk-navigator-resources.c)

target_link_libraries (navigator ${GLIB2_LIBRARIES}
                                 ${GTK3_LIBRARIES}
                                 ${HYSCAN_FNN_LIBRARY}
                                 ${HYSCAN_LIBRARIES})

