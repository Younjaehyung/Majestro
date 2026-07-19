#pragma once

#include "OnnxModelRunner.h"

#include <unordered_map>

class AIManager
{
public:
    using InputArray = std::array<float, OnnxModelRunner::kInputSize>;
    using OutputArray = std::array<float, OnnxModelRunner::kOutputSize>;
    using DynamicInput = std::vector<float>;
    using DynamicOutputs = std::vector<std::vector<float>>;

public:
    void Initialize();

    bool LoadModel(const std::wstring& modelKey, const std::wstring& modelPath, std::wstring* errorMessage = nullptr);
    bool HasModel(const std::wstring& modelKey) const;

    bool RunModel(
        const std::wstring& modelKey,
        const InputArray& input,
        OutputArray& output,
        std::wstring* errorMessage = nullptr) const;

    bool RunModelMulti(
        const std::wstring& modelKey,
        const DynamicInput& input,
        DynamicOutputs& outputs,
        std::wstring* errorMessage = nullptr) const;

private:
    void LoadDefaultModels();

private:
    std::unordered_map<std::wstring, OnnxModelRunner> mModels;
};
