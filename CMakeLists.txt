cmake_minimum_required(VERSION 3.4)
project(MiniGames)

set(CMAKE_CXX_STANDARD 11)

set(MY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)



set(ADDITIONAL_SOURCES "")
set(COMMON_SOURCES "")
set(ADDITIONAL_LIBS "")

set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_DISABLE_RTTI ON CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd-party/protobuf)
link_libraries(libprotobuf libprotobuf-lite libprotoc)



if(NOT EMSCRIPTEN)
add_subdirectory(3rd-party/glad)
include_directories(${GLAD_INCLUDE_DIRS})
link_libraries(${GLAD_LIBRARIES})
add_subdirectory(3rd-party/NFDriver)
include_directories(3rd-party/NFDriver/include)
link_libraries(NFDriver)
endif()

include_directories(3rd-party/glm)

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
elseif(EMSCRIPTEN)
	add_subdirectory(Emscripten)
else()
	if(ANDROID)
		add_subdirectory(Android/src/)
	endif()
	
endif()
