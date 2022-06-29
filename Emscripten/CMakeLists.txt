

# NOT BUILD THIS FOR NOW
#set(CMAKE_CXX_FLAGS "-s ASYNCIFY=1")
#add_executable(MiniGames ${COMMON_SOURCES})






add_executable(Wrapper "CanvasWrapper/WrapperMain.cpp")
target_include_directories(Wrapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party/glfw/include ${CMAKE_CURRENT_SOURCE_DIR}/../Common)
target_link_libraries(Wrapper glfw Uno)
set_target_properties(Wrapper PROPERTIES LINK_FLAGS "-s FULL_ES3=1 -s USE_GLFW=3 -sALLOW_MEMORY_GROWTH=1 --preload-file ../../../bin/Assets/@Assets/")
















add_custom_command(TARGET Wrapper POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.data ${CMAKE_CURRENT_SOURCE_DIR}/Site)

add_custom_command(TARGET Wrapper POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.js ${CMAKE_CURRENT_SOURCE_DIR}/Site)

add_custom_command(TARGET Wrapper POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                     ${CMAKE_CURRENT_SOURCE_DIR}/../build/emscripten/Emscripten/Wrapper.wasm ${CMAKE_CURRENT_SOURCE_DIR}/Site)

