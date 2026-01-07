#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <string_view>
#include <mutex>
#include <chrono>
#include <format>
#include <source_location>



#define PRINTLOG(fmt, ...) \
{ \
	wchar_t logBuffer[1024]; \
	swprintf_s(logBuffer, 1024, fmt, __VA_ARGS__); \
	std::cout <<["SERVER"]<< logBuffer; \
	OutputDebugStringW(logBuffer); \
}



#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}


#ifdef _DEBUG

static std::mutex logMutex;

// std::source_location으로 호출부 정보를 자동으로 가져옴

static void LogInternal(std::string_view level, std::string_view formatted_msg, const std::source_location& location) {
    const auto now = std::chrono::system_clock::now();


    std::string logEntry = std::format(
        "[{:%F %T}] [{}] {}:{} | {}\n",
        now, level, location.file_name(), location.line(), formatted_msg
    );

    // std::lock_guard<std::mutex> lock(logMutex);
    std::cout << logEntry;
}


template <typename... Args>
static void LogHelper(std::string_view level,
    const std::source_location location,
    std::string_view fmt,
    Args&&... args) {


    try {
        std::string formatted_msg = std::vformat(fmt, std::make_format_args(args...));
        LogInternal(level, formatted_msg, location);
    }
    catch (const std::format_error& e) {
        LogInternal("ERROR", std::string("Format Error: ") + e.what(), location);
    }
}

#define LOG_INFO(fmt, ...)  LogHelper("INFO",  std::source_location::current(), fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LogHelper("WARN",  std::source_location::current(), fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LogHelper("ERROR", std::source_location::current(), fmt, ##__VA_ARGS__)



// ANSI
static void LogDebug(const std::string& msg) {
	std::string output = "[LOG] " + msg + "\n";
	OutputDebugStringA(output.c_str());
}

// Unicode
static void LogDebugW(const std::wstring& msg) {
	std::wstring output = L"[LOG] " + msg + L"\n";
	OutputDebugStringW(output.c_str());
}

#else


#endif

/*
LogHelper("DEBUG", "Entity[{}] State: {}, Pos: ({:.2f}, {:.2f})",
	entityID, state, posX, posY);
*/
