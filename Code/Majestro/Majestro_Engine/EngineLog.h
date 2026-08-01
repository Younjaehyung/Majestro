#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

// ══ 로그 켜고 끄기 ═════════════════════════════════════════════════════════
//
// 여기가 스위치판이다. 아래 매크로의 0 / 1 만 바꾸면 된다. (바꾸면 엔진 전체 리빌드)
//
//   그룹째 켜기   : MAJESTRO_ENABLE_PERFORMANCE_LOGS 를 1 로  -> 성능 4개가 한꺼번에 켜짐
//   하나만 켜기   : MAJESTRO_LOG_GRAPHICS_MEMORY 를 1 로      -> 그룹이 0 이어도 이것만 켜짐
//   하나만 끄기   : MAJESTRO_LOG_DATA_TABLE 을 0 으로         -> 그룹이 1 이어도 이것만 꺼짐
//
// 개별(MAJESTRO_LOG_*)이 그룹(MAJESTRO_ENABLE_*)보다 항상 우선시 됨.


// ===================== 그룹 스위치 =====================

// 실패 로그
#ifndef MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#define MAJESTRO_ENABLE_DIAGNOSTIC_LOGS 1
#endif

// 메모리 예산 출력
#ifndef MAJESTRO_ENABLE_BUDGET_LOGS
#define MAJESTRO_ENABLE_BUDGET_LOGS 0
#endif

// 60프레임마다 1번씩 찍히는 성능 로그
#ifndef MAJESTRO_ENABLE_PERFORMANCE_LOGS
#define MAJESTRO_ENABLE_PERFORMANCE_LOGS 0
#endif

// 매 프레임, 매 이벤트마다 출력 (인게임)
#ifndef MAJESTRO_ENABLE_RUNTIME_LOGS
#define MAJESTRO_ENABLE_RUNTIME_LOGS 0
#endif

// 매 프레임, 매 이벤트마다 출력 (로드/초기화)
#ifndef MAJESTRO_ENABLE_VERBOSE_LOGS
#define MAJESTRO_ENABLE_VERBOSE_LOGS 0
#endif

// ===================== 예산 계측 도메인 (기본 OFF) =====================

// D3D12 리소스 생성 시점에 잡히는 GPU 메모리 예산
#ifndef MAJESTRO_LOG_GPU_BUDGET
#define MAJESTRO_LOG_GPU_BUDGET MAJESTRO_ENABLE_BUDGET_LOGS
#endif

// 전체 VRAM 크기, 사용 중인 VRAM, 예약된 VRAM, 예약 가능한 VRAM
#ifndef MAJESTRO_LOG_DXGI_BUDGET
#define MAJESTRO_LOG_DXGI_BUDGET MAJESTRO_ENABLE_BUDGET_LOGS
#endif

// DirectXTK 가 내부에 잡아두는 임시 페이지 양.
#ifndef MAJESTRO_LOG_GRAPHICS_MEMORY
#define MAJESTRO_LOG_GRAPHICS_MEMORY MAJESTRO_ENABLE_BUDGET_LOGS
#endif

// 텍스쳐 관련 로그
#ifndef MAJESTRO_LOG_TEXTURE_BUDGET
#define MAJESTRO_LOG_TEXTURE_BUDGET MAJESTRO_ENABLE_BUDGET_LOGS
#endif

// ===================== 진단 도메인 (기본 ON) =====================

// 파일을 로드 실패
#ifndef MAJESTRO_LOG_RESOURCE_LOAD
#define MAJESTRO_LOG_RESOURCE_LOAD MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// 씬 관련 로드 문제
#ifndef MAJESTRO_LOG_SCENE_DIAGNOSTIC
#define MAJESTRO_LOG_SCENE_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// JSON 로드 관련 문제
#ifndef MAJESTRO_LOG_DATA_TABLE
#define MAJESTRO_LOG_DATA_TABLE MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// SFX 관련 문제
#ifndef MAJESTRO_LOG_AUDIO_DIAGNOSTIC
#define MAJESTRO_LOG_AUDIO_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// 네트워크 문제
#ifndef MAJESTRO_LOG_NETWORK_DIAGNOSTIC
#define MAJESTRO_LOG_NETWORK_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// VFX 관련 문제
#ifndef MAJESTRO_LOG_VFX_DIAGNOSTIC
#define MAJESTRO_LOG_VFX_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// CPU->GPU 업로드 문제
#ifndef MAJESTRO_LOG_GPU_UPLOAD
#define MAJESTRO_LOG_GPU_UPLOAD MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// 애니메이션 버퍼 한도 초과
#ifndef MAJESTRO_LOG_ANIMATION_BUDGET
#define MAJESTRO_LOG_ANIMATION_BUDGET MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// 씬을 떠날 때 이전 씬 리소스를 얼마나 정리했는지.
#ifndef MAJESTRO_LOG_SCENE_RESOURCE
#define MAJESTRO_LOG_SCENE_RESOURCE MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// 로딩 화면에서 로딩중인 FBX 작업
#ifndef MAJESTRO_LOG_LOADING_TASK
#define MAJESTRO_LOG_LOADING_TASK MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// Jolt 물리가 내보내는 경고
#ifndef MAJESTRO_LOG_PHYSICS
#define MAJESTRO_LOG_PHYSICS MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

// ===================== 성능 도메인 (기본 OFF) =====================
// FRAME 으로 먼저 재고
// 로직이 느리면 SYSTEM, 렌더가 느리면 RENDER -> RENDER_PASS

// 한 프레임이 몇 ms 인지와 각 파트별 시간 (대기/로직/렌더/제출)
#ifndef MAJESTRO_LOG_FRAME_PERFORMANCE
#define MAJESTRO_LOG_FRAME_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

// ECS 시스템 별 상위 8개 시간
#ifndef MAJESTRO_LOG_SYSTEM_PERFORMANCE
#define MAJESTRO_LOG_SYSTEM_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

// RenderSystem 내부 구간과 처리량
#ifndef MAJESTRO_LOG_RENDER_PERFORMANCE
#define MAJESTRO_LOG_RENDER_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

// RENDER_PASS 하나하나가 명령 기록에 쓴 시간
#ifndef MAJESTRO_LOG_RENDER_PASS_PERFORMANCE
#define MAJESTRO_LOG_RENDER_PASS_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

// ===================== 매 프레임, 매 이벤트마다 출력 (인게임) (기본 OFF) =====================

// Rythm이 안맞아 드리프트 관련 로그
#ifndef MAJESTRO_LOG_AUDIO_RUNTIME
#define MAJESTRO_LOG_AUDIO_RUNTIME MAJESTRO_ENABLE_RUNTIME_LOGS
#endif

// 패킷 및 판정 관련 로그
#ifndef MAJESTRO_LOG_NETWORK_RUNTIME
#define MAJESTRO_LOG_NETWORK_RUNTIME MAJESTRO_ENABLE_RUNTIME_LOGS
#endif

// 상태 진입/이탈 관련 로그
#ifndef MAJESTRO_LOG_PLAYER_STATE_RUNTIME
#define MAJESTRO_LOG_PLAYER_STATE_RUNTIME MAJESTRO_ENABLE_RUNTIME_LOGS
#endif

// 플레이어 입력 처리 결과
#ifndef MAJESTRO_LOG_PLAYER_INPUT_RUNTIME
#define MAJESTRO_LOG_PLAYER_INPUT_RUNTIME MAJESTRO_ENABLE_RUNTIME_LOGS
#endif

// ===================== 매 프레임, 매 이벤트마다 출력 (로드/초기화) (기본 OFF) =====================
// 
// 스켈레톤 본 목록과 조준 본 인덱스
#ifndef MAJESTRO_LOG_SKELETON_BONES
#define MAJESTRO_LOG_SKELETON_BONES MAJESTRO_ENABLE_VERBOSE_LOGS
#endif

// 상태별 애니메이션 종료 시각 표
#ifndef MAJESTRO_LOG_STATE_TIMING
#define MAJESTRO_LOG_STATE_TIMING MAJESTRO_ENABLE_VERBOSE_LOGS
#endif





namespace EngineLog
{
	enum class Domain
	{
		// 예산 계측 (기본 OFF)
		GpuBudget,        // 우리가 만든 D3D12 리소스별 MB (켜두면 목록이 계속 쌓임)
		DxgiBudget,       // 그래픽 카드 전체 기준 사용량/한도
		GraphicsMemory,   // DirectXTK 내부 임시 페이지 양
		TextureBudget,    // 텍스처 로드량과 재사용 절감량

		// 진단 (기본 ON)
		ResourceLoad,     // 파일을 못 읽음
		SceneDiagnostic,  // 파일은 읽혔는데 레벨에 못 얹음
		DataTable,        // JSON 설정·표의 값이 이상함
		AudioDiagnostic,  // 소리가 안 남
		NetworkDiagnostic,// 서버와 안 맞음
		VfxDiagnostic,    // 이펙트가 안 보임
		GpuUpload,        // CPU->GPU 업로드 실패
		AnimationBudget,  // 캐릭터/뼈가 버퍼 한도 초과 (그 프레임 애니메이션 스킵)
		SceneResource,    // 씬 떠날 때 리소스 정리 결과
		LoadingTask,      // 로딩 중인 FBX 작업
		Physics,          // Jolt 물리 경고·단언 실패

		// 성능 (기본 OFF)
		FramePerformance,     // 프레임 전체와 대기/로직/렌더/제출 분해
		SystemPerformance,    // ECS 시스템별 시간 상위 8개
		RenderPerformance,    // RenderSystem 내부 구간과 처리량
		RenderPassPerformance,// 렌더 패스별 명령 기록 시간

		// 매 프레임,매 이벤트 (인게임) (기본 OFF)
		AudioRuntime,         // 박자 정렬과 드리프트
		NetworkRuntime,       // 패킷과 판정
		PlayerStateRuntime,   // 플레이어 상태 진입/이탈
		PlayerInputRuntime,   // 입력 처리 결과(리듬 전환 등)

		//매 프레임,매 이벤트 (로드/초기화) (기본 OFF)
		SkeletonBones,        // 스켈레톤 본 목록
		StateTiming,          // 상태별 애니메이션 종료 시각 표
	};

	inline constexpr bool Enabled(Domain domain)
	{
		switch (domain)
		{
		case Domain::GpuBudget:
			return MAJESTRO_LOG_GPU_BUDGET != 0;
		case Domain::DxgiBudget:
			return MAJESTRO_LOG_DXGI_BUDGET != 0;
		case Domain::GraphicsMemory:
			return MAJESTRO_LOG_GRAPHICS_MEMORY != 0;
		case Domain::TextureBudget:
			return MAJESTRO_LOG_TEXTURE_BUDGET != 0;
		case Domain::SceneResource:
			return MAJESTRO_LOG_SCENE_RESOURCE != 0;
		case Domain::LoadingTask:
			return MAJESTRO_LOG_LOADING_TASK != 0;
		case Domain::Physics:
			return MAJESTRO_LOG_PHYSICS != 0;
		case Domain::GpuUpload:
			return MAJESTRO_LOG_GPU_UPLOAD != 0;
		case Domain::AnimationBudget:
			return MAJESTRO_LOG_ANIMATION_BUDGET != 0;
		case Domain::ResourceLoad:
			return MAJESTRO_LOG_RESOURCE_LOAD != 0;
		case Domain::SceneDiagnostic:
			return MAJESTRO_LOG_SCENE_DIAGNOSTIC != 0;
		case Domain::DataTable:
			return MAJESTRO_LOG_DATA_TABLE != 0;
		case Domain::AudioDiagnostic:
			return MAJESTRO_LOG_AUDIO_DIAGNOSTIC != 0;
		case Domain::NetworkDiagnostic:
			return MAJESTRO_LOG_NETWORK_DIAGNOSTIC != 0;
		case Domain::VfxDiagnostic:
			return MAJESTRO_LOG_VFX_DIAGNOSTIC != 0;
		case Domain::FramePerformance:
			return MAJESTRO_LOG_FRAME_PERFORMANCE != 0;
		case Domain::SystemPerformance:
			return MAJESTRO_LOG_SYSTEM_PERFORMANCE != 0;
		case Domain::RenderPerformance:
			return MAJESTRO_LOG_RENDER_PERFORMANCE != 0;
		case Domain::RenderPassPerformance:
			return MAJESTRO_LOG_RENDER_PASS_PERFORMANCE != 0;
		case Domain::AudioRuntime:
			return MAJESTRO_LOG_AUDIO_RUNTIME != 0;
		case Domain::NetworkRuntime:
			return MAJESTRO_LOG_NETWORK_RUNTIME != 0;
		case Domain::PlayerStateRuntime:
			return MAJESTRO_LOG_PLAYER_STATE_RUNTIME != 0;
		case Domain::PlayerInputRuntime:
			return MAJESTRO_LOG_PLAYER_INPUT_RUNTIME != 0;
		case Domain::SkeletonBones:
			return MAJESTRO_LOG_SKELETON_BONES != 0;
		case Domain::StateTiming:
			return MAJESTRO_LOG_STATE_TIMING != 0;
		default:
			return false;
		}
	}

	inline constexpr const char* Name(Domain domain)
	{
		switch (domain)
		{
		case Domain::GpuBudget:
			return "gpu-budget";
		case Domain::DxgiBudget:
			return "dxgi-budget";
		case Domain::GraphicsMemory:
			return "graphics-memory";
		case Domain::TextureBudget:
			return "texture-budget";
		case Domain::SceneResource:
			return "scene-resource";
		case Domain::LoadingTask:
			return "loading-task";
		case Domain::Physics:
			return "physics";
		case Domain::GpuUpload:
			return "gpu-upload";
		case Domain::AnimationBudget:
			return "animation-budget";
		case Domain::ResourceLoad:
			return "resource-load";
		case Domain::SceneDiagnostic:
			return "scene-diagnostic";
		case Domain::DataTable:
			return "data-table";
		case Domain::AudioDiagnostic:
			return "audio-diagnostic";
		case Domain::NetworkDiagnostic:
			return "network-diagnostic";
		case Domain::VfxDiagnostic:
			return "vfx-diagnostic";
		case Domain::FramePerformance:
			return "frame-performance";
		case Domain::SystemPerformance:
			return "system-performance";
		case Domain::RenderPerformance:
			return "render-performance";
		case Domain::RenderPassPerformance:
			return "render-pass-performance";
		case Domain::AudioRuntime:
			return "audio-runtime";
		case Domain::NetworkRuntime:
			return "network-runtime";
		case Domain::PlayerStateRuntime:
			return "player-state-runtime";
		case Domain::PlayerInputRuntime:
			return "player-input-runtime";
		case Domain::SkeletonBones:
			return "skeleton-bones";
		case Domain::StateTiming:
			return "state-timing";
		default:
			return "unknown";
		}
	}

	namespace Detail
	{
		inline bool MarkOnce(Domain domain, const std::string& key)
		{
			static std::mutex mutex;
			static std::unordered_set<std::string> keys;
			const std::string domainKey =
				std::string(Name(domain)) + ":" + key;
			const std::scoped_lock lock(mutex);
			return keys.insert(domainKey).second;
		}

		inline bool MarkOnce(Domain domain, const std::wstring& key)
		{
			static std::mutex mutex;
			static std::unordered_set<std::wstring> keys;
			std::wstring domainKey;
			for (const char* name = Name(domain); *name != '\0'; ++name)
				domainKey.push_back(static_cast<wchar_t>(*name));
			domainKey += L":";
			domainKey += key;
			const std::scoped_lock lock(mutex);
			return keys.insert(domainKey).second;
		}

		// 태그가 없으면 "[Log : domain] ", 있으면 "[Log : domain : tag] " 로 맞춘다.
		inline void WritePrefix(std::ostringstream& stream, Domain domain, const char* tag)
		{
			stream << "[Log : " << Name(domain);
			if (tag != nullptr && *tag != '\0')
				stream << " : " << tag;
			stream << "] ";
		}

		template<typename... Args>
		inline void Emit(Domain domain, const char* tag, Args&&... args)
		{
			std::ostringstream stream;
			WritePrefix(stream, domain, tag);
			(stream << ... << std::forward<Args>(args));
			stream << '\n';
			::OutputDebugStringA(stream.str().c_str());
		}
	}


	template<typename... Args>
	inline void Write(Domain domain, Args&&... args)
	{
		if (!Enabled(domain))
			return;

		Detail::Emit(domain, nullptr, std::forward<Args>(args)...);
	}


	template<typename... Args>
	inline void WriteTagged(Domain domain, const char* tag, Args&&... args)
	{
		if (!Enabled(domain))
			return;

		Detail::Emit(domain, tag, std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline void WriteWide(Domain domain, Args&&... args)
	{
		if (!Enabled(domain))
			return;

		std::wostringstream stream;
		stream << L"[Log : ";
		for (const char* name = Name(domain); *name != '\0'; ++name)
			stream << static_cast<wchar_t>(*name);
		stream << L"] ";
		(stream << ... << std::forward<Args>(args));
		stream << L'\n';
		const std::wstring text = stream.str();
		::OutputDebugStringW(text.c_str());
	}


	template<typename... Args>
	inline void WriteOnce(
		Domain domain,
		const std::string& key,
		Args&&... args)
	{
		if (!Enabled(domain) || !Detail::MarkOnce(domain, key))
			return;

		Detail::Emit(domain, nullptr, std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline void WriteTaggedOnce(
		Domain domain,
		const char* tag,
		const std::string& key,
		Args&&... args)
	{
		if (!Enabled(domain) || !Detail::MarkOnce(domain, key))
			return;

		Detail::Emit(domain, tag, std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline void WriteWideOnce(
		Domain domain,
		const std::wstring& key,
		Args&&... args)
	{
		if (!Enabled(domain) || !Detail::MarkOnce(domain, key))
			return;

		WriteWide(domain, std::forward<Args>(args)...);
	}

	using CpuClock = std::chrono::steady_clock;
	using CpuTimePoint = CpuClock::time_point;


	inline CpuTimePoint CpuNow()
	{
		return CpuClock::now();
	}

	inline double CpuElapsedMilliseconds(
		const CpuTimePoint& begin,
		const CpuTimePoint& end)
	{
		return std::chrono::duration<double, std::milli>(end - begin).count();
	}

	struct CpuProfileSample
	{
		double totalMs = 0.0;
		double maxMs = 0.0;

		void Add(double elapsedMs)
		{
			totalMs += elapsedMs;
			if (elapsedMs > maxMs)
				maxMs = elapsedMs;
		}

		double Average(std::uint32_t sampleCount) const
		{
			if (sampleCount == 0)
				return 0.0;

			return totalMs / static_cast<double>(sampleCount);
		}

		void Reset()
		{
			totalMs = 0.0;
			maxMs = 0.0;
		}
	};

	class ScopedCpuProfile
	{
	public:
		ScopedCpuProfile(Domain domain, CpuProfileSample& sample)
			: mSample(Enabled(domain) ? &sample : nullptr)
		{
			if (mSample)
				mBegin = CpuNow();
		}

		~ScopedCpuProfile()
		{
			Stop();
		}

		ScopedCpuProfile(const ScopedCpuProfile&) = delete;
		ScopedCpuProfile& operator=(const ScopedCpuProfile&) = delete;

		double Stop()
		{
			if (mSample == nullptr)
				return 0.0;

			const double elapsedMs =
				CpuElapsedMilliseconds(mBegin, CpuNow());
			mSample->Add(elapsedMs);
			mSample = nullptr;
			return elapsedMs;
		}

	private:
		CpuProfileSample* mSample = nullptr;
		CpuTimePoint mBegin{};
	};


	inline void OutputPerformance(Domain domain, const char* text)
	{
		if (Enabled(domain) && text)
			::OutputDebugStringA(text);
	}

	inline void OutputPerformance(Domain domain, const wchar_t* text)
	{
		if (Enabled(domain) && text)
			::OutputDebugStringW(text);
	}

	inline std::string Narrow(const std::wstring& text)
	{
		if (text.empty())
			return std::string();

		const int length = ::WideCharToMultiByte(
			CP_ACP, 0, text.c_str(), static_cast<int>(text.size()),
			nullptr, 0, nullptr, nullptr);
		if (length <= 0)
			return std::string();

		std::string result(static_cast<size_t>(length), '\0');
		::WideCharToMultiByte(
			CP_ACP, 0, text.c_str(), static_cast<int>(text.size()),
			result.data(), length, nullptr, nullptr);
		return result;
	}
}
