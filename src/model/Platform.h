#pragma once

#include <string>

namespace arb {

struct Platform {
    long long platformId = 0;
    std::string name;

    Platform() = default;
    explicit Platform(std::string platformName) : name(std::move(platformName)) {}
    Platform(long long id, std::string platformName) : platformId(id), name(std::move(platformName)) {}
};

}
