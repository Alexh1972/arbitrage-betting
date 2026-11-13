#pragma once

#include "model/Market.h"
#include "repository/MarketRepository.h"

#include <vector>

namespace arb {

class MarketService {
public:
    explicit MarketService(MarketRepository& marketRepository);

    void save(const Market& market);
    void saveAll(const std::vector<Market>& markets);
    std::vector<Market> getRecentMarkets();

private:
    MarketRepository& marketRepository_;
};

}
