#pragma once

#ifdef __ANDROID__
#include <Android/log.h>
#define LOG(msg, ...) __android_log_print(ANDROID_LOG_ERROR, "TTT", msg, ##__VA_ARGS__)
#else
#include <iostream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <debugapi.h>
//#define LOG(msg, ...) printf_s(msg, ##__VA_ARGS__)
template<typename... Args>
void LOG(const char* fmt, Args&&... args)
{
	static constexpr int MAX_PRINT_SZ = 10000;
	char buf[MAX_PRINT_SZ] = { 0 };
	sprintf_s(buf, MAX_PRINT_SZ, fmt, std::forward<Args>(args)...);
	OutputDebugStringA(buf);
}
#else
#define LOG(msg, ...) printf(msg, ##__VA_ARGS__)
#endif
#endif