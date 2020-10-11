cmake_minimum_required(VERSION 3.6)

project(XmodemProject VERSION "1.0.0" LANGUAGES C)

set_source_files_properties(
        include/xmodem.h
        include/crc16.h
        PROPERTIES
        HEADER_FILE_ONLY TRUE # Don't need compiling
)

add_library(xmodem STATIC
        src/xmodem.c
        src/crc16.c
        )

target_include_directories(xmodem
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
