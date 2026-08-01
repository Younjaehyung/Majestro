#pragma once

#include <cstdint>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

// ==================== 서버 로그 켜고 끄기 ====================
//
// 아래 매크로의 0 / 1 만 바꾸면 됨
//
//   그룹째 켜기 : MJLOG_ENABLE_VERBOSE 를 1 로  -> 고빈도 도메인이 한꺼번에 켜짐
//   하나만 켜기 : MJLOG_PACKET_FLOW 를 1 로     -> 그룹이 0 이어도 이것만 켜짐
//   하나만 끄기 : MJLOG_AI 를 0 으로            -> 그룹이 1 이어도 이것만 꺼짐

// ==================== 실패했거나 상태가 바뀌었을 때만 ====================
#ifndef MJLOG_ENABLE_DIAGNOSTIC
#define MJLOG_ENABLE_DIAGNOSTIC 1
#endif

// 고빈도: 패킷, 틱마다 출력
#ifndef MJLOG_ENABLE_VERBOSE
#define MJLOG_ENABLE_VERBOSE 0
#endif

// 성능: 구간 측정 결과
#ifndef MJLOG_ENABLE_PERF
#define MJLOG_ENABLE_PERF 0
#endif

// ==================== 진단 도메인 (기본 ON) ====================
// 서버 기동 관련 로그
#ifndef MJLOG_STARTUP
#define MJLOG_STARTUP MJLOG_ENABLE_DIAGNOSTIC
#endif

// 클라이언트 접속 관련 로그
#ifndef MJLOG_SESSION
#define MJLOG_SESSION MJLOG_ENABLE_DIAGNOSTIC
#endif

// 파일 로드 실패 로그
#ifndef MJLOG_RESOURCE_LOAD
#define MJLOG_RESOURCE_LOAD MJLOG_ENABLE_DIAGNOSTIC
#endif

// JSON 설, 표의 로드와 값 로그
#ifndef MJLOG_DATA_TABLE
#define MJLOG_DATA_TABLE MJLOG_ENABLE_DIAGNOSTIC
#endif

// 인게임 관련 로그
#ifndef MJLOG_GAME_RULE
#define MJLOG_GAME_RULE MJLOG_ENABLE_DIAGNOSTIC
#endif

// 적 AI 관련 로그
#ifndef MJLOG_AI
#define MJLOG_AI MJLOG_ENABLE_DIAGNOSTIC
#endif

// ==================== 고빈도 도메인 (기본 OFF) ====================

// 패킷 로그
#ifndef MJLOG_PACKET_FLOW
#define MJLOG_PACKET_FLOW MJLOG_ENABLE_VERBOSE
#endif

// 플레이어 상태 머신
#ifndef MJLOG_PLAYER_STATE
#define MJLOG_PLAYER_STATE MJLOG_ENABLE_VERBOSE
#endif

// 인게임 전투 로그
#ifndef MJLOG_COMBAT_DETAIL
#define MJLOG_COMBAT_DETAIL MJLOG_ENABLE_VERBOSE
#endif

// ==================== 성능 도메인 (기본 OFF) ====================

// 틱, 시스템 구간 성능 소요 시간.
#ifndef MJLOG_PERF
#define MJLOG_PERF MJLOG_ENABLE_PERF
#endif

namespace ServerLog
{
	// 도메인 : 무엇에 대한 로그인가
	enum class Domain
	{
		// 진단 (기본 ON)
		Startup,       // 서버 기동, 설정, 소켓 bind/listen
		Session,       // 세션 관련
		ResourceLoad,  // 파일 로드
		DataTable,     // JSON 설정
		GameRule,      // 인게임 관련
		Ai,            // AI

		// 고빈도 (기본 OFF)
		PacketFlow,    // 패킷 로그
		PlayerState,   // 상태 머신
		CombatDetail,  // 전투 로그 상세

		// 성능 (기본 OFF)
		Perf,          // 구간 소요 시간
	};

	// 심각도 : 얼마나 급한가
	enum class Level
	{
		Info,	// 정상 흐름의 기록
		Warn,	// 이상하지만 계속 돌아감
		Error,	// 기능이 실패함. 파일에 즉시 flush
	};

	inline constexpr bool Enabled(Domain domain)
	{
		switch (domain)
		{
		case Domain::Startup:      return MJLOG_STARTUP != 0;
		case Domain::Session:      return MJLOG_SESSION != 0;
		case Domain::ResourceLoad: return MJLOG_RESOURCE_LOAD != 0;
		case Domain::DataTable:    return MJLOG_DATA_TABLE != 0;
		case Domain::GameRule:     return MJLOG_GAME_RULE != 0;
		case Domain::Ai:           return MJLOG_AI != 0;
		case Domain::PacketFlow:   return MJLOG_PACKET_FLOW != 0;
		case Domain::PlayerState:  return MJLOG_PLAYER_STATE != 0;
		case Domain::CombatDetail: return MJLOG_COMBAT_DETAIL != 0;
		case Domain::Perf:         return MJLOG_PERF != 0;
		default:                   return false;
		}
	}

	inline constexpr const char* Name(Domain domain)
	{
		switch (domain)
		{
		case Domain::Startup:      return "startup";
		case Domain::Session:      return "session";
		case Domain::ResourceLoad: return "resource-load";
		case Domain::DataTable:    return "data-table";
		case Domain::GameRule:     return "game-rule";
		case Domain::Ai:           return "ai";
		case Domain::PacketFlow:   return "packet-flow";
		case Domain::PlayerState:  return "player-state";
		case Domain::CombatDetail: return "combat-detail";
		case Domain::Perf:         return "perf";
		default:                   return "unknown";
		}
	}


	void Initialize();


	void Shutdown();


	void Emit(
		Domain domain,
		Level level,
		const std::source_location& location,
		std::string_view message);

	// 같은 key 로는 한 번만 통과
	bool PassOnce(Domain domain, std::string_view key);


	bool PassEvery(
		Domain domain,
		std::string_view key,
		double seconds,
		std::uint64_t& outSuppressed);


	template<typename... Args>
	inline void Format(
		Domain domain,
		Level level,
		const std::source_location& location,
		std::string_view fmt,
		Args&&... args)
	{
		try
		{
			Emit(domain, level, location,
				std::vformat(fmt, std::make_format_args(args...)));
		}
		catch (const std::format_error& e)
		{
			Emit(domain, Level::Error, location,
				std::string("Format Error: ") + e.what());
		}
	}

	// MJLOG_EVERY 전용
	template<typename... Args>
	inline void FormatSuppressed(
		Domain domain,
		Level level,
		const std::source_location& location,
		std::uint64_t suppressed,
		std::string_view fmt,
		Args&&... args)
	{
		try
		{
			std::string message = std::vformat(fmt, std::make_format_args(args...));
			if (suppressed != 0)
				message += " (억제 " + std::to_string(suppressed) + "회)";
			Emit(domain, level, location, message);
		}
		catch (const std::format_error& e)
		{
			Emit(domain, Level::Error, location,
				std::string("Format Error: ") + e.what());
		}
	}
}

// ==================== 사용법 ====================
//   MJLOG_INFO (Session, "New Client Connected: [{}]", id);
//   MJLOG_ERROR(Startup, "err(bind) code={}", err);
//
//   반복될 수 있는 자리에는 반드시 아래를 쓴다.
//   MJLOG_ONCE (PacketFlow, Warn, key,      "주인 없는 UDP clientId={}", id);  // key 당 1회
//   MJLOG_EVERY(PacketFlow, Warn, key, 5.0, "주인 없는 UDP clientId={}", id);  // key 당 5초에 1회
//
//   MJLOG_EVERY 는 억제된 횟수를 " (억제 N회)" 로 자동으로 덧붙인다.

#define MJLOG(domainName, levelName, fmt, ...)                                     \
	do {                                                                          \
		if constexpr (::ServerLog::Enabled(::ServerLog::Domain::domainName))      \
		{                                                                         \
			::ServerLog::Format(                                                  \
				::ServerLog::Domain::domainName,                                  \
				::ServerLog::Level::levelName,                                    \
				std::source_location::current(),                                  \
				fmt, ##__VA_ARGS__);                                              \
		}                                                                         \
	} while (false)

#define MJLOG_INFO(domainName, fmt, ...)  MJLOG(domainName, Info,  fmt, ##__VA_ARGS__)
#define MJLOG_WARN(domainName, fmt, ...)  MJLOG(domainName, Warn,  fmt, ##__VA_ARGS__)
#define MJLOG_ERROR(domainName, fmt, ...) MJLOG(domainName, Error, fmt, ##__VA_ARGS__)

// 같은 key 는 한 번만. key 는 std::string 으로 만들 수 있는 값이면 된다.
#define MJLOG_ONCE(domainName, levelName, key, fmt, ...)                           \
	do {                                                                          \
		if constexpr (::ServerLog::Enabled(::ServerLog::Domain::domainName))      \
		{                                                                         \
			if (::ServerLog::PassOnce(::ServerLog::Domain::domainName, key))      \
			{                                                                     \
				::ServerLog::Format(                                              \
					::ServerLog::Domain::domainName,                              \
					::ServerLog::Level::levelName,                                \
					std::source_location::current(),                              \
					fmt, ##__VA_ARGS__);                                          \
			}                                                                     \
		}                                                                         \
	} while (false)

// 같은 key 는 seconds 마다 한 번만. 억제된 횟수를 뒤에 붙여준다.
#define MJLOG_EVERY(domainName, levelName, key, seconds, fmt, ...)                 \
	do {                                                                          \
		if constexpr (::ServerLog::Enabled(::ServerLog::Domain::domainName))      \
		{                                                                         \
			std::uint64_t _mjlogSuppressed = 0;                                  \
			if (::ServerLog::PassEvery(                                           \
					::ServerLog::Domain::domainName, key, seconds,                \
					_mjlogSuppressed))                                           \
			{                                                                     \
				::ServerLog::FormatSuppressed(                                    \
					::ServerLog::Domain::domainName,                              \
					::ServerLog::Level::levelName,                                \
					std::source_location::current(),                              \
					_mjlogSuppressed,                                            \
					fmt, ##__VA_ARGS__);                                          \
			}                                                                     \
		}                                                                         \
	} while (false)
