cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)

add_executable(PluginCLI PluginResourceCLI.cpp)

set_target_properties(PluginCLI PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY})