# Поиск GLib компилятора ресурсов.
find_program (RESOURCE_COMPILER glib-compile-resources)
if (${RESOURCE_COMPILER} STREQUAL RESOURCE_COMPILER-NOTFOUND)
  message (FATAL_ERROR "GLib resource compiler not found.")
endif ()

# Генерация ресурсного C-файла.
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
execute_process (COMMAND ${RESOURCE_COMPILER}
                 --generate-source
                 --target ${CMAKE_BINARY_DIR}/resources/hyscan-ame-resources.c
                 hyscan-ame-resources.xml
                 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_library (${HYSCAN_AME_LIBRARY} SHARED
             hyscan-ame-button.c
             hyscan-ame-fixed.c
             hyscan-ame-project.c
             hyscan-ame-splash.c
             hyscan-fl-coords.c
             hyscan-gtk-forward-look.c
             hyscan-gtk-mark-editor.c
             hyscan-gtk-project-viewer.c
             hyscan-gtk-nav-indicator.c
             hyscan-gtk-ame-box.c
             hyscan-mark-sync.c
             sonar-configure.c
             ${CMAKE_BINARY_DIR}/resources/hyscan-ame-resources.c)

target_link_libraries (${HYSCAN_AME_LIBRARY} ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES})

set_target_properties (${HYSCAN_AME_LIBRARY} PROPERTIES DEFINE_SYMBOL "HYSCAN_API_EXPORTS")
set_target_properties (${HYSCAN_AME_LIBRARY} PROPERTIES SOVERSION ${HYSCAN_AME_VERSION})

# configure_file (hyscangtk.pc.in
#                 "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hyscangtk-${HYSCAN_MAJOR_VERSION}.pc"
#                 @ONLY)

install (TARGETS ${HYSCAN_AME_LIBRARY}
         COMPONENT runtime
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
