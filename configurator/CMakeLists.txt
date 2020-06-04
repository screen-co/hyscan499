add_definitions (-DGETTEXT_PACKAGE="hyscan-499")

add_executable (configurator configurator.c
                             hyscan-configurator.c
                             hyscan-migrate-config.c
                             hyscan-gtk-configurator.c)

add_executable (migrate-config migrate-config.c
                               hyscan-migrate-config.c)

target_link_libraries (configurator ${GTK3_LIBRARIES} ${HYSCAN_LIBRARIES})
target_link_libraries (migrate-config ${GLIB2_LIBRARIES} ${HYSCAN_LIBRARIES})

if ((${CMAKE_C_COMPILER_ID} STREQUAL MSVC) AND (${CMAKE_BUILD_TYPE} STREQUAL "Release"))
  set_property (TARGET configurator PROPERTY WIN32_EXECUTABLE true)
endif()

install (TARGETS configurator
                 migrate-config
         COMPONENT runtime
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
