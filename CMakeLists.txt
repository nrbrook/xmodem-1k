cmake_minimum_required(VERSION 3.12)

project(XmodemProject)

set(CMAKE_C_STANDARD 99)

add_subdirectory(src)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    add_subdirectory(tests)
endif()