#include "embedding/EmbeddingModel.h"

#include <array>
#include <cmath>
#include <stdexcept>

namespace arb {

EmbeddingModel::EmbeddingModel(const std::filesystem::path& modelPath, const std::filesystem::path& tokenizerPath)
    : env_(ORT_LOGGING_LEVEL_WARNING, "embedding"), tokenizer_(tokenizerPath) {
    Ort::SessionOptions options;
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_ = std::make_unique<Ort::Session>(env_, modelPath.c_str(), options);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < session_->GetInputCount(); ++i) {
        inputNames_.emplace_back(session_->GetInputNameAllocated(i, allocator).get());
    }
    for (size_t i = 0; i < session_->GetOutputCount(); ++i) {
        outputNames_.emplace_back(session_->GetOutputNameAllocated(i, allocator).get());
    }
}

std::vector<float> EmbeddingModel::embed(const std::string& text) const {
    std::vector<std::int64_t> inputIds = tokenizer_.encode(text, MAX_LENGTH);
    std::vector<std::int64_t> attentionMask(inputIds.size(), 1);
    std::vector<std::int64_t> tokenTypeIds(inputIds.size(), 0);
    const std::array<std::int64_t, 2> shape{1, static_cast<std::int64_t>(inputIds.size())};

    const auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> inputs;
    std::vector<const char*> inputNames;
    for (const auto& name : inputNames_) {
        std::vector<std::int64_t>* data;
        if (name == "input_ids") {
            data = &inputIds;
        } else if (name == "attention_mask") {
            data = &attentionMask;
        } else if (name == "token_type_ids") {
            data = &tokenTypeIds;
        } else {
            throw std::runtime_error("Unsupported model input " + name);
        }
        inputs.push_back(Ort::Value::CreateTensor<std::int64_t>(memoryInfo, data->data(), data->size(), shape.data(), shape.size()));
        inputNames.push_back(name.c_str());
    }

    const char* outputName = outputNames_.front().c_str();
    auto outputs = session_->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputs.data(), inputs.size(), &outputName, 1);

    const auto outputShape = outputs.front().GetTensorTypeAndShapeInfo().GetShape();
    const auto hiddenSize = static_cast<size_t>(outputShape.back());
    const float* hidden = outputs.front().GetTensorData<float>();

    std::vector<float> embedding(hidden, hidden + hiddenSize);

    double norm = 0.0;
    for (const float value : embedding) {
        norm += static_cast<double>(value) * value;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0) {
        for (float& value : embedding) {
            value = static_cast<float>(value / norm);
        }
    }

    return embedding;
}

std::vector<std::vector<float>> EmbeddingModel::embedAll(const std::vector<std::string>& texts) const {
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size());
    for (const auto& text : texts) {
        embeddings.push_back(embed(text));
    }
    return embeddings;
}

}
