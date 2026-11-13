#include "util/TimeUtil.h"

#include <chrono>
#include <format>

namespace arb::TimeUtil {

long long nowEpochSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

long long nowEpochMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string toIsoString(long long epochSeconds) {
    const std::chrono::sys_seconds time{std::chrono::seconds{epochSeconds}};
    return std::format("{:%Y-%m-%dT%H:%M:%S}Z", time);
}

}
