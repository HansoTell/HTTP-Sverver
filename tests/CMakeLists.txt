file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(HTTP_Server_Tests ${TEST_SOURCES})

target_link_libraries(HTTP_Server_Tests
    PRIVATE
        HTTP_Server_Lib
        gtest
        gmock
        gtest_main
        GameNetworkingSockets
)


add_test(NAME HTTP_Server_Tests COMMAND HTTP_Server_Tests)
