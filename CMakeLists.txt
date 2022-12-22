cmake_minimum_required(VERSION 3.4)
project(MiniGames)

set(CMAKE_CXX_STANDARD 17)

set(MY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
add_compile_definitions(PROTOBUF_USE_DLLS)


set(ADDITIONAL_SOURCES "")
set(COMMON_SOURCES "")
set(ADDITIONAL_LIBS "")
set(IMGUI_BASE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/ImGui/imgui.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/ImGui/imgui_draw.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/ImGui/imgui_demo.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/ImGui/imgui_widgets.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/ImGui/imgui_tables.cpp")


set(GameNetworkingSocketsPath "D:/SDKs/GameNetworkingSockets")

if(NOT EMSCRIPTEN)
add_subdirectory(3rd-party/glad)
include_directories(${GLAD_INCLUDE_DIRS})
link_libraries(${GLAD_LIBRARIES})
add_subdirectory(3rd-party/NFDriver)
include_directories(3rd-party/NFDriver/include)
link_libraries(NFDriver)
endif()

include_directories(3rd-party/glm)
include_directories(3rd-party/ImGui)

include_directories(${GameNetworkingSocketsPath}/include/)
link_libraries(${GameNetworkingSocketsPath}/lib/GameNetworkingSockets.lib)

include_directories(Common)
add_subdirectory(Common)




if(WIN32 OR (UNIX AND NOT ANDROID AND NOT EMSCRIPTEN))
	add_subdirectory(Server)

	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	find_package(Qt6 REQUIRED COMPONENTS Widgets)
	find_package(Qt6 REQUIRED COMPONENTS Core)
	find_package(Qt6 REQUIRED COMPONENTS Gui)
	find_package(Qt6 REQUIRED COMPONENTS svg)
	find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
	

	if(APPLE) #MACOS
		
	elseif(UNIX) #LINUX
	
	else() #WINDOWS
		add_subdirectory(Windows)
		include_directories(Windows)
	endif()

	
	add_subdirectory(Desktop)
	add_subdirectory(PluginCLI)

elseif(EMSCRIPTEN)
	add_subdirectory(Emscripten)
else()
	if(ANDROID)
		add_subdirectory(Android/src/)
	endif()
	
endif()
