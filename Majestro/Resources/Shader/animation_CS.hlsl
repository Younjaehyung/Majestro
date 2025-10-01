#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

// ComputeAnimation
// StructuredBuffer<matrix> SkeletonBone : register(t0, space3);
// StructuredBuffer<ANIMFRAMEPARAMS> AnimationClip : register(t1, space3);
// StructuredBuffer<ANIMATIONMETA> AnimationMeta : register(t2, space3);
// StructuredBuffer<Matrix> SFinalBone : register(t4, space1);
// RWStructuredBuffer<Matrix> RFinalBone : register(u0, space1);



//uint Index0 : animationClip;
//uint Index1 : currentFrame;
//uint Index2 : nextFrame;
//uint Index3 : ratioInt;
    


[numthreads(64, 4, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    uint nowbone = threadIdx.x;
    uint rel = threadIdx.y; // 현재 인스턴스 idx   | GlobalParams.BaseInstanceID : 전체 인스턴스 idx
    uint inst = GlobalParams.BaseInstanceID + rel;
    ANIMATIONMETA animationclipmeta = AnimationMeta[AnimInstance[inst].AnimClipIdx];
    
    uint bonecount = animationclipmeta.BoneCount;
    uint frame = animationclipmeta.NumFrame;
    
    uint currentframe = AnimInstance[inst].CurrentFrame;
    uint nextframe = AnimInstance[inst].NextFrame;
    float ratio = AnimInstance[inst].Ratio;
   
    if (nowbone >= bonecount || rel >= GlobalParams.etc)
        return; //etc : 현재 애니메이션을 사용하는 인스턴스 수
        
  
    
    uint idx = nowbone * animationclipmeta.NumFrame + currentframe;
    uint nextidx = nowbone * animationclipmeta.NumFrame + currentframe;

    float4 quaternionzero = float4(0.f, 0.f, 0.f, 1.f);

    
    
    float4 scale = lerp(AnimationClip[idx].Scale, AnimationClip[nextidx].Scale, ratio);
    float4 rotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextidx].Rotation, ratio);
    float4 translation = lerp(AnimationClip[idx].Translation, AnimationClip[nextidx].Translation, ratio);

    matrix matbone = MatrixAffineTransformation(scale, quaternionzero, rotation, translation);

    RFinalBone[AnimInstance[inst].ReulstIndex + nowbone] = mul(SkeletonBone[nowbone], matbone);

}
