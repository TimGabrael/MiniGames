cmake_minimum_required(VERSION 3.4)




add_executable(Server Server.cpp ${COMMON_SOURCES})
target_include_directories(Server PUBLIC ${CMAKE_SOURCE_DIR}/Common)
target_link_libraries(Server PRIVATE ws2_32.lib)

set_target_properties(Server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY})