
cmake_minimum_required (VERSION 2.6)

set (HYSCAN_MAJOR_VERSION 5)
set (HYSCAN_FNN_VERSION 0)
project (hyscan-499)

set (HYSCAN_FNN_LIBRARY "hyscanfnn-${HYSCAN_MAJOR_VERSION}")

include (FindPkgConfig)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif ()

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -Wall -Wextra -Wno-unused-parameter $ENV{CFLAGS}")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers $ENV{CFLAGS}")
  set (MATH_LIBRARIES m)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /wd4244 $ENV{CFLAGS}")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /wd4244 $ENV{CFLAGS}")
else ()
  message (FATAL_ERROR "Unsupported compiler ${CMAKE_C_COMPILER_ID}")
endif ()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

pkg_check_modules (GLIB2 REQUIRED "glib-2.0 gobject-2.0 gthread-2.0 gio-2.0 gmodule-2.0")
pkg_check_modules (GTK3 REQUIRED "gtk+-3.0")
add_definitions (${GLIB2_CFLAGS} ${GTK3_CFLAGS})
link_directories (${GLIB2_LIBRARY_DIRS} ${GTK3_LIBRARY_DIRS})

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../urpc/urpc")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../gtkcifroarea/gtkcifroarea")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscantypes/hyscantypes")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscandriver/hyscandriver")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscandb/hyscandb")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscancache/hyscancache")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscancore/hyscancore")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscanmodel/hyscanmodel")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscangui/hyscangui")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscangtk/hyscangtk")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/libhyscanfnn")


link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../urpc/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../gtkcifroarea/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscantypes/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscandriver/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscandb/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscancache/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscancore/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscanmodel/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscangui/bin")
link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscangtk/bin")
set (HYSCAN_LIBRARIES gtkcifroarea-3.1
                      hyscantypes-${HYSCAN_MAJOR_VERSION}
                      hyscandriver-${HYSCAN_MAJOR_VERSION}
                      hyscandb-${HYSCAN_MAJOR_VERSION}
                      hyscancache-${HYSCAN_MAJOR_VERSION}
                      hyscancore-${HYSCAN_MAJOR_VERSION}
                      hyscanmodel-${HYSCAN_MAJOR_VERSION}
                      hyscangui-${HYSCAN_MAJOR_VERSION}
                      hyscangtk-${HYSCAN_MAJOR_VERSION}
                      urpc)

add_definitions (-DG_LOG_DOMAIN="Fnn")
add_subdirectory (libhyscanfnn)
add_subdirectory (hw_ui)
add_subdirectory (sw_ui)
add_subdirectory (evo_ui)
add_subdirectory (test)
add_subdirectory (fnn)
