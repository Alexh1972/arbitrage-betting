#pragma once

#include "model/Platform.h"
#include "repository/PlatformRepository.h"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace arb {

class PlatformService {
public:
    explicit PlatformService(PlatformRepository& platformRepository);

    std::optional<Platform> getPlatform(const std::string& name);
    Platform save(const Platform& platform);

private:
    PlatformRepository& platformRepository_;
    std::mutex mutex_;
    std::unordered_map<std::string, Platform> cache_;
};

}
