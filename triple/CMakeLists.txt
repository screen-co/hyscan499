
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
if (NOT SONAR_DRIVERS_PATH)
  set (SONAR_DRIVERS_PATH ".")
endif ()
add_definitions (-DSONAR_DRIVERS_PATH="${SONAR_DRIVERS_PATH}")

add_executable (triple
                hyscan-ame-project.c
                hyscan-ame-splash.c
                hyscan-ame-button.c
                hyscan-ame-fixed.c
                hyscan-gtk-forward-look.c
                hyscan-fl-coords.c
                hyscan-mark-sync.c
                sonar-configure.c
                triple.c
                ${CMAKE_BINARY_DIR}/resources/triple-resources.c)

add_executable (ame-fixed-test
                hyscan-ame-button.c
                hyscan-ame-fixed.c
                ame-fixed-test.c)
add_executable (mark-sync-test
                mark-sync-test.c
                hyscan-mark-sync.c)
add_executable (ame-project-test
                hyscan-ame-project.c
                ame-project-test.c)

target_link_libraries (triple ${GLIB_LIBRARIES} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES} ${MATH_LIBRARIES})
target_link_libraries (ame-fixed-test ${GLIB_LIBRARIES} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES} ${MATH_LIBRARIES})
target_link_libraries (mark-sync-test ${GLIB_LIBRARIES} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES} ${MATH_LIBRARIES})
target_link_libraries (ame-project-test ${GLIB_LIBRARIES} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES} ${MATH_LIBRARIES})
