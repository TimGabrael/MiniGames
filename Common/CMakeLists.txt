cmake_minimum_required(VERSION 3.4)


set(LOCAL_COLLECTION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/FileQuery.cpp ${CMAKE_CURRENT_LIST_DIR}/Audio/WavFile.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/Cryptography.cpp)
set(OPENGL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/Graphics/GLCompat.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Helper.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/PbrRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Simple3DRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/UiRendering.cpp ${CMAKE_CURRENT_LIST_DIR}/Graphics/Camera.cpp)




add_library(Uno SHARED "Plugins/Shared/Uno/UnoMessages.pb.cc" "Plugins/Client/Uno/Uno.cpp" "Plugins/Client/Uno/Card.h" "Plugins/Client/Uno/Card.cpp" ${IMGUI_BASE_SOURCES} ${LOCAL_COLLECTION_SOURCES} ${OPENGL_SOURCES} "Plugins/Client/Uno/Animator.h" "Plugins/Client/Uno/Animator.cpp" "Plugins/Client/Uno/Pointer.h" "Plugins/Client/Uno/Pointer.cpp" "Graphics/Scene.h" "Graphics/Scene.cpp" "Graphics/Renderer.h" "Graphics/Renderer.cpp" "Graphics/ReflectiveSurfaceRendering.cpp" "Graphics/ReflectiveSurfaceRendering.h" "Graphics/BloomRendering.h" "Graphics/BloomRendering.cpp" "Graphics/AmbientOcclusionRendering.h" "Graphics/AmbientOcclusionRendering.cpp" "Network/NetworkBase.h" "Plugins/Shared/Uno/UnoBase.h" "Plugins/Client/Uno/NetHandlers.cpp")

add_library(UnoServer SHARED "Plugins/Server/Uno/Uno.cpp" "Plugins/Shared/Uno/UnoBase.h" "Plugins/Shared/Uno/UnoMessages.pb.cc")

if(EMSCRIPTEN)
set_target_properties(Uno PROPERTIES LINK_FLAGS "-s FULL_ES3=1 -s USE_GLFW=3")
endif()


if(WIN32)
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32.lib PARENT_SCOPE)
	set_target_properties(Uno PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY}/Plugins)
	target_link_libraries(Uno PRIVATE ws2_32.lib ${Protobuf_LIBRARIES})

	set_target_properties(UnoServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY}/ServerPlugins)
	target_link_libraries(UnoServer PRIVATE ws2_32.lib ${Protobuf_LIBRARIES})

endif()

if(ANDROID)
  	target_link_libraries(Uno PUBLIC log android EGL GLESv3)
endif()

set(COMMON_SOURCES ${LOCAL_COLLECTION_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/Network/NetServer.cpp ${CMAKE_CURRENT_LIST_DIR}/Network/NetClient.cpp  ${CMAKE_CURRENT_LIST_DIR}/Network/NetCommon.cpp  ${CMAKE_CURRENT_LIST_DIR}/Network/BaseMessages.pb.cc PARENT_SCOPE)
