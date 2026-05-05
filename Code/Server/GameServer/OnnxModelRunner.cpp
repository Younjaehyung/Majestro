#include "pch.h"
#include "OnnxModelRunner.h"

OnnxModelRunner::OnnxModelRunner()
    : mEnv(ORT_LOGGING_LEVEL_WARNING, "MajestroOnnx")
{
    mSessionOptions.SetIntraOpNumThreads(1);
    mSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
}

bool OnnxModelRunner::Load(const std::wstring& modelPath, std::wstring* errorMessage)
{
    try
    {
        mSession = std::make_unique<Ort::Session>(mEnv, modelPath.c_str(), mSessionOptions);

        Ort::AllocatorWithDefaultOptions allocator;

        auto inputName = mSession->GetInputNameAllocated(0, allocator);
        auto outputName = mSession->GetOutputNameAllocated(0, allocator);

        mInputName = inputName.get();
        mOutputName = outputName.get();

        return true;
    }
    catch (const Ort::Exception& ex)
    {
        mSession.reset();
        mInputName.clear();
        mOutputName.clear();

        if (errorMessage)
            *errorMessage = ToWide(ex.what());
        return false;
    }
}

bool OnnxModelRunner::IsLoaded() const
{
    return mSession != nullptr;
}

bool OnnxModelRunner::Run(
    const std::array<float, kInputSize>& input,
    std::array<float, kOutputSize>& output,
    std::wstring* errorMessage) const
{
    if (!mSession)
    {
        if (errorMessage)
            *errorMessage = L"ONNX session is not loaded.";
        return false;
    }

    try
    {
        std::array<int64_t, 2> inputShape = { 1, static_cast<int64_t>(kInputSize) };

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float*>(input.data()),
            input.size(),
            inputShape.data(),
            inputShape.size());

        const char* inputNames[] = { mInputName.c_str() };
        const char* outputNames[] = { mOutputName.c_str() };

        auto outputs = mSession->Run(
            Ort::RunOptions{ nullptr },
            inputNames,
            &inputTensor,
            1,
            outputNames,
            1);

        if (outputs.empty() || !outputs[0].IsTensor())
        {
            if (errorMessage)
                *errorMessage = L"ONNX output is empty or not a tensor.";
            return false;
        }

        const Ort::TensorTypeAndShapeInfo outputInfo = outputs[0].GetTensorTypeAndShapeInfo();
        const size_t outputCount = outputInfo.GetElementCount();
        if (outputCount < kOutputSize)
        {
            if (errorMessage)
                *errorMessage = L"ONNX output tensor has fewer elements than expected.";
            return false;
        }

        const float* outputData = outputs[0].GetTensorData<float>();
        for (size_t i = 0; i < kOutputSize; ++i)
            output[i] = outputData[i];

        return true;
    }
    catch (const Ort::Exception& ex)
    {
        if (errorMessage)
            *errorMessage = ToWide(ex.what());
        return false;
    }
}

std::wstring OnnxModelRunner::ToWide(const std::string& text)
{
    return s2ws(text);
}
