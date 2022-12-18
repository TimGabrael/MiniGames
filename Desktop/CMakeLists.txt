cmake_minimum_required(VERSION 3.4)


if(WIN32)
	add_executable(MiniGames WIN32 MiniGames.cpp ${CMAKE_CURRENT_LIST_DIR}/../Common/Network/BaseMessages.pb.cc ${ADDITIONAL_SOURCES} ${COMMON_SOURCES} ${IMGUI_BASE_SOURCES} "CustomWidgets/CustomButton.h" "CustomWidgets/CustomButton.cpp" "UtilFuncs.cpp" "UtilFuncs.h" "Frames/MainMenuFrame.h" "Frames/MainMenuFrame.cpp" "CustomWidgets/CustomTextInput.h" "CustomWidgets/CustomTextInput.cpp" "Frames/LobbyFrame.cpp" "Frames/LobbyFrame.h" "CustomWidgets/CustomScrollBar.h" "CustomWidgets/CustomScrollBar.cpp" "Widgets/Header.cpp" "Widgets/Header.h" "Frames/SettingsFrame.cpp" "Frames/SettingsFrame.h" "CustomWidgets/CustomCheckBox.h" "CustomWidgets/CustomCheckBox.cpp" "Application.h" "Application.cpp" "Animations/GrowingCircle.h" "Animations/GrowingCircle.cpp"  "util/FileStorage.h" "util/FileStorage.cpp" "Frames/PluginFrame.h" "Frames/PluginFrame.cpp" "util/PluginLoader.h" "util/PluginLoader.cpp" "CustomWidgets/PluginWidget.cpp" "CustomWidgets/PluginWidget.h" "CustomWidgets/InfoPopup.h" "CustomWidgets/InfoPopup.cpp" "Frames/StateFrame.h" "util/QImGui.h" "util/QImGui.cpp")
else()
	add_executable(MiniGames MiniGames.cpp "CameraPanel.cpp" "CameraPanel.h")
endif()

include_directories(${Qt6Widgets_INCLUDE_DIRS} ${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS})
target_link_libraries(MiniGames Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Svg Qt6::OpenGLWidgets ${ADDITIONAL_LIBS}  ${Protobuf_LIBRARIES})


set_target_properties(MiniGames PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_OUTPUT_DIRECTORY})