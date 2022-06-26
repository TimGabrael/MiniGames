

# NOT BUILD THIS FOR NOW
#set(CMAKE_CXX_FLAGS "-s ASYNCIFY=1")
#add_executable(MiniGames ${COMMON_SOURCES})







add_executable(Wrapper "CanvasWrapper/WrapperMain.cpp")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party/glfw/include)
target_link_libraries(Wrapper glfw)
set_target_properties(Wrapper PROPERTIES COMPILE_FLAGS "-sMAIN_MODULE=1")
set_target_properties(Wrapper PROPERTIES LINK_FLAGS "-sMAIN_MODULE=1 -s FULL_ES3=1 -s USE_GLFW=3 --use-preload-plugins") # --preload-file ../../../bin/Assets/@Assets/
















add_custom_command(TARGET Wrapper POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.data ${CMAKE_CURRENT_SOURCE_DIR}/Site)

add_custom_command(TARGET Wrapper POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.js ${CMAKE_CURRENT_SOURCE_DIR}/Site)

add_custom_command(TARGET Wrapper POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                     ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.wasm ${CMAKE_CURRENT_SOURCE_DIR}/Site)


add_custom_command(TARGET Wrapper POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Common/Uno.wasm ${CMAKE_CURRENT_SOURCE_DIR}/Site)