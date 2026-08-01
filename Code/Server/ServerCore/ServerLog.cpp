#include "pch.h"
#include "ServerLog.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace
{
	std::mutex gSinkMutex; 
	std::ofstream gLogFile;
	std::atomic<bool> gInitialized{ false };


	std::mutex gThrottleMutex;
	std::unordered_map<std::string, std::uint64_t> gOnceKeys;

	struct ThrottleEntry
	{
		std::chrono::steady_clock::time_point lastEmit{};
		std::uint64_t suppressed = 0;
		bool everEmitted = false;
	};
	std::unordered_map<std::string, ThrottleEntry> gEveryKeys;

	const char* LevelName(ServerLog::Level level)
	{
		switch (level)
		{
		case ServerLog::Level::Info:  return "INFO ";
		case ServerLog::Level::Warn:  return "WARN ";
		case ServerLog::Level::Error: return "ERROR";
		default:                      return "?????";
		}
	}


	const char* BaseName(const char* path)
	{
		if (path == nullptr)
			return "?";

		const char* last = path;
		for (const char* p = path; *p != '\0'; ++p)
		{
			if (*p == '\\' || *p == '/')
				last = p + 1;
		}
		return last;
	}


	const char* CachedTimestamp(int& outMilliseconds)
	{
		using namespace std::chrono;

		static thread_local char sBuffer[32] = {};
		static thread_local std::time_t sCachedSecond = 0;

		const auto now = system_clock::now();
		const auto sinceEpoch = now.time_since_epoch();
		const std::time_t second =
			static_cast<std::time_t>(duration_cast<seconds>(sinceEpoch).count());
		outMilliseconds =
			static_cast<int>(duration_cast<milliseconds>(sinceEpoch).count() % 1000);

		if (second != sCachedSecond || sBuffer[0] == '\0')
		{
			sCachedSecond = second;
			std::tm tmValue{};
			::localtime_s(&tmValue, &second);
			std::strftime(sBuffer, sizeof(sBuffer), "%m-%d %H:%M:%S", &tmValue);
		}
		return sBuffer;
	}


	void DisableConsoleQuickEdit()
	{
		const HANDLE input = ::GetStdHandle(STD_INPUT_HANDLE);
		if (input == nullptr || input == INVALID_HANDLE_VALUE)
			return;

		DWORD mode = 0;
		if (!::GetConsoleMode(input, &mode))
			return;

		mode &= ~static_cast<DWORD>(ENABLE_QUICK_EDIT_MODE);
		mode |= ENABLE_EXTENDED_FLAGS;
		::SetConsoleMode(input, mode);
	}
}

namespace ServerLog
{
	void Initialize()
	{
		bool expected = false;
		if (!gInitialized.compare_exchange_strong(expected, true))
			return;

		DisableConsoleQuickEdit();

		::SetConsoleOutputCP(CP_UTF8);

		std::ios::sync_with_stdio(false);

		std::error_code ec;
		std::filesystem::create_directories("Logs", ec);

		int milliseconds = 0;
		const std::time_t now =
			std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm tmValue{};
		::localtime_s(&tmValue, &now);

		char fileName[64] = {};
		std::strftime(fileName, sizeof(fileName), "Logs/server_%Y%m%d_%H%M%S.log", &tmValue);

		gLogFile.open(fileName, std::ios::out | std::ios::app);
		(void)milliseconds;

		MJLOG_INFO(Startup, "로그 시작 — 파일 {}", fileName);
	}

	void Shutdown()
	{
		const std::scoped_lock lock(gSinkMutex);
		if (gLogFile.is_open())
		{
			gLogFile.flush();
			gLogFile.close();
		}
		std::cout.flush();
	}

	void Emit(
		Domain domain,
		Level level,
		const std::source_location& location,
		std::string_view message)
	{
		int milliseconds = 0;
		const char* stamp = CachedTimestamp(milliseconds);

		std::string line;
		line.reserve(message.size() + 96);
		line += '[';
		line += stamp;
		line += '.';
		{
			char ms[8] = {};
			std::snprintf(ms, sizeof(ms), "%03d", milliseconds);
			line += ms;
		}
		line += "] [";
		line += LevelName(level);
		line += "] [";
		line += Name(domain);
		line += "] ";
		line += message;
		line += "  (";
		line += BaseName(location.file_name());
		line += ':';
		line += std::to_string(location.line());
		line += ")\n";

		const std::scoped_lock lock(gSinkMutex);

		std::fwrite(line.data(), 1, line.size(), stdout);

		std::fflush(stdout);

		if (gLogFile.is_open())
		{
			gLogFile.write(line.data(), static_cast<std::streamsize>(line.size()));

			// 같은 이유로 파일도 매 줄 flush 한다. 버퍼에만 남겨두면 정작 사고 직전 로그를 못 본다
			// (파일 로그를 두는 이유가 바로 그것이다).
			// 지금은 동기 출력이라 쓰기 비용이 이미 있어 flush 가 지배적이지 않다.
			// 비동기(2단계)로 바꾸면 전용 스레드에서 묶어 쓰고 주기적으로 flush 하면 된다.
			gLogFile.flush();
		}
	}

	bool PassOnce(Domain domain, std::string_view key)
	{
		std::string full(Name(domain));
		full += ':';
		full += key;

		const std::scoped_lock lock(gThrottleMutex);
		auto [it, inserted] = gOnceKeys.try_emplace(std::move(full), 0);
		++it->second;
		return inserted;
	}

	bool PassEvery(
		Domain domain,
		std::string_view key,
		double seconds,
		std::uint64_t& outSuppressed)
	{
		std::string full(Name(domain));
		full += ':';
		full += key;

		const auto now = std::chrono::steady_clock::now();

		const std::scoped_lock lock(gThrottleMutex);
		ThrottleEntry& entry = gEveryKeys[std::move(full)];

		if (!entry.everEmitted)
		{
			entry.everEmitted = true;
			entry.lastEmit = now;
			outSuppressed = 0;
			return true;
		}

		const double elapsed =
			std::chrono::duration<double>(now - entry.lastEmit).count();
		if (elapsed < seconds)
		{
			++entry.suppressed;
			return false;
		}

		entry.lastEmit = now;
		outSuppressed = entry.suppressed;
		entry.suppressed = 0;
		return true;
	}
}
