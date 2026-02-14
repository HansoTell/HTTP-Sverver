cmake_minimum_required(VERSION 3.28.3)

project(HTTP_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

# ============================================
# Options
# ============================================

set(PRODUCTION_BUILD OFF CACHE BOOL "Make prod Build" FORCE)

# ============================================
# Third Party Libraries
# ============================================

# Disable gtest install
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/googletest)
add_subdirectory(thirdparty/GameNetworkingSockets)

# ============================================
# Collect Source Files (except main.cpp)
# ============================================

file(GLOB_RECURSE MY_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Remove main.cpp from library sources
list(FILTER MY_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# ============================================
# Create Library (Core Code)
# ============================================

add_library(HTTP_Server_Lib ${MY_SOURCES})

target_include_directories(HTTP_Server_Lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(HTTP_Server_Lib
    PUBLIC
        GameNetworkingSockets
)

# ============================================
# Create Executable (main only)
# ============================================

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        HTTP_Server_Lib
)

# ============================================
# Compile Definitions
# ============================================

if(PRODUCTION_BUILD)

    target_compile_definitions(${PROJECT_NAME} PUBLIC
        RESOURCES_PATH="./resources/"
        PRODUCTION_BUILD=1
        DEVELOPLEMT_BUILD=0
    )

else()

    target_compile_definitions(${PROJECT_NAME} PUBLIC
        RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
        PRODUCTION_BUILD=0
        DEVELOPLEMT_BUILD=1
    )

endif()

# ============================================
# MSVC Specific
# ============================================

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /UUNICODE /U_UNICODE)
endif()

# ============================================
# Tests
# ============================================

add_subdirectory(tests)
