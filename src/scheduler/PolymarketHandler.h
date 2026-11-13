#pragma once

#include "dto/PolymarketMarketResponse.h"
#include "scheduler/MarketAbstract.h"
#include "service/ApiService.h"
#include "service/MarketService.h"
#include "service/PlatformService.h"

namespace arb {

class PolymarketHandler : public MarketAbstract {
public:
    PolymarketHandler(PlatformService& platformService, MarketService& marketService,
                      ApiService& apiService, const EmbeddingModel& embeddingModel);

    Market convertToMarket(const PolymarketMarketInfoResponse& apiResponse);
    int readPeriod(const std::string& endDateMin, const std::string& endDateMax);
    void read() override;

private:
    static constexpr int LIMIT = 100;
    static constexpr int NTHREADS = 30;

    PlatformService& platformService_;
    MarketService& marketService_;
    ApiService& apiService_;
};

}
