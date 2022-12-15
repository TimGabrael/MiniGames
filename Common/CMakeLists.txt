cmake_minimum_required(VERSION 3.4)


set(LOCAL_COLLECTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/FileQuery.cpp ${CMAKE_CURRENT_LIST_DIR}/Audio/WavFile.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/NetworkBase.cpp)
set(OPENGL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/Graphics/GLCompat.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/PbrRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Simple3DRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/UiRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Camera.cpp)




add_library(Uno SHARED "Plugins/Uno/Uno.cpp" "Plugins/Uno/Card.h" "Plugins/Uno/Card.cpp" ${IMGUI_BASE_SOURCES} ${LOCAL_COLLECTION_SOURCES} ${OPENGL_SOURCES} "Plugins/Uno/Animator.h" "Plugins/Uno/Animator.cpp" "Plugins/Uno/Pointer.h" "Plugins/Uno/Pointer.cpp" "Graphics/Scene.h" "Graphics/Scene.cpp" "Graphics/Renderer.h" "Graphics/Renderer.cpp" "Graphics/ReflectiveSurfaceRendering.cpp" "Graphics/ReflectiveSurfaceRendering.h" "Graphics/BloomRendering.h" "Graphics/BloomRendering.cpp" "Graphics/AmbientOcclusionRendering.h" "Graphics/AmbientOcclusionRendering.cpp" "Network/NetworkBase.h" "Network/NetworkBase.cpp" "Network/NetServer.h" "Network/NetServer.cpp" "Plugins/ServerPlugin.h")


if(EMSCRIPTEN)
set_target_properties(Uno PROPERTIES LINK_FLAGS "-s FULL_ES3=1 -s USE_GLFW=3")
endif()

if(WIN32)
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32.lib PARENT_SCOPE)
	set_target_properties(Uno PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY}/Plugins)
	target_link_libraries(Uno PUBLIC ws2_32.lib)
endif()

if(ANDROID)
  	target_link_libraries(Uno PUBLIC log android EGL GLESv3)
endif()

set(COMMON_SOURCES ${LOCAL_COLLECTION_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/Network/Networking.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/Cryptography.cpp PARENT_SCOPE)