cmake_minimum_required(VERSION 3.10)
project(PosixPipeProject LANGUAGES CXX)

# 1. ONE-LINE BOOTSTRAP: Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "projects") # find BASE_SOURCE_PATH
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

# Load the modern automated framework builder
include(GTestSetup)

# 2. Register your executable directly using industry-standard explicit parameters
enable_testing()

register_gtest_target(sandbox
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/call_every_handler_test.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/call_every_handler.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/lirasdk_time.cpp"
    INCLUDES
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/include/lira/"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/"
    LIBRARIES
        Threads::Threads
        fmt::fmt
        GTest::gtest_main
)

# "${INTERNAL_ROOT_SEARCH_PATH}"
