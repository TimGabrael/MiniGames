cmake_minimum_required(VERSION 3.4)

project(MiniGames)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS svg)


set(MY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)



set(ADDITIONAL_SOURCES "")
set(COMMON_SOURCES "")
set(ADDITIONAL_LIBS "")

add_subdirectory(Common)
add_subdirectory(Server)
include_directories(Common)

if(WIN32 OR UNIX)

	if(APPLE) #MACOS
		
	elseif(UNIX) #LINUX
	
	else() #WINDOWS
		add_subdirectory(Windows)
		include_directories(Windows)
	endif()


	add_subdirectory(Desktop)
else()
	
endif()