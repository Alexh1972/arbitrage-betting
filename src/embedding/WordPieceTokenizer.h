#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace arb {

class WordPieceTokenizer {
public:
    explicit WordPieceTokenizer(const std::filesystem::path& tokenizerJson);

    std::vector<std::int64_t> encode(const std::string& text, size_t maxLength) const;

private:
    std::vector<std::u32string> basicTokenize(const std::string& text) const;
    void wordPiece(const std::u32string& word, std::vector<std::int64_t>& ids) const;
    std::int64_t tokenId(const std::string& token) const;

    std::unordered_map<std::string, std::int64_t> vocab_;
    std::string continuingSubwordPrefix_ = "##";
    size_t maxInputCharsPerWord_ = 100;
    std::int64_t clsId_ = 0;
    std::int64_t sepId_ = 0;
    std::int64_t unkId_ = 0;
};

}
