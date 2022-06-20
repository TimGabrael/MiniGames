#pragma once

#ifdef __ANDROID__
#include <Android/log.h>
#define LOG(msg, ...) __android_log_print(ANDROID_LOG_ERROR, "TTT", msg, ##__VA_ARGS__)
#else
#include <iostream>
#ifdef _WIN32
#define LOG(msg, ...) printf_s(msg, ##__VA_ARGS__)
#else
#define LOG(msg, ...) printf(msg, ##__VA_ARGS__)
#endif
#endif