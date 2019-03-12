set (TEST_LIBRARIES ${GTK3_LIBRARIES}
                    ${LIBXML2_LIBRARIES}
                    ${HYSCAN_LIBRARIES}
                    ${HYSCAN_FNN_LIBRARY}
                    ${MATH_LIBRARIES})

add_executable (fnn-fixed-test fnn-fixed-test.c)
add_executable (fnn-meter fnn-meter.c)
add_executable (fnn-box-test fnn-box-test.c)
add_executable (mark-sync-test mark-sync-test.c)
add_executable (fnn-project-test fnn-project-test.c)
add_executable (button-emu button-emu.c)

target_link_libraries (fnn-fixed-test ${TEST_LIBRARIES})
target_link_libraries (fnn-meter ${TEST_LIBRARIES})
target_link_libraries (fnn-box-test ${TEST_LIBRARIES})
target_link_libraries (mark-sync-test ${TEST_LIBRARIES})
target_link_libraries (fnn-project-test ${TEST_LIBRARIES})
target_link_libraries (button-emu ${TEST_LIBRARIES})
