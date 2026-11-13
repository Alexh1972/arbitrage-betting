#pragma once

#include "embedding/WordPieceTokenizer.h"

#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace arb {

class EmbeddingModel {
public:
    EmbeddingModel(const std::filesystem::path& modelPath, const std::filesystem::path& tokenizerPath);

    std::vector<float> embed(const std::string& text) const;
    std::vector<std::vector<float>> embedAll(const std::vector<std::string>& texts) const;

private:
    static constexpr size_t MAX_LENGTH = 512;

    Ort::Env env_;
    std::unique_ptr<Ort::Session> session_;
    WordPieceTokenizer tokenizer_;
    std::vector<std::string> inputNames_;
    std::vector<std::string> outputNames_;
};

}
