#pragma once

#include "db/OracleDatabase.h"
#include "model/Market.h"

#include <string>
#include <vector>

namespace arb {

class MarketRepository {
public:
    explicit MarketRepository(OracleDatabase& database);

    void createSchema();
    void save(const Market& market);
    void saveAll(const std::vector<Market>& markets);
    std::vector<Market> getRecentMarkets(const std::string& then);

private:
    void save(Connection& connection, const Market& market);

    OracleDatabase& database_;
};

}
