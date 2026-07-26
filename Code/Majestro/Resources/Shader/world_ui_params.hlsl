#ifndef _WORLD_UI_PARAMS_HLSL_
#define _WORLD_UI_PARAMS_HLSL_


#define MJ_OVERRIDE_GLOBAL_PARAMS
#include "params.hlsl"

struct WORLD_UI_SPRITE_PARAMS
{
    uint BaseInstanceID;
    uint PassFlags;
    uint SpriteRole;
    uint ReservedHeader;

    float3 Anchor;
    float Progress;

    float2 SizePx;
    float2 PivotPx;

    uint BackgroundTextureIndex;
    uint FillTextureIndex;
    uint Reserved0;
    uint Reserved1;
};

struct WORLD_UI_CONQUEST_PARAMS
{
    uint BaseInstanceID;
    uint PassFlags;
    uint InnerRadiusEncoded;
    uint AlphaEncoded;

    float3 Anchor;
    float Progress;

    float2 SizePx;
    float2 PivotPx;

    uint BackgroundTextureIndex;
    uint FillTextureIndex;
    uint Reserved0;
    uint Reserved1;
};

struct WORLD_UI_HP_EFFECT_PARAMS
{
    uint BaseInstanceID;
    uint PassFlags;
    uint ReservedHeader0;
    uint ReservedHeader1;

    float3 Anchor;
    float FollowRatio;

    float2 SizePx;
    float2 PivotPx;

    uint BackgroundTextureIndex;
    uint FillTextureIndex;
    uint HitTextureIndex;
    uint HitConfig;
};

#endif
