#pragma once

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