#include "pch.h"
#include "AIManager.h"

void AIManager::Initialize()
{
    LoadDefaultModels();
}

bool AIManager::LoadModel(const std::wstring& modelKey, const std::wstring& modelPath, std::wstring* errorMessage)
{
    OnnxModelRunner runner;
    if (!runner.Load(modelPath, errorMessage))
        return false;

    mModels[modelKey] = std::move(runner);
    return true;
}

bool AIManager::HasModel(const std::wstring& modelKey) const
{
    return mModels.find(modelKey) != mModels.end();
}

bool AIManager::RunModel(
    const std::wstring& modelKey,
    const InputArray& input,
    OutputArray& output,
    std::wstring* errorMessage) const
{
    const auto findIt = mModels.find(modelKey);
    if (findIt == mModels.end())
    {
        if (errorMessage)
            *errorMessage = L"Requested model key is not registered: " + modelKey;
        return false;
    }

    return findIt->second.Run(input, output, errorMessage);
}

bool AIManager::RunModelMulti(
    const std::wstring& modelKey,
    const DynamicInput& input,
    DynamicOutputs& outputs,
    std::wstring* errorMessage) const
{
    const auto findIt = mModels.find(modelKey);
    if (findIt == mModels.end())
    {
        if (errorMessage)
            *errorMessage = L"Requested model key is not registered: " + modelKey;
        return false;
    }

    return findIt->second.RunMulti(input, outputs, errorMessage);
}

void AIManager::LoadDefaultModels()
{
    std::wstring errorMessage;

    const std::array<std::pair<std::wstring, std::wstring>, 7> defaultModels = {
        std::make_pair(L"front", L"../Resources/AI/front.onnx"),
        std::make_pair(L"cover", L"../Resources/AI/cover.onnx"),
        std::make_pair(L"base_move", L"../Resources/AI/base_move.onnx"),
        std::make_pair(L"surround", L"../Resources/AI/surround.onnx"),
        std::make_pair(L"kiting", L"../Resources/AI/kiting.onnx"),
        std::make_pair(L"brass_boss", L"../Resources/AI/brass_boss.onnx"),
        std::make_pair(L"dragon_boss", L"../Resources/AI/dragon_boss.onnx"),
    };

    for (const auto& [modelKey, modelPath] : defaultModels)
    {
        if (!LoadModel(modelKey, modelPath, &errorMessage))
        {
            MJLOG_ERROR(ResourceLoad, "ONNX 모델 로드 실패 key={} path={} err={}", ws2s(modelKey), ws2s(modelPath), ws2s(errorMessage));
        }
    }
}
