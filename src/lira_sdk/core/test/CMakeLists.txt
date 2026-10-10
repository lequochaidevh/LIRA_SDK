cmake_minimum_required(VERSION 3.12)
project(TestCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_subdirectory(call_every_handler_test)
# add_subdirectory(connection_result_test)
# add_subdirectory(udp_connection_test)
add_subdirectory(lira_protocol_message_handler_test)
