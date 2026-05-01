#include "pch.h"
#include "PathLoadComponent.h"
#include "GameCore.h"
#include "ResourceManager.h"

PathLoadComponent::PathLoadComponent(const std::wstring& path) : mPath(path)
{
	mPathData = RESOURCEMANAGER.Get<PayloadPathData>(path);

	mTotalDistance = mPathData ? mPathData->GetLength() : 0.f;
}




