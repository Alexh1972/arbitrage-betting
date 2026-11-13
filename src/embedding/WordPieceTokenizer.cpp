#include "embedding/WordPieceTokenizer.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace arb {

namespace {

std::u32string decodeUtf8(const std::string& text) {
    std::u32string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        char32_t cp;
        size_t length;

        if (c < 0x80) {
            cp = c;
            length = 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            length = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            length = 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            length = 4;
        } else {
            result.push_back(0xFFFD);
            ++i;
            continue;
        }

        if (i + length > text.size()) {
            result.push_back(0xFFFD);
            break;
        }

        bool valid = true;
        for (size_t j = 1; j < length; ++j) {
            const auto next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (next & 0x3F);
        }

        if (!valid) {
            result.push_back(0xFFFD);
            ++i;
            continue;
        }

        result.push_back(cp);
        i += length;
    }

    return result;
}

void appendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::string encodeUtf8(const std::u32string& text, size_t begin, size_t end) {
    std::string out;
    for (size_t i = begin; i < end; ++i) {
        appendUtf8(out, text[i]);
    }
    return out;
}

bool isWhitespace(char32_t cp) {
    return cp == U' ' || cp == U'\t' || cp == U'\n' || cp == U'\r' || cp == 0xA0 || cp == 0x1680
        || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

bool isControl(char32_t cp) {
    if (cp == U'\t' || cp == U'\n' || cp == U'\r') {
        return false;
    }
    return cp < 0x20 || (cp >= 0x7F && cp <= 0x9F) || cp == 0xAD || (cp >= 0x200B && cp <= 0x200F)
        || (cp >= 0x202A && cp <= 0x202E) || (cp >= 0x2060 && cp <= 0x2064) || cp == 0xFEFF;
}

bool isPunctuation(char32_t cp) {
    if ((cp >= 33 && cp <= 47) || (cp >= 58 && cp <= 64) || (cp >= 91 && cp <= 96) || (cp >= 123 && cp <= 126)) {
        return true;
    }
    return cp == 0xA1 || cp == 0xA7 || cp == 0xAB || cp == 0xB6 || cp == 0xB7 || cp == 0xBB || cp == 0xBF
        || (cp >= 0x2010 && cp <= 0x2027) || (cp >= 0x2030 && cp <= 0x205E)
        || (cp >= 0x3001 && cp <= 0x3003) || (cp >= 0x3008 && cp <= 0x3011) || (cp >= 0x3014 && cp <= 0x301F)
        || (cp >= 0xFF01 && cp <= 0xFF0F) || (cp >= 0xFF1A && cp <= 0xFF20) || (cp >= 0xFF3B && cp <= 0xFF40)
        || (cp >= 0xFF5B && cp <= 0xFF65);
}

bool isChineseChar(char32_t cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x20000 && cp <= 0x2A6DF)
        || (cp >= 0x2A700 && cp <= 0x2B73F) || (cp >= 0x2B740 && cp <= 0x2B81F) || (cp >= 0x2B820 && cp <= 0x2CEAF)
        || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0x2F800 && cp <= 0x2FA1F);
}

bool isCombiningMark(char32_t cp) {
    return (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) || (cp >= 0x1DC0 && cp <= 0x1DFF)
        || (cp >= 0x20D0 && cp <= 0x20FF) || (cp >= 0xFE20 && cp <= 0xFE2F);
}

char32_t toLower(char32_t cp) {
    if (cp >= U'A' && cp <= U'Z') {
        return cp + 32;
    }
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) {
        return cp + 32;
    }
    if (cp >= 0x100 && cp <= 0x137) {
        return cp | 1;
    }
    if (cp >= 0x139 && cp <= 0x148) {
        return (cp & 1) ? cp + 1 : cp;
    }
    if (cp >= 0x14A && cp <= 0x177) {
        return cp | 1;
    }
    if (cp == 0x178) {
        return 0xFF;
    }
    if (cp >= 0x179 && cp <= 0x17E) {
        return (cp & 1) ? cp + 1 : cp;
    }
    if (cp >= 0x218 && cp <= 0x21B) {
        return cp | 1;
    }
    if (cp >= 0x391 && cp <= 0x3A9 && cp != 0x3A2) {
        return cp + 32;
    }
    if (cp >= 0x410 && cp <= 0x42F) {
        return cp + 32;
    }
    if (cp >= 0x400 && cp <= 0x40F) {
        return cp + 80;
    }
    return cp;
}

char32_t stripAccent(char32_t cp) {
    static const char32_t latin1[] = {
        U'a', U'a', U'a', U'a', U'a', U'a', 0xE6, U'c', U'e', U'e', U'e', U'e', U'i', U'i', U'i', U'i',
        0xF0, U'n', U'o', U'o', U'o', U'o', U'o', 0xF7, 0xF8, U'u', U'u', U'u', U'u', U'y', 0xFE, U'y',
    };
    static const char32_t latinExtendedA[] = {
        U'a', U'a', U'a', U'a', U'a', U'a', U'c', U'c', U'c', U'c', U'c', U'c', U'c', U'c', U'd', U'd',
        0x110, 0x111, U'e', U'e', U'e', U'e', U'e', U'e', U'e', U'e', U'e', U'e', U'g', U'g', U'g', U'g',
        U'g', U'g', U'g', U'g', U'h', U'h', 0x126, 0x127, U'i', U'i', U'i', U'i', U'i', U'i', U'i', U'i',
        U'i', 0x131, 0x132, 0x133, U'j', U'j', U'k', U'k', 0x138, U'l', U'l', U'l', U'l', U'l', U'l', 0x13F,
        0x140, 0x141, 0x142, U'n', U'n', U'n', U'n', U'n', U'n', 0x149, 0x14A, 0x14B, U'o', U'o', U'o', U'o',
        U'o', U'o', 0x152, 0x153, U'r', U'r', U'r', U'r', U'r', U'r', U's', U's', U's', U's', U's', U's',
        U's', U's', U't', U't', U't', U't', 0x166, 0x167, U'u', U'u', U'u', U'u', U'u', U'u', U'u', U'u',
        U'u', U'u', U'u', U'u', U'w', U'w', U'y', U'y', U'y', U'z', U'z', U'z', U'z', U'z', U'z', 0x17F,
    };

    if (cp >= 0xE0 && cp <= 0xFF) {
        return latin1[cp - 0xE0];
    }
    if (cp >= 0x100 && cp <= 0x17F) {
        return latinExtendedA[cp - 0x100];
    }
    if (cp == 0x219) {
        return U's';
    }
    if (cp == 0x21B) {
        return U't';
    }
    return cp;
}

}

WordPieceTokenizer::WordPieceTokenizer(const std::filesystem::path& tokenizerJson) {
    std::ifstream in(tokenizerJson);
    if (!in) {
        throw std::runtime_error("Cannot open tokenizer " + tokenizerJson.string());
    }

    const nlohmann::json json = nlohmann::json::parse(in);
    const auto& model = json.at("model");

    for (const auto& [token, id] : model.at("vocab").items()) {
        vocab_.emplace(token, id.get<std::int64_t>());
    }

    if (model.contains("continuing_subword_prefix") && model["continuing_subword_prefix"].is_string()) {
        continuingSubwordPrefix_ = model["continuing_subword_prefix"].get<std::string>();
    }
    if (model.contains("max_input_chars_per_word") && model["max_input_chars_per_word"].is_number()) {
        maxInputCharsPerWord_ = model["max_input_chars_per_word"].get<size_t>();
    }

    const std::string unkToken = model.contains("unk_token") ? model["unk_token"].get<std::string>() : "[UNK]";
    unkId_ = tokenId(unkToken);
    clsId_ = tokenId("[CLS]");
    sepId_ = tokenId("[SEP]");
}

std::int64_t WordPieceTokenizer::tokenId(const std::string& token) const {
    const auto it = vocab_.find(token);
    if (it == vocab_.end()) {
        throw std::runtime_error("Token " + token + " is missing from vocabulary");
    }
    return it->second;
}

std::vector<std::u32string> WordPieceTokenizer::basicTokenize(const std::string& text) const {
    std::vector<std::u32string> words;
    std::u32string current;

    const auto flush = [&] {
        if (!current.empty()) {
            words.push_back(std::move(current));
            current.clear();
        }
    };

    for (char32_t cp : decodeUtf8(text)) {
        if (cp == 0 || cp == 0xFFFD || isControl(cp)) {
            continue;
        }
        if (isWhitespace(cp)) {
            flush();
            continue;
        }
        if (isChineseChar(cp)) {
            flush();
            words.emplace_back(1, cp);
            continue;
        }

        cp = stripAccent(toLower(cp));
        if (isCombiningMark(cp)) {
            continue;
        }

        if (isPunctuation(cp)) {
            flush();
            words.emplace_back(1, cp);
            continue;
        }

        current.push_back(cp);
    }
    flush();

    return words;
}

void WordPieceTokenizer::wordPiece(const std::u32string& word, std::vector<std::int64_t>& ids) const {
    if (word.size() > maxInputCharsPerWord_) {
        ids.push_back(unkId_);
        return;
    }

    std::vector<std::int64_t> pieces;
    size_t start = 0;

    while (start < word.size()) {
        size_t end = word.size();
        bool found = false;

        while (start < end) {
            std::string piece = encodeUtf8(word, start, end);
            if (start > 0) {
                piece = continuingSubwordPrefix_ + piece;
            }

            const auto it = vocab_.find(piece);
            if (it != vocab_.end()) {
                pieces.push_back(it->second);
                found = true;
                break;
            }
            --end;
        }

        if (!found) {
            ids.push_back(unkId_);
            return;
        }
        start = end;
    }

    ids.insert(ids.end(), pieces.begin(), pieces.end());
}

std::vector<std::int64_t> WordPieceTokenizer::encode(const std::string& text, size_t maxLength) const {
    std::vector<std::int64_t> ids;
    ids.push_back(clsId_);

    for (const auto& word : basicTokenize(text)) {
        wordPiece(word, ids);
    }

    if (ids.size() > maxLength - 1) {
        ids.resize(maxLength - 1);
    }
    ids.push_back(sepId_);

    return ids;
}

}
