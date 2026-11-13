#include "service/MarketService.h"

#include "util/TimeUtil.h"

namespace arb {

MarketService::MarketService(MarketRepository& marketRepository) : marketRepository_(marketRepository) {}

void MarketService::save(const Market& market) {
    marketRepository_.save(market);
}

void MarketService::saveAll(const std::vector<Market>& markets) {
    marketRepository_.saveAll(markets);
}

std::vector<Market> MarketService::getRecentMarkets() {
    const std::string then = TimeUtil::toIsoString(TimeUtil::nowEpochSeconds() - 7 * TimeUtil::SECONDS_PER_DAY);
    return marketRepository_.getRecentMarkets(then);
}

}
