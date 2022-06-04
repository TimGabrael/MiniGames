cmake_minimum_required(VERSION 3.4)


if(WIN32)
	add_executable(MiniGames WIN32 MiniGames.cpp ${ADDITIONAL_SOURCES} ${COMMON_SOURCES} "CustomWidgets/CustomButton.h" "CustomWidgets/CustomButton.cpp" "UtilFuncs.cpp" "UtilFuncs.h" "Frames/MainMenuFrame.h" "Frames/MainMenuFrame.cpp" "CustomWidgets/CustomTextInput.h" "CustomWidgets/CustomTextInput.cpp" "Frames/LobbyFrame.cpp" "Frames/LobbyFrame.h" "CustomWidgets/CustomScrollBar.h" "CustomWidgets/CustomScrollBar.cpp" "Widgets/Header.cpp" "Widgets/Header.h" "Frames/SettingsFrame.cpp" "Frames/SettingsFrame.h" "CustomWidgets/CustomCheckBox.h" "CustomWidgets/CustomCheckBox.cpp" "Application.h" "Application.cpp" "Animations/GrowingCircle.h" "Animations/GrowingCircle.cpp")
else()
	add_executable(MiniGames MiniGames.cpp "CameraPanel.cpp" "CameraPanel.h")
endif()

include_directories(${Qt6Widgets_INCLUDE_DIRS} ${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS})
target_link_libraries(MiniGames Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Svg ${ADDITIONAL_LIBS})


set_target_properties(MiniGames PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY})