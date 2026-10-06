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
        "${CMAKE_CURRENT_SOURCE_DIR}/udp_connection_test.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/connection/udp_connection.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/log.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/lirasdk_impl.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/lira_protocol_receiver.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/lira_distributing_receiver.cpp"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/connection.cpp"
    INCLUDES
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/include/lira/"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/core/connection/"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/"
        "${BASE_SOURCE_PATH}/LIRA_SDK/src/lira_sdk/liradef/"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/"
    LIBRARIES
        Threads::Threads
        fmt::fmt
        GTest::gtest_main
)
