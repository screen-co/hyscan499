set (AME_SW_UI software)

add_library (${AME_SW_UI} SHARED
             ame-ui.c)

target_link_libraries (${AME_SW_UI}
                       ${GLIB2_LIBRARIES}
                       ${GTK3_LIBRARIES}
                       ${HYSCAN_LIBRARIES}
                       ${HYSCAN_AME_LIBRARY}
                       ${MATH_LIBRARIES})

set_target_properties (${AME_SW_UI} PROPERTIES PREFIX "")
set_target_properties (${AME_SW_UI} PROPERTIES SUFFIX ".ui")
