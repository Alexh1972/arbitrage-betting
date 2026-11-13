#pragma once

#include <string>

namespace arb::TimeUtil {

long long nowEpochSeconds();
long long nowEpochMillis();
std::string toIsoString(long long epochSeconds);

constexpr long long SECONDS_PER_DAY = 24LL * 60 * 60;

}
