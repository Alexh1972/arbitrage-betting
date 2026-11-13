#pragma once

#include "model/Platform.h"

#include <cstdint>
#include <string>
#include <vector>

namespace arb {

class Market {
public:
    std::string marketId;
    Platform platform;
    std::string question;
    std::string slug;
    std::string outcomes;
    std::string outcomePrices;
    double volumeNum = 0.0;
    double liquidityNum = 0.0;
    bool active = false;
    std::string endDate;
    std::string startDate;

    void setQuestionVectorFloat(std::vector<float> vector);
    const std::vector<float>& getQuestionVectorFloat() const;

    void setQuestionVector(const std::vector<std::uint8_t>& bytes);
    std::vector<std::uint8_t> getQuestionVector() const;

    static std::uint32_t floatToIntBits(float value);
    static float intBitsToFloat(std::uint32_t bits);

private:
    std::vector<float> questionVector_;
};

}
