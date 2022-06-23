cmake_minimum_required(VERSION 3.4)

set(COMMON_SOURCES ${COMMON_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.h ${CMAKE_CURRENT_LIST_DIR}/Network/Cryptography.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.h PARENT_SCOPE)


add_library(Uno SHARED ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Plugins/Uno.cpp   "Graphics/PbrRendering.h" "Graphics/PbrRendering.cpp" "Graphics/UiRendering.h" "Graphics/UiRendering.cpp" "Plugins/KeyboardKeys.h")

if(WIN32)
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32.lib PARENT_SCOPE)
endif()

if(ANDROID)
  	target_link_libraries(Uno PUBLIC log)
endif()

set_target_properties(Uno PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY}/Plugins)