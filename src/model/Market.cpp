#include "model/Market.h"

#include <cstring>

namespace arb {

std::uint32_t Market::floatToIntBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float Market::intBitsToFloat(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void Market::setQuestionVectorFloat(std::vector<float> vector) {
    questionVector_ = std::move(vector);
}

const std::vector<float>& Market::getQuestionVectorFloat() const {
    return questionVector_;
}

void Market::setQuestionVector(const std::vector<std::uint8_t>& bytes) {
    questionVector_.clear();
    questionVector_.reserve(bytes.size() / 4);

    for (size_t i = 0; i + 3 < bytes.size(); i += 4) {
        const std::uint32_t bits = static_cast<std::uint32_t>(bytes[i]) << 24
            | static_cast<std::uint32_t>(bytes[i + 1]) << 16
            | static_cast<std::uint32_t>(bytes[i + 2]) << 8
            | static_cast<std::uint32_t>(bytes[i + 3]);
        questionVector_.push_back(intBitsToFloat(bits));
    }
}

std::vector<std::uint8_t> Market::getQuestionVector() const {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(questionVector_.size() * 4);

    for (const float value : questionVector_) {
        const std::uint32_t bits = floatToIntBits(value);
        bytes.push_back(static_cast<std::uint8_t>(bits >> 24));
        bytes.push_back(static_cast<std::uint8_t>(bits >> 16));
        bytes.push_back(static_cast<std::uint8_t>(bits >> 8));
        bytes.push_back(static_cast<std::uint8_t>(bits));
    }

    return bytes;
}

}
