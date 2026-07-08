#pragma once

#include <iostream>
#include <string>

#ifndef MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
#ifdef NDEBUG
#define MAJESTRO_ENABLE_DIAGNOSTIC_LOGS 0
#else
#define MAJESTRO_ENABLE_DIAGNOSTIC_LOGS 1
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
#define MAJESTRO_LOG_GPU_UPLOAD 1
#endif

#ifndef MAJESTRO_LOG_ANIMATION_BUDGET
#define MAJESTRO_LOG_ANIMATION_BUDGET 1
#endif

#ifndef MAJESTRO_LOG_RESOURCE_LOAD
#define MAJESTRO_LOG_RESOURCE_LOAD MAJESTRO_ENABLE_DIAGNOSTIC_LOGS
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
	};

	inline bool Enabled(Domain domain)
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
		default:
			return false;
		}
	}

	inline const char* Name(Domain domain)
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
		default:
			return "unknown";
		}
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
