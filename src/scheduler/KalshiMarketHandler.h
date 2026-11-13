#pragma once

#include "dto/KalshiMarketResponse.h"
#include "scheduler/MarketAbstract.h"
#include "service/ApiService.h"
#include "service/MarketService.h"
#include "service/PlatformService.h"

namespace arb {

class KalshiMarketHandler : public MarketAbstract {
public:
    KalshiMarketHandler(PlatformService& platformService, MarketService& marketService,
                        ApiService& apiService, const EmbeddingModel& embeddingModel);

    Market convertToMarket(const KalshiMarketInfoResponse& apiResponse);
    int readPeriod(long long endDateMin, long long endDateMax);
    void read() override;

private:
    static constexpr int LIMIT = 1000;
    static constexpr int NTHREADS = 30;

    PlatformService& platformService_;
    MarketService& marketService_;
    ApiService& apiService_;
};

}
