add_library(MiniGames SHARED native-lib.cpp ${COMMON_SOURCES})

find_library(log-lib log)

target_link_libraries(MiniGames ${log-lib})


add_library(TestNative SHARED native-activity.cpp android_native_app_glue.c ${COMMON_SOURCES})
target_link_libraries(TestNative ${log-lib} android EGL GLESv3)