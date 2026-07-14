#ifndef _WORLD_UI_PARAMS_HLSL_
#define _WORLD_UI_PARAMS_HLSL_

// WorldUIPass(HP 바 / 점령 링) 전용 b0/space0 오버라이드 레이아웃.

#define MJ_OVERRIDE_GLOBAL_PARAMS
#include "params.hlsl"

struct WORLD_UI_PARAMS
{
    MJ_GLOBAL_PARAMS_HEADER      // BaseInstanceID / PassScalar0 / PassScalar1 / PassCustomIndex

    float3 HpBarAnchorWorld;     // 월드 공간 앵커 위치 (HUD 모드는 xy=화면 픽셀)
    float  HpBarFollowRatio;     // 현재 비율 (0~1) — fill 크롭용 (r1.w 빈 lane 활용)

    float2 HpBarSizePx;          // 화면 픽셀 단위 바 크기
    float2 HpBarPivotPx;         // 앵커 기준 좌상단 픽셀 오프셋 (보통 -size/2)

    uint   HpBarBgTexIdx;        // TextureMaps 배열 인덱스 (배경)
    uint   HpBarFillTexIdx;      // TextureMaps 배열 인덱스 (채움)
    uint   HpBarHitTexIdx;       // TextureMaps 배열 인덱스 (hit effect 시트, 0이면 비활성)
    uint   HpBarHitConfig;       // packed: cols (bits 0-7) | rows (bits 8-15) | frameCount (bits 16-31)
};
ConstantBuffer<WORLD_UI_PARAMS> GlobalParams : register(b0, space0);

#endif
