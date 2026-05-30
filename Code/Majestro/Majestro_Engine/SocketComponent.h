#pragma once
#include <string>
#include "Component.h"
#include "Skeleton.h"


struct SocketDef
{
	std::string mSocketName;                    // 소켓 식별 이름
	std::string mBoneName;                      // 부착 대상 본 이름 (비우면 엔티티 직접 부착)
	Matrix      mLocalOffset = Matrix::Identity; // 본 로컬 공간 기준 오프셋 (row-major, v*M)

	//런타임 캐시
	uint32 mCachedBoneIndex = Skeleton::INVALID_BONE_INDEX; // 본 인덱스 캐시(1회 해석)
	Matrix mWorldMatrix     = Matrix::Identity;             // 갱신된 소켓 월드행렬
	bool   mValid           = false;                        // 이번 프레임 갱신 성공 여부
};

class SocketComponent : public Component<SocketComponent>
{
public:
	// 소켓 이름으로 월드행렬 조회.
	bool TryGetSocketWorldMatrix(const std::string& socketName, Matrix& outMatrix) const
	{
		for (const SocketDef& socket : mSockets)
		{
			if (socket.mSocketName == socketName)
			{
				if (socket.mValid == false)
					return false;
				outMatrix = socket.mWorldMatrix;
				return true;
			}
		}
		return false;
	}

	// 소켓 정의 조회 및 수정
	SocketDef* FindSocket(const std::string& socketName)
	{
		for (SocketDef& socket : mSockets)
			if (socket.mSocketName == socketName)
				return &socket;
		return nullptr;
	}

public:
	std::vector<SocketDef> mSockets;
};
