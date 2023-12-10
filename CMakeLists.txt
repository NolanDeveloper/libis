cmake_minimum_required(VERSION 3.2)

project(libis)

include(CTest)

set(CMAKE_C_STANDARD 11)

add_subdirectory(include)
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(test-unit)
endif()
