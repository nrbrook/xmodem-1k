project(XmodemTestsMain CXX)

if (APPLE)
    add_definitions(-DGTEST_USE_OWN_TR1_TUPLE)
    add_definitions(-D__GLIBCXX__)
endif (APPLE)

add_executable(runTests
    xmodemTests.cpp
    )

target_link_libraries(runTests PUBLIC xmodem)
target_link_libraries(runTests PUBLIC gtest gtest_main)
target_link_libraries(runTests PUBLIC gmock gmock_main)