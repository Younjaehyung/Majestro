#pragma once

struct AnimationInstance;
struct KeyFrameInfo;
struct AnimationClipMeta;
struct SkeletonBoneParams;
struct AimParams;

class AnimationEvaluator
{
public:
	// outModelBones[i] : 부모 누적 모델 공간 본 (offset 미적용, VFX/소켓 어태치용)
	// outFinalBones[i] : matOffset.Transpose() * outModelBones[i] (스키닝용 row-major)
	// 두 출력은 모두 inst.BoneCount 만큼의 길이를 미리 확보해야 한다.
	// aim이 null이 아니면 spine/neck 체인에 AimPitch/AimYaw 회전을 가중 분배해서 덧붙인다.
	static void Evaluate(
		const AnimationInstance& inst,
		const KeyFrameInfo* clipBuffer,
		const AnimationClipMeta* metaBuffer,
		uint32 metaCount,
		const SkeletonBoneParams* boneBuffer,
		Matrix* outModelBones,
		Matrix* outFinalBones,
		const AimParams* aim = nullptr);

	static inline float CalculateBlendWeight(uint32 boneIndex, uint32 rangeStart, uint32 rangeEnd, float featherRange);
	static inline void SampleAnimation(
		uint32 boneIndex,
		uint32 frameCount,
		uint32 currentFrame,
		uint32 nextFrame,
		float ratio,
		uint32 animOffset,
		const KeyFrameInfo* clipBuffer,
		Vec4& outScale, Vec4& outRotation, Vec4& outTranslation);


public:

	// 메쉬공간 회전 누적용 스크래치(쿼터니언).
	static inline std::vector<Vec4> baseMeshRot;  // 하체 레이어만으로 누적한 본의 모델공간 회전
	static inline std::vector<Vec4> upperMeshRot;	// 상체 클립으로 누적한 본의 모델공간 회전(마스크 아래는 base와 동일)
	static inline std::vector<Vec4> blendMeshRot;	// 위 둘을 가중 blend 한 최종 모델공간 회전(aim 적용 전)

	// 본별 유효 상체 가중치(부모로부터 전파된 값). 부착 본(무기 등)이 부모를 따라가게
	static inline std::vector<float> effUpperW;
};
