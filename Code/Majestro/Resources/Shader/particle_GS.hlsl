#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 viewPos : POSITION;
    float2 uv : TEXCOORD;
    float id : ID;
};

struct GS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint id : ID;
};

[maxvertexcount(6)]
void GS_Main(point VS_OUT input[1], inout TriangleStream<GS_OUT> outputStream)
{
    GS_OUT output[4] =
    {
        (GS_OUT)0.f, (GS_OUT)0.f, (GS_OUT)0.f, (GS_OUT)0.f
    };

    VS_OUT vtx = input[0];
    const uint id = (uint) vtx.id;
    if (Particle[id].alive == 0)
        return;

    const uint emitterIndex = GlobalParams.etc;
    const float ratio = saturate(Particle[id].curTime / max(Particle[id].lifeTime, 0.0001f));
    const float scale = lerp(ParticleShared[emitterIndex].StartScale, ParticleShared[emitterIndex].EndScale, ratio) * 0.5f;

    output[0].position = vtx.viewPos + float4(-scale, scale, 0.f, 0.f);
    output[1].position = vtx.viewPos + float4(scale, scale, 0.f, 0.f);
    output[2].position = vtx.viewPos + float4(scale, -scale, 0.f, 0.f);
    output[3].position = vtx.viewPos + float4(-scale, -scale, 0.f, 0.f);

    output[0].position = mul(output[0].position, PassParams.MatProjection);
    output[1].position = mul(output[1].position, PassParams.MatProjection);
    output[2].position = mul(output[2].position, PassParams.MatProjection);
    output[3].position = mul(output[3].position, PassParams.MatProjection);

    output[0].uv = float2(0.f, 0.f);
    output[1].uv = float2(1.f, 0.f);
    output[2].uv = float2(1.f, 1.f);
    output[3].uv = float2(0.f, 1.f);

    output[0].id = id;
    output[1].id = id;
    output[2].id = id;
    output[3].id = id;

    outputStream.Append(output[0]);
    outputStream.Append(output[1]);
    outputStream.Append(output[2]);
    outputStream.RestartStrip();

    outputStream.Append(output[0]);
    outputStream.Append(output[2]);
    outputStream.Append(output[3]);
    outputStream.RestartStrip();
}
