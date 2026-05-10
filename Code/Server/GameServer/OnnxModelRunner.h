#pragma once
#include "pch.h"
#include "onnxruntime/onnxruntime_cxx_api.h"


class OnnxModelRunner
{
public:
    static constexpr size_t kInputSize = 24;
    static constexpr size_t kOutputSize = 2;

public:
    OnnxModelRunner();

    bool Load(const std::wstring& modelPath, std::wstring* errorMessage = nullptr);
    bool IsLoaded() const;

    bool Run(
        const std::array<float, kInputSize>& input,
        std::array<float, kOutputSize>& output,
        std::wstring* errorMessage = nullptr) const;

private:
    static std::wstring ToWide(const std::string& text);

private:
    Ort::Env mEnv;
    Ort::SessionOptions mSessionOptions;
    std::unique_ptr<Ort::Session> mSession;
    std::string mInputName;
    std::string mOutputName;
};
