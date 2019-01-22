set (AME_HW_UI hardware)

add_library (${AME_HW_UI} SHARED
             ame-ui.c)

target_link_libraries (${AME_HW_UI}
                       ${GLIB2_LIBRARIES}
                       ${GTK3_LIBRARIES}
                       ${HYSCAN_LIBRARIES}
                       ${HYSCAN_AME_LIBRARY}
                       ${MATH_LIBRARIES})

set_target_properties (${AME_HW_UI} PROPERTIES PREFIX "")
set_target_properties (${AME_HW_UI} PROPERTIES SUFFIX ".ui")
