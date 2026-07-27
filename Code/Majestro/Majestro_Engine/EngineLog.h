#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#ifndef MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#ifdef NDEBUG
#define MAJESTRO_ENABLE_DIAGNOSTIC_LOGS 0
#else
#define MAJESTRO_ENABLE_DIAGNOSTIC_LOGS 0
#endif
#endif

#ifndef MAJESTRO_LOG_GPU_BUDGET
#define MAJESTRO_LOG_GPU_BUDGET MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_DXGI_BUDGET
#define MAJESTRO_LOG_DXGI_BUDGET MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_GRAPHICS_MEMORY
#define MAJESTRO_LOG_GRAPHICS_MEMORY MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_TEXTURE_BUDGET
#define MAJESTRO_LOG_TEXTURE_BUDGET MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_SCENE_RESOURCE
#define MAJESTRO_LOG_SCENE_RESOURCE MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_LOADING_TASK
#define MAJESTRO_LOG_LOADING_TASK MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_GPU_UPLOAD
#define MAJESTRO_LOG_GPU_UPLOAD 0
#endif

#ifndef MAJESTRO_LOG_ANIMATION_BUDGET
#define MAJESTRO_LOG_ANIMATION_BUDGET 0
#endif

#ifndef MAJESTRO_LOG_RESOURCE_LOAD
#define MAJESTRO_LOG_RESOURCE_LOAD MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_ENABLE_PERFORMANCE_LOGS
#define MAJESTRO_ENABLE_PERFORMANCE_LOGS 0
#endif

#ifndef MAJESTRO_LOG_FRAME_PERFORMANCE
#define MAJESTRO_LOG_FRAME_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

#ifndef MAJESTRO_LOG_SYSTEM_PERFORMANCE
#define MAJESTRO_LOG_SYSTEM_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

#ifndef MAJESTRO_LOG_RENDER_PERFORMANCE
#define MAJESTRO_LOG_RENDER_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

#ifndef MAJESTRO_LOG_RENDER_PASS_PERFORMANCE
#define MAJESTRO_LOG_RENDER_PASS_PERFORMANCE MAJESTRO_ENABLE_PERFORMANCE_LOGS
#endif

#ifndef MAJESTRO_LOG_AUDIO_RUNTIME
#define MAJESTRO_LOG_AUDIO_RUNTIME 0
#endif

#ifndef MAJESTRO_LOG_NETWORK_RUNTIME
#define MAJESTRO_LOG_NETWORK_RUNTIME 0
#endif

#ifndef MAJESTRO_LOG_AUDIO_DIAGNOSTIC
#define MAJESTRO_LOG_AUDIO_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_NETWORK_DIAGNOSTIC
#define MAJESTRO_LOG_NETWORK_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

#ifndef MAJESTRO_LOG_VFX_DIAGNOSTIC
#define MAJESTRO_LOG_VFX_DIAGNOSTIC MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#endif

namespace EngineLog
{
	enum class Domain
	{
		GpuBudget,        // 엔진이 생성한 D3D12 리소스의 그룹별 메모리 사용량 추적
		DxgiBudget,       // DXGI 기준 전용/공유 GPU 메모리 사용량 확인
		GraphicsMemory,   // DirectXTK GraphicsMemory 내부 페이지 사용량 확인
		TextureBudget,    // 파일 텍스처 로드량과 dedup 절감량 확인
		SceneResource,    // 씬 전환 시 prefix 기반 리소스 언로드 결과 확인
		LoadingTask,      // 로딩 큐에서 어떤 FBX 작업이 실행 중인지 확인
		GpuUpload,        // GPU 업로드 버퍼 생성/복사 실패 같은 안전장치 로그
		AnimationBudget,  // 애니메이션 인스턴스/본 버퍼 한계 초과 확인
		ResourceLoad,     // ResourceManager 키 충돌 같은 로드 문제 확인
		FramePerformance,     // 프레임 전체와 주요 엔진 구간의 CPU 시간
		SystemPerformance,    // ECS 시스템별 CPU 시간
		RenderPerformance,    // RenderSystem 내부 구간과 작업량
		RenderPassPerformance,// 렌더 패스별 CPU 명령 기록 시간
		AudioRuntime,         // 박자 정렬과 드리프트 같은 반복 오디오 추적
		NetworkRuntime,       // 패킷과 판정 같은 반복 네트워크 추적
		AudioDiagnostic,      // 오디오 초기화와 재생 실패 진단
		NetworkDiagnostic,    // 잘못된 패킷과 엔티티 연결 실패 진단
		VfxDiagnostic,        // VFX 리소스 누락과 풀 고갈 진단
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
		case Domain::GpuUpload:
			return MAJESTRO_LOG_GPU_UPLOAD != 0;
		case Domain::AnimationBudget:
			return MAJESTRO_LOG_ANIMATION_BUDGET != 0;
		case Domain::ResourceLoad:
			return MAJESTRO_LOG_RESOURCE_LOAD != 0;
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
		case Domain::AudioDiagnostic:
			return MAJESTRO_LOG_AUDIO_DIAGNOSTIC != 0;
		case Domain::NetworkDiagnostic:
			return MAJESTRO_LOG_NETWORK_DIAGNOSTIC != 0;
		case Domain::VfxDiagnostic:
			return MAJESTRO_LOG_VFX_DIAGNOSTIC != 0;
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
		case Domain::GpuUpload:
			return "gpu-upload";
		case Domain::AnimationBudget:
			return "animation-budget";
		case Domain::ResourceLoad:
			return "resource-load";
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
		case Domain::AudioDiagnostic:
			return "audio-diagnostic";
		case Domain::NetworkDiagnostic:
			return "network-diagnostic";
		case Domain::VfxDiagnostic:
			return "vfx-diagnostic";
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
	}

	// 호출부에서 스트림과 줄바꿈 flush를 직접 사용하지 않도록 한 번에 출력한다.
	template<typename... Args>
	inline void Write(Domain domain, Args&&... args)
	{
		if (!Enabled(domain))
			return;

		std::ostringstream stream;
		stream << "[Log : " << Name(domain) << "] ";
		(stream << ... << std::forward<Args>(args));
		stream << '\n';
		const std::string text = stream.str();
		::OutputDebugStringA(text.c_str());
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

	// 동일한 진단 키는 한 번만 출력해 반복 실패가 프레임 시간을 흔들지 않게 한다.
	template<typename... Args>
	inline void WriteOnce(
		Domain domain,
		const std::string& key,
		Args&&... args)
	{
		if (!Enabled(domain) || !Detail::MarkOnce(domain, key))
			return;

		Write(domain, std::forward<Args>(args)...);
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

	// 모든 CPU 계측 지점이 같은 시계와 밀리초 변환을 사용하도록 공통화한다.
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

	// 성능 출력의 활성화 정책과 출력 대상을 EngineLog에서 일관되게 관리한다.
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

	inline std::ostream& Prefix(Domain domain, const char* label, std::ostream& out = std::cout)
	{
		// Unified log prefix: [Log : domain : label].
		out << "[Log : " << Name(domain) << " : " << label << "] ";
		return out;
	}

	inline std::string Narrow(const std::wstring& text)
	{
		return std::string(text.begin(), text.end());
	}
}
