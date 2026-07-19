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
        mInputName = inputName.get();
        mOutputNames.clear();
        const size_t outputCount = mSession->GetOutputCount();
        mOutputNames.reserve(outputCount);
        for (size_t i = 0; i < outputCount; ++i)
        {
            auto outputName = mSession->GetOutputNameAllocated(i, allocator);
            mOutputNames.emplace_back(outputName.get());
        }

        return true;
    }
    catch (const Ort::Exception& ex)
    {
        mSession.reset();
        mInputName.clear();
        mOutputNames.clear();

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
    std::vector<std::vector<float>> outputs;
    if (!RunMulti(std::vector<float>(input.begin(), input.end()), outputs, errorMessage))
        return false;

    if (outputs.empty() || outputs[0].size() < kOutputSize)
    {
        if (errorMessage)
            *errorMessage = L"ONNX output tensor has fewer elements than expected.";
        return false;
    }

    for (size_t i = 0; i < kOutputSize; ++i)
        output[i] = outputs[0][i];
    return true;
}

bool OnnxModelRunner::RunMulti(
    const std::vector<float>& input,
    std::vector<std::vector<float>>& outputs,
    std::wstring* errorMessage) const
{
    outputs.clear();
    if (!mSession)
    {
        if (errorMessage)
            *errorMessage = L"ONNX session is not loaded.";
        return false;
    }
    if (input.empty() || mOutputNames.empty())
    {
        if (errorMessage)
            *errorMessage = L"ONNX input or output metadata is empty.";
        return false;
    }

    try
    {
        std::array<int64_t, 2> inputShape = { 1, static_cast<int64_t>(input.size()) };

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float*>(input.data()),
            input.size(),
            inputShape.data(),
            inputShape.size());

        const char* inputNames[] = { mInputName.c_str() };
        std::vector<const char*> outputNames;
        outputNames.reserve(mOutputNames.size());
        for (const std::string& outputName : mOutputNames)
            outputNames.push_back(outputName.c_str());

        auto ortOutputs = mSession->Run(
            Ort::RunOptions{ nullptr },
            inputNames,
            &inputTensor,
            1,
            outputNames.data(),
            outputNames.size());

        if (ortOutputs.size() != mOutputNames.size())
        {
            if (errorMessage)
                *errorMessage = L"ONNX returned an unexpected number of outputs.";
            return false;
        }

        outputs.reserve(ortOutputs.size());
        for (const Ort::Value& ortOutput : ortOutputs)
        {
            if (!ortOutput.IsTensor())
            {
                if (errorMessage)
                    *errorMessage = L"ONNX output is not a tensor.";
                return false;
            }

            const size_t count = ortOutput.GetTensorTypeAndShapeInfo().GetElementCount();
            const float* data = ortOutput.GetTensorData<float>();
            outputs.emplace_back(data, data + count);
        }

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
