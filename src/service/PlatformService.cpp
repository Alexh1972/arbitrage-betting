#include "service/PlatformService.h"

namespace arb {

PlatformService::PlatformService(PlatformRepository& platformRepository) : platformRepository_(platformRepository) {}

std::optional<Platform> PlatformService::getPlatform(const std::string& name) {
    std::lock_guard lock(mutex_);
    if (const auto it = cache_.find(name); it != cache_.end()) {
        return it->second;
    }

    auto platform = platformRepository_.getPlatformByName(name);
    if (platform) {
        cache_[name] = *platform;
    }
    return platform;
}

Platform PlatformService::save(const Platform& platform) {
    if (auto oldPlatform = getPlatform(platform.name)) {
        return *oldPlatform;
    }

    Platform saved = platformRepository_.save(platform);
    std::lock_guard lock(mutex_);
    cache_[saved.name] = saved;
    return saved;
}

}
