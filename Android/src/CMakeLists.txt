add_library(MiniGames SHARED native-lib.cpp ${COMMON_SOURCES})

find_library(log-lib log)

target_link_libraries(MiniGames ${log-lib})