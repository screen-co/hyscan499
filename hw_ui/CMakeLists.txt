set (HW_UI hardware)

add_library (${HW_UI} SHARED
             cheese-flash.c
             hw-ui.c)

target_link_libraries (${HW_UI}
                       ${GLIB2_LIBRARIES}
                       ${GTK3_LIBRARIES}
                       ${HYSCAN_LIBRARIES}
                       ${HYSCAN_FNN_LIBRARY}
                       ${MATH_LIBRARIES})

set_target_properties (${HW_UI} PROPERTIES PREFIX "")
set_target_properties (${HW_UI} PROPERTIES SUFFIX ".ui")

add_executable (cheese-flash-test
                cheese-flash.c
                cheese-flash-test.c)

target_link_libraries (cheese-flash-test ${GTK3_LIBRARIES})
