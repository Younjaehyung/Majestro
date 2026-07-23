#pragma once
#include "Component.h"
#include "Shader.h"

class Mesh;
class Material;
class TransformComponent;

enum class StaticCasterChangeType : uint8
{
	None = 0,
	Content,
	Bounds
};

union InstanceID
{
	struct
	{
		uint32 MeshID;
		uint32 MaterialID;
	};
	uint64 ID;
};


class RenderComponent : public Component<RenderComponent>
{
public:
	RenderComponent();
	RenderComponent(shared_ptr<Mesh> mesh, vector<shared_ptr<Material>>& materials);
	uint8 GetLayerIndex() { return mLayerIndex; }
	bool IsVisibility() { return mVisibility; }
	uint64 GetInstanceID();
	void SetMesh(shared_ptr<Mesh> mesh);
	void SetLocalOBB(const Vec3& center, const Vec3& halfExtents);
	bool UpdateWorldOBB(TransformComponent* transformComponent);
	StaticCasterChangeType UpdateStaticCasterState(bool isStatic);
public:
	bool mCheckFrustum = true;
	bool mCheckVisibilty = true;
	bool mIsNotObject = false;

	//자기 layer인덱스 확인
	uint8 mLayerIndex = 0;

	shared_ptr<Mesh> mMesh;
	vector<shared_ptr<Material>> mMaterials;

	uint32	mObjectIndex{};
	bool mVisibility{true};

	float mOpacity{ 1.f };

	// 디졸브 소멸 진행도 (0=정상, 1=완전 소멸)
	float mDissolve{ 0.f };

	Vec3 mObbCenter = Vec3(0.f, 0.f, 0.f);
	Vec3 mObbHalfExtents = Vec3(0.5f, 0.5f, 0.5f);

	BoundingOrientedBox mLocalOBB{};
	BoundingOrientedBox mWorldOBB{};

private:
	// 월드 행렬이 같은 정적 오브젝트의 OBB 8코너 변환을 매 프레임 반복하지 않도록 캐시한다.
	Matrix mCachedObbWorldMatrix{};
	bool mWorldObbInitialized = false;
	const Mesh* mCachedStaticCasterMesh = nullptr;
	bool mCachedStaticCasterVisibility = false;
	bool mCachedStaticCasterFlag = false;
	bool mStaticCasterStateInitialized = false;
};

