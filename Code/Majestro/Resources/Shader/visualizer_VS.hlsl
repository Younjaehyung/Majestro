
#include "params.hlsl"
#include "utils.hlsl"

// visualizer_VS.hlsl
// AudioVisualizer 전용 VS — UI_VS와 동일하되 GlobalParams.BaseInstanceID로
// UIInfo 버퍼 내 바 데이터의 시작 오프셋을 더한다.
// (D3D12의 SV_InstanceID는 StartInstanceLocation을 포함하지 않으므로
//  HpBar 셰이더들과 동일하게 루트 상수 dword0(BaseInstanceID)로 오프셋을 전달받는다)

struct VS_IN
{
    float3 pos : POSITION; // (x, y, z) = pixel 좌표
    float2 uv : TEXCOORD;

    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_POSITION; // 클립 공간
    float2 uv : TEXCOORD;

    uint instanceID : InstanceID;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    uint index = GlobalParams.BaseInstanceID + input.instanceID;
    output.instanceID = index;

    UIInstanceData inst = UIInstances[index];

    // 1. Pivot 보정
    float2 local = input.pos.xy - inst.Pivot;

    // 2. 로컬 -> 픽셀
    float2 pixelPos = inst.Position + local * inst.Size;

    // 3. 픽셀 -> NDC
    float2 ndc;
    ndc.x = (pixelPos.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = 1 - (pixelPos.y / PassParams.ScreenSize.y) * 2.0f;

    output.pos = float4(ndc, inst.ZOrder, 1.0f);
    output.uv = input.uv;

    return output;
}
