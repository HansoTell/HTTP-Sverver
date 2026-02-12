file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(HTTP_Server_Tests ${TEST_SOURCES})

target_link_libraries(HTTP_Server_Tests
    PRIVATE
        gtest
        gmock
        gtest_main
        GameNetworkingSockets
)

target_include_directories(HTTP_Server_Tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME HTTP_Server_Tests COMMAND HTTP_Server_Tests)
