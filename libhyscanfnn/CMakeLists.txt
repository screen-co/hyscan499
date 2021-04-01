# Поиск GLib компилятора ресурсов.
find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

# Генерация ресурсного C-файла.
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/resources/hyscan-fnn-resources.c
                    COMMAND ${RESOURCE_COMPILER}
                    --generate-source
                    --target ${CMAKE_BINARY_DIR}/resources/hyscan-fnn-resources.c
                    hyscan-fnn-resources.xml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    DEPENDS hyscan-fnn-resources.xml
                            hyscan-gtk-mark-editor.ui
                            hyscan-gtk-dev-indicator.css
                            hyscan-gtk-nav-indicator.ui
                            hyscan-gtk-project-viewer.ui
                            hyscan-gtk-con.ui
                            hyscan-fnn-fl.ui
                            hyscan-gtk-map-preload.ui
                            hyscan-gtk-map-go.ui
                    VERBATIM)

add_library (${HYSCAN_FNN_LIBRARY} SHARED
             fnn-param-proxy.c
             hyscan-fnn-flog.c
             hyscan-fnn-offsets.c
             hyscan-fnn-button.c
             hyscan-fnn-fixed.c
             hyscan-fnn-project.c
             hyscan-fnn-splash.c
             hyscan-fl-coords.c
             hyscan-gtk-fnn-offsets.c
             hyscan-gtk-forward-look.c
             hyscan-gtk-mark-editor.c
             hyscan-gtk-project-viewer.c
             hyscan-gtk-nav-indicator.c
             hyscan-gtk-dev-indicator.c
             hyscan-gtk-fnn-gliko-wrapper.c
             hyscan-gtk-fnn-box.c
             hyscan-gtk-con.c
             hyscan-gtk-rec.c
             hyscan-gtk-map-preload.c
             hyscan-gtk-map-go.c
             hyscan-mark-sync.c
             sonar-configure.c
             fnn.c
             ${CMAKE_BINARY_DIR}/resources/hyscan-fnn-resources.c)

target_link_libraries (${HYSCAN_FNN_LIBRARY} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES})

set_target_properties (${HYSCAN_FNN_LIBRARY} PROPERTIES DEFINE_SYMBOL "HYSCAN_API_EXPORTS")
set_target_properties (${HYSCAN_FNN_LIBRARY} PROPERTIES SOVERSION ${HYSCAN_FNN_VERSION})

install (TARGETS ${HYSCAN_FNN_LIBRARY}
         COMPONENT runtime
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
