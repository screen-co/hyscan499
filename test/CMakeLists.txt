set (TEST_LIBRARIES ${GTK3_LIBRARIES}
                    ${LIBXML2_LIBRARIES}
                    ${HYSCAN_LIBRARIES}
                    ${HYSCAN_AME_LIBRARY}
                    ${MATH_LIBRARIES})

add_executable (ame-fixed-test ame-fixed-test.c)
add_executable (ame-box-test ame-box-test.c)
add_executable (mark-sync-test mark-sync-test.c)
add_executable (ame-project-test ame-project-test.c)

target_link_libraries (ame-fixed-test ${TEST_LIBRARIES})
target_link_libraries (ame-box-test ${TEST_LIBRARIES})
target_link_libraries (mark-sync-test ${TEST_LIBRARIES})
target_link_libraries (ame-project-test ${TEST_LIBRARIES})
