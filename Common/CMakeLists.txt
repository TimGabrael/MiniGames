cmake_minimum_required(VERSION 3.4)

set(COMMON_SOURCES ${COMMON_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.h ${CMAKE_CURRENT_LIST_DIR}/Network/Cryptography.cpp PARENT_SCOPE)


add_library(Uno SHARED ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Plugins/Uno.cpp)

if(WIN32)
set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32.lib PARENT_SCOPE)
endif()