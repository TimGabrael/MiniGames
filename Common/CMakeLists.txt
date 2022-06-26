cmake_minimum_required(VERSION 3.4)

set(COMMON_SOURCES ${COMMON_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.h ${CMAKE_CURRENT_LIST_DIR}/Network/Cryptography.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.h PARENT_SCOPE)

if(EMSCRIPTEN)
add_executable(Uno ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Plugins/Uno.cpp   "Graphics/PbrRendering.h" "Graphics/PbrRendering.cpp" "Graphics/UiRendering.h" "Graphics/UiRendering.cpp" "Plugins/KeyboardKeys.h" "Graphics/GLCompat.h" "Graphics/GLCompat.cpp" "FileQuery.h" "FileQuery.cpp")
set_target_properties(Uno PROPERTIES COMPILE_FLAGS "-sSIDE_MODULE=1")
set_target_properties(Uno PROPERTIES LINK_FLAGS "-sSIDE_MODULE=1 -s WASM=1 -s FULL_ES3=1 -s USE_GLFW=3 --preload-file ../../../bin/Assets/@Assets/")
else()
add_library(Uno SHARED ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Plugins/Uno.cpp   "Graphics/PbrRendering.h" "Graphics/PbrRendering.cpp" "Graphics/UiRendering.h" "Graphics/UiRendering.cpp" "Plugins/KeyboardKeys.h" "Graphics/GLCompat.h" "Graphics/GLCompat.cpp" "FileQuery.h" "FileQuery.cpp")
endif()

if(WIN32)
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32.lib PARENT_SCOPE)
	set_target_properties(Uno PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY}/Plugins)
endif()

if(ANDROID)
  	target_link_libraries(Uno PUBLIC log android EGL GLESv3)
endif()

