add_executable(libis_tests main.c)

target_link_libraries(libis_tests
        PUBLIC libis)

add_test(unit libis_tests)
