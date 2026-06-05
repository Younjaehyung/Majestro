#pragma once

enum class TrailRenderStyle : uint8
{
	FlameRibbon = 0,
	SwordSlash = 1,
	SpeedLine = 2,
	HammerFlame = 3,
	Smoke = 4,
};

struct TrailRenderSample
{
	Vec3 LeftWorld{};
	Vec3 RightWorld{};
	float Age = 0.0f;
};

struct TrailRenderDesc
{
	std::vector<TrailRenderSample> Samples;
	std::wstring TextureName;

	TrailRenderStyle Style = TrailRenderStyle::FlameRibbon;
	uint32 LayerCount = 1;
	uint32 SmoothingSubdivisions = 0;

	float Lifetime = 0.2f;
	float BaseAlpha = 1.0f;
	float Intensity = 1.0f;
	float UvTiling = 1.0f;

	float WidthMultiplier = 1.0f;
	float TailWidthScale = 1.0f;
	float MidWidthScale = 1.0f;
	float HeadWidthScale = 1.0f;
	float LayerSpread = 0.0f;

	float CutStrength = 0.0f;
	float LineStrength = 0.0f;
	// SwordSlash 전용
	// 외곽 EdgeColor 발광 배수 / 중심 CoreColor 틴트 배수.
	// 다른 스타일은 1.0 기본값이라 영향을 주지 않음
	float EdgeBoost = 1.0f;
	float CoreBoost = 1.0f;
	
	// uv.y(길이) 방향 텍스처/결 시간 스크롤 속도. 0이면 정지.
	float TexScrollSpeed = 0.0f;

	Vec3 CoreColor = Vec3(1.0f, 1.0f, 1.0f);
	Vec3 EdgeColor = Vec3(1.0f, 1.0f, 1.0f);
	Vec3 SubColor = Vec3(0.0f, 0.0f, 0.0f);
};
