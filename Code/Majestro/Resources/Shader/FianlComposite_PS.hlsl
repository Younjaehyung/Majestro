
#include "params.hlsl"
#include "utils.hlsl"


struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


float4 PS_Main(VS_OUT input) : SV_Target0
{
   // return Gbuffer[10].Sample(g_sam_0, input.uv);
    PASS_CUSTOM_DATA index = PassCustomTable[5];
    Texture2D t = Gbuffer[index.PreviousStep];

  
    if (index.ExtValue[0][0]>0.5f)
    {
        float4 c0;
        float4 c1;
        float4 c2;
        float4 c3;
        float4 c4;

        float offsetX = 0.005f;
    
        c0 = t.Sample(g_sam_0, float2(input.uv.x, input.uv.y - offsetX * 1));
        c1 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 1, input.uv.y));
        c2 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 0, input.uv.y));
        c3 = t.Sample(g_sam_0, float2(input.uv.x + offsetX * 1, input.uv.y));
        c4 = t.Sample(g_sam_0, float2(input.uv.x, input.uv.y + offsetX * 1));
        float4 sum = c0 + c1 + c2 + c3 + c4;

        
        c0 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 1, input.uv.y - offsetX * 1));
        c1 = t.Sample(g_sam_0, float2(input.uv.x + offsetX * 1, input.uv.y + offsetX * 1));
        c2 = t.Sample(g_sam_0, float2(input.uv.x + offsetX * 1, input.uv.y - offsetX * 1));
        c3 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 1, input.uv.y + offsetX * 1));

        sum += c0 + c1 + c2 + c3;

        

        
        sum /= 9.0f;

        return sum;
        
        //float4 c0;
        //float4 c1;
        //float4 c2;
        //float4 c3;
        //float4 c4;

        //float offsetX = 0.01f;
    
        //c0 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 2, input.uv.y));
        //c1 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 1, input.uv.y));
        //c2 = t.Sample(g_sam_0, float2(input.uv.x - offsetX * 0, input.uv.y));
        //c3 = t.Sample(g_sam_0, float2(input.uv.x + offsetX * 1, input.uv.y));
        //c4 = t.Sample(g_sam_0, float2(input.uv.x + offsetX * 2, input.uv.y));

        //float4 sum = c0 + c1 + c2 + c3 + c4;
        //sum /= 5.0f;
        
        //c0 = t.Sample(g_sam_0, float2(input.uv.x , input.uv.y - offsetX * 2));
        //c1 = t.Sample(g_sam_0, float2(input.uv.x , input.uv.y- offsetX * 1));
        //c2 = t.Sample(g_sam_0, float2(input.uv.x , input.uv.y- offsetX * 0));
        //c3 = t.Sample(g_sam_0, float2(input.uv.x , input.uv.y+ offsetX * 1));
        //c4 = t.Sample(g_sam_0, float2(input.uv.x , input.uv.y+ offsetX * 2));
        
        //float4 sum2 = c0 + c1 + c2 + c3 + c4;
        //sum2 /= 5.0f;
        
        //float4 finalColor = (sum + sum2) / 2.0f;
        
        //return finalColor;
    }

    return Gbuffer[index.PreviousStep].Sample(g_sam_0, input.uv);

}
