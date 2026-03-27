#include "pch.h"
#include "FBXData.h"
#include <fstream>
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Animator.h"
#include "Skeleton.h"
#include "Material.h"

/*
*	[STEP0]
		All Texture Load
	[STEP1]
		FBX Load (model Load)
			-> bin to class Resource
	[STEP1-2]
		If Animation exist Load Animation too
	[STEP2]
		Distribute loaded animation files into each class

*/


string ReadString(std::ifstream& file)
{
	uint32 length;
	file.read(reinterpret_cast<char*>(&length), sizeof(uint32));

	if (length == 0)
		return "";

	string utf8Str(length, '\0');
	file.read(&utf8Str[0], length);
	cout<< utf8Str <<endl;
	return utf8Str; // 기존의 s2ws 함수 사용
}

namespace
{
	inline Matrix ToMatrix(const XMFLOAT4X4& m)
	{
		return Matrix(m);
	}

	inline XMFLOAT4X4 ToFloat4x4(const Matrix& m)
	{
		XMFLOAT4X4 out{};
		XMStoreFloat4x4(&out, m);
		return out;
	}

	inline FBXKeyFrameInfo MakeIdentityKey(double time)
	{
		FBXKeyFrameInfo key{};
		XMStoreFloat4x4(&key.MatTransform, Matrix::Identity);
		key.Time = time;
		return key;
	}

	void ResolveClipToModelSpace(FBXAnimClipInfo& clip, const shared_ptr<Skeleton>& skeleton)
	{
		if (!skeleton)
			return;

		const auto& bones = skeleton->GetBones();
		if (clip.KeyFrameInfo.empty() || bones.empty())
			return;

		const uint32 boneCount = static_cast<uint32>(bones.size());
		clip.KeyFrameInfo.resize(boneCount);

		uint32 maxFrames = 0;
		for (const auto& track : clip.KeyFrameInfo)
			maxFrames = max(maxFrames, static_cast<uint32>(track.size()));

		if (maxFrames == 0)
			maxFrames = 1;

		const double clipDuration = max(0.0001, clip.EndTime - clip.StartTime);
		const double frameStep = clipDuration / static_cast<double>(maxFrames);

		for (uint32 b = 0; b < boneCount; ++b)
		{
			auto& track = clip.KeyFrameInfo[b];
			if (track.empty())
			{
				track.resize(maxFrames);
				for (uint32 f = 0; f < maxFrames; ++f)
					track[f] = MakeIdentityKey(clip.StartTime + frameStep * f);
			}
			else if (track.size() < maxFrames)
			{
				const FBXKeyFrameInfo last = track.back();
				const size_t oldSize = track.size();
				track.resize(maxFrames, last);
				for (uint32 f = static_cast<uint32>(oldSize); f < maxFrames; ++f)
					track[f].Time = clip.StartTime + frameStep * f;
			}
		}

		vector<Matrix> modelPose(boneCount, Matrix::Identity);
		for (uint32 frame = 0; frame < maxFrames; ++frame)
		{
			for (uint32 bone = 0; bone < boneCount; ++bone)
			{
				const Matrix local = ToMatrix(clip.KeyFrameInfo[bone][frame].MatTransform);
				const int32 parent = bones[bone].parentIdx;

				if (parent >= 0 && static_cast<uint32>(parent) < boneCount)
					modelPose[bone] = local * modelPose[parent];
				else
					modelPose[bone] = local;

				clip.KeyFrameInfo[bone][frame].MatTransform = ToFloat4x4(modelPose[bone]);
			}
		}
	}
}


FBXMaterialInfo FBXData::ReadMaterialData(std::ifstream& file)
{

	FBXMaterialInfo m{};
	FBXMaterialValue mv{};
	file.read(reinterpret_cast<char*>(&mv), sizeof(mv));
	m.MaterialValueInfo = mv;

	m.ShaderName = ReadString(file);

	m.DiffuseMap0Name = ReadString(file);
	m.DiffuseMap1Name = ReadString(file);
	m.DiffuseMap2Name = ReadString(file);
	m.DiffuseMap3Name = ReadString(file);

	m.NormalMapName = ReadString(file);
	m.SpecularcMapName = ReadString(file);
	m.EmissiveMapName = ReadString(file);
	m.MetallicMapName = ReadString(file);
	m.OcclusionMapName = ReadString(file);

	return m;
}

//////////////////////////////////////////////////////////////////////////


FBXData::FBXData() : Object(OBJECT_TYPE::FBXDATA)
{
}

FBXData::~FBXData()
{
}

void FBXData::Load(const wstring& path, const wstring& shader)
{
	mPath = ws2s(path);
	std::string filePath{ filesystem::path(mPath).parent_path().string() + "\\" + filesystem::path(mPath).filename().stem().string()};


	if (std::ifstream f(filePath + ".mesh", std::ios::binary);f) {
		f.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));
		CreateMeshFromFBX(f, shader);
	}
	if (std::ifstream f(filePath + ".skel", std::ios::binary); f) {
		CreateSkeletonFromFBX(f);
	}
	if (std::ifstream f(filePath + ".ani", std::ios::binary); f) {
		CreateAnimatorFromFBX(f);
	}
	

}

void FBXData::LoadMeshOnly(const wstring& path)
{
	mPath = ws2s(path);
	std::string filePath{ filesystem::path(mPath).parent_path().string() + "\\" + filesystem::path(mPath).filename().stem().string() };


	if (std::ifstream f(filePath + ".mesh", std::ios::binary); f) {
		f.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));
		CreateMeshFromFBX(f);
	}

}

vector<shared_ptr<Mesh>>& FBXData::CreateMeshFromFBX(ifstream& loader, const wstring& shader)
{
	for (uint8 i = 0; i < mHeader.MeshCount; ++i) {
		shared_ptr<Mesh> mesh = make_shared<Mesh>();
		shared_ptr<CollisionMesh> collisionMesh = make_shared<CollisionMesh>();
		// === 1) .mesh ===

		string meshName = ReadString(loader);
		//if (RESOURCEMANAGER.Get<Mesh>(s2ws(meshName))) {
		//	
		//	mMeshs.push_back(RESOURCEMANAGER.Get<Mesh>(s2ws(meshName)));
		//}

		FBXMeshInfo metaMeshInfo{};
		loader.read(reinterpret_cast<char*>(&metaMeshInfo), sizeof(metaMeshInfo));

		FBXBMeshInfo meshInfo;

		// Mesh Load
		static_assert(std::is_trivially_copyable_v<Vertex>,
			"Vertex must be trivially copyable");
		meshInfo.Vertices.resize(metaMeshInfo.VertexCount);

		if (metaMeshInfo.VertexCount)
			loader.read(reinterpret_cast<char*>(meshInfo.Vertices.data()),
				sizeof(Vertex) * metaMeshInfo.VertexCount);

		// Indices (by material) Load
		meshInfo.Indices.resize(metaMeshInfo.MaterialCount);

		for (uint32 s = 0; s < metaMeshInfo.MaterialCount; ++s)
		{
			uint32 ic = 0;
			loader.read(reinterpret_cast<char*>(&ic), sizeof(ic));
			meshInfo.Indices[s].resize(ic);

			if (ic)
				loader.read(reinterpret_cast<char*>(meshInfo.Indices[s].data()),
					sizeof(uint32) * ic);
		}
		mesh->SetName(s2ws(meshName));
		mesh->CreateMesh(meshInfo);
		mMeshs.push_back(mesh);
		RESOURCEMANAGER.Add<Mesh>(mesh->GetName(), mesh);

		
		/*collisionMesh->SetName(s2ws(meshName));
		collisionMesh->CreateMesh(meshInfo);
		mColliders.push_back(collisionMesh);
		RESOURCEMANAGER.Add<CollisionMesh>(collisionMesh->GetName(), collisionMesh);*/

		CreateMaterialFromFBX(loader, metaMeshInfo, meshInfo, shader);
	}
	loader.close();
	cout << "Materials OVER" << endl;

	return mMeshs;
}

vector<shared_ptr<Material>>& FBXData::CreateMaterialFromFBX(ifstream& loader, FBXMeshInfo& metaInfo, FBXBMeshInfo& meshInfo, const wstring& shader)
{
	// Materials

	meshInfo.Materials.reserve(metaInfo.MaterialCount);
	mMaterials.resize(metaInfo.MaterialCount);
	for (uint32 s = 0; s < metaInfo.MaterialCount; ++s) {
		mMaterials[s] =make_shared<Material>();
		mMaterials[s]->SetName(s2ws(ReadString(loader)));
		meshInfo.Materials.push_back(ReadMaterialData(loader));

		mMaterials[s]->CreateMaterial(meshInfo.Materials[s]);
		mMaterials[s]->SetShader(shader);
		RESOURCEMANAGER.Add<Material>(mMaterials[s]->GetName(), mMaterials[s]);
	}
	
	return mMaterials;
}

shared_ptr<Skeleton> FBXData::CreateSkeletonFromFBX(ifstream& loader)
{
	float isup = 0.f;
	mSkeleton = make_shared<Skeleton>();
	mSkeleton->mBones.reserve(mHeader.BoneCount);
	struct Dummy {
		int32					parentIdx{};
		XMFLOAT4X4				matOffset{};
	};
	string fileName = ReadString(loader);
	for (uint32 bi = 0; bi < mHeader.BoneCount; ++bi)
	{
		BoneInfo	fbxBondInfo;
		Dummy dummy;
		fbxBondInfo.boneName = ReadString(loader);

		loader.read(reinterpret_cast<char*>(&dummy), sizeof(Dummy));
		fbxBondInfo.parentIdx = dummy.parentIdx;
		fbxBondInfo.matOffset = dummy.matOffset; // XMFLOAT4X4 그대로
		fbxBondInfo.blendWeight = ComputeBodyBlendWeight(fbxBondInfo.boneName, isup);
		mSkeleton->mBones.emplace_back(fbxBondInfo);
	}
	loader.close();

	// ── Spine 경계 인덱스 자동 계산 ────────────────────────────────────────
	// 스켈레톤 본 배열 순서: [ Spine 구간 | Upper 구간 | Lower 구간 ]
	//   blendWeight == 0.5  → Spine 전환 구간 (pelvis, spine)
	//   blendWeight  > 0.5  → 완전 상체 (spine1/2/3, arm, head 등)
	//   blendWeight == 0.0  → 완전 하체 (thigh, calf, foot 등)
	//
	// mSpineBoneStartOffset: UpperMask 시작 인덱스
	//   = blendWeight > 0 인 첫 번째 본 (Spine 전환 구간 시작)
	//   스켈레톤 순서가 Spine|Upper|Lower 라면 인덱스 0이 pelvis 이므로 0을 반환한다.
	//   Lower|Spine|Upper 순서라면 하체 다음 첫 번째 비하체 본 인덱스를 반환한다.
	//
	// mSpineBoneEndOffset: UpperMask 끝 인덱스
	//   = blendWeight == 0 이 처음 나타나기 직전 인덱스 (Lower 구간 시작 전)
	//   Lower 본이 없으면 마지막 본까지 사용.
	{
		const auto& bones = mSkeleton->mBones;
		const uint32 boneCount = static_cast<uint32>(bones.size());
		mSkeleton->mBoneCount   = boneCount;
		mSkeleton->mStartOffset = 0;
		mSkeleton->mEndOffset = boneCount-1;
		mSkeleton->mSpineBoneEndOffset = boneCount > 0 ? boneCount - 1 : 0;

		// UpperMask 시작: blendWeight > 0 인 첫 번째 본
		mSkeleton->mSpineBoneStartOffset = 0;
		for (uint32 i = 0; i < boneCount; ++i)
		{
			if (bones[i].boneName == "Bip001 Spine1")
			{
				mSkeleton->mSpineBoneStartOffset = i;
				break;
			}
		}

		// UpperMask 끝: blendWeight > 0 인 마지막 본 (하체 구간 제외)
		// Lower 본들이 마지막에 몰려 있는 경우를 대비해 뒤에서 역방향으로 탐색
		for (int32 i = static_cast<int32>(boneCount) - 1; i >= 0; --i)
		{
			if ((bones[i].boneName == "Bip001 HeadNub") || (bones[i].boneName == "Bip001 Ponytail1Nub"))
			{
				mSkeleton->mSpineBoneEndOffset = static_cast<uint32>(i);
				// break;
			}
		}
	}
	// ───────────────────────────────────────────────────────────────────────

	RESOURCEMANAGER.Add(s2ws(fileName),mSkeleton);

	return mSkeleton;
}

vector<shared_ptr<Animator>>& FBXData::CreateAnimatorFromFBX(ifstream& loader)
{
	mAnimators.resize(mHeader.AnimClipCount);

	for (uint32 ai = 0; ai < mHeader.AnimClipCount; ++ai)
	{

		FBXAnimClipInfo animClipInfo{};

		struct DummyAnimClipInfo
		{
			// string	Name; -> WriteString
			double StartTime;
			double EndTime;
			uint32 TimeMode;                         // FbxTime::EMode를 uint32로
			//	BinaryKeyFrameInfo	KeyFrameInfo;-> vector<Vector>
		};

		DummyAnimClipInfo dummy{};

		animClipInfo.Name = ReadString(loader);
		loader.read(reinterpret_cast<char*>(&dummy), sizeof(DummyAnimClipInfo));
		animClipInfo.StartTime = dummy.StartTime;
		animClipInfo.EndTime = dummy.EndTime;
		animClipInfo.TimeMode = dummy.TimeMode;

		// boneTracks
		uint32 boneTracks = 0;
		loader.read(reinterpret_cast<char*>(&boneTracks), sizeof(uint32));
		animClipInfo.KeyFrameInfo.resize(boneTracks);

		uint32 tempCount = 0;
		uint32 maxTrackCount = 0;
		// 각 본 트랙
		for (uint32 b = 0; b < boneTracks; ++b)
		{
			uint32 kcount = 0;
			loader.read(reinterpret_cast<char*>(&kcount), sizeof(uint32));
			auto& track = animClipInfo.KeyFrameInfo[b];
			track.resize(kcount); // frame

			if (kcount != 0) {
				for (uint32 k = 0; k < kcount; ++k)
				{
					FBXKeyFrameInfo binKF;
					loader.read(reinterpret_cast<char*>(&binKF), sizeof(FBXKeyFrameInfo));

					track[k] = binKF;
				}
				tempCount = kcount;
				maxTrackCount = max(maxTrackCount, kcount);
			}
			else {
				const uint32 fillCount = max(tempCount, 1u);
				track.resize(fillCount);
				for (uint32 k = 0; k < fillCount; ++k)
					track[k] = MakeIdentityKey(animClipInfo.StartTime);
				maxTrackCount = max(maxTrackCount, fillCount);
			}
		}
		if (maxTrackCount == 0)
			maxTrackCount = 1;

		for (auto& track : animClipInfo.KeyFrameInfo)
		{
			if (track.empty())
				track.resize(maxTrackCount, MakeIdentityKey(animClipInfo.StartTime));
			else if (track.size() < maxTrackCount)
				track.resize(maxTrackCount, track.back());
		}
		mAnimators[ai]= std::make_shared<Animator>(animClipInfo);
		mAnimators[ai]->mClipMeta.NumFrame = maxTrackCount;
		mAnimators[ai]->SetSkeleton(mSkeleton);
		RESOURCEMANAGER.Add<Animator>(mAnimators[ai]->GetName(), mAnimators[ai]);
	}
	loader.close();

	return mAnimators;
}

float FBXData::ComputeBodyBlendWeight(const string& boneName, float& isUpper)
{
	string lower = boneName;
	/*std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });*/

	//if (lower.find("thigh") != string::npos || lower.find("calf") != string::npos ||
	//	lower.find("foot") != string::npos || lower.find("toe") != string::npos ||
	//	lower.find("Toe") != string::npos || lower.find("toe") != string::npos ||
	//	lower.find("leg") != string::npos || lower.find("ik_foot") != string::npos)
	//	return 0.f;




	////if (lower.find("spine1") != string::npos)
	////	return 0.5f;
	////if (lower.find("spine2") != string::npos)
	////	return 0.7f;

	////if (lower.find("spine3") != string::npos)
	////	return 0.9f;

	//if (lower.find("pelvis") != string::npos || lower.find("Spine") != string::npos)
	//	return .5f;

	//if (lower.find("chest") != string::npos)
	//	return .9f;

	//if (lower.find("clavicle") != string::npos || lower.find("shoulder") != string::npos)
	//	return .9f;

	//if (lower.find("finger") != string::npos ||
	//	lower.find("neck") != string::npos || lower.find("head") != string::npos ||
	//	lower.find("arm") != string::npos || lower.find("hand") != string::npos)
	//	return 1.0f;

	//if (lower.find("Bip001") != string::npos)
	//	return .5f;

	
	// [버그 수정] 구체적인 조건을 먼저 검사해야 함.
	// 이전 코드는 "Bip001"이 가장 먼저 있어서 모든 Bip001 본이 0.00f를 반환했고
	// 뒤의 Spine/상체 조건들은 절대 도달하지 않는 dead code였음.

		// 하체 본 (먼저 체크)
		if (lower.find("Thigh") != string::npos || lower.find("Calf") != string::npos
			|| lower.find("Foot") != string::npos || lower.find("Toe") != string::npos) return 0.00f;

		if (lower.find("Prop1") != string::npos) return 0.00f;

		// 완전 상체 본
		if (lower.find("Bip001 Neck") != string::npos) return 1.00f;
		if (lower.find("Head") != string::npos || lower.find("HeadNub") != string::npos
			|| lower.find("Ponytail") != string::npos) return 1.00f;
		if (lower.find("Clavicle") != string::npos || lower.find("UpperArm") != string::npos
			|| lower.find("Forearm") != string::npos) return 1.00f;
		if (lower.find("Hand") != string::npos || lower.find("Finger") != string::npos) return 1.00f;

		// Additive 블렌딩이므로 그라데이션을 사용해도 부모 좌표계 오염이 없다.
		// Pelvis에 소량의 delta를 더하면 허리가 자연스럽게 조준 방향으로 기운다.
		if (lower.find("Bip001 Spine3") != string::npos) return 1.00f;
		if (lower.find("Bip001 Spine2") != string::npos) return 0.90f;
		if (lower.find("Bip001 Spine1") != string::npos) return 0.75f;
		if (lower.find("Bip001 Spine")  != string::npos) return 0.40f;
		if (lower.find("Bip001 Pelvis") != string::npos) return 0.15f;

		// 기본 Bip001 루트 (가장 마지막에 체크)
		if (lower.find("Bip001") != string::npos) return 0.00f;

	return 0.5f;
}


////////////////////////////////////////////////////

