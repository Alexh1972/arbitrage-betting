#include "scheduler/PolymarketHandler.h"

#include "util/Logger.h"
#include "util/TimeUtil.h"

#include <atomic>
#include <thread>

namespace arb {

PolymarketHandler::PolymarketHandler(PlatformService& platformService, MarketService& marketService,
                                     ApiService& apiService, const EmbeddingModel& embeddingModel)
    : MarketAbstract(embeddingModel),
      platformService_(platformService),
      marketService_(marketService),
      apiService_(apiService) {}

Market PolymarketHandler::convertToMarket(const PolymarketMarketInfoResponse& apiResponse) {
    Market market;
    market.marketId = apiResponse.id;
    market.question = apiResponse.question;
    market.slug = apiResponse.slug;
    market.outcomes = apiResponse.outcomes;
    market.outcomePrices = apiResponse.outcomePrices;
    market.volumeNum = apiResponse.volumeNum;
    market.liquidityNum = apiResponse.liquidityNum;
    market.active = apiResponse.active;
    market.endDate = apiResponse.endDate;
    market.startDate = apiResponse.startDate;
    market.platform = platformService_.getPlatform("Polymarket").value_or(Platform{});
    return market;
}

int PolymarketHandler::readPeriod(const std::string& endDateMin, const std::string& endDateMax) {
    int marketsRead = 0;
    std::optional<std::string> cursor = "";

    while (cursor) {
        try {
            const std::string url = "https://gamma-api.polymarket.com/markets/keyset?closed=false&limit=" + std::to_string(LIMIT)
                + "&after_cursor=" + *cursor
                + "&end_date_min=" + endDateMin
                + "&end_date_max=" + endDateMax;

            const HttpResponse response = apiService_.publish("POLYMARKET", url, 10);
            const std::string& jsonResponse = response.body;

            PolymarketMarketResponse polymarketMarkets;
            try {
                polymarketMarkets = nlohmann::json::parse(jsonResponse).get<PolymarketMarketResponse>();
            } catch (const std::exception& e) {
                Logger::error("Error during deserializing page {}: {}", jsonResponse, e.what());
                continue;
            }

            cursor = polymarketMarkets.nextCursor;

            if (!polymarketMarkets.markets) {
                Logger::error("[POLY] Markets is null: {}", jsonResponse);
                break;
            }

            std::vector<Market> markets;
            markets.reserve(polymarketMarkets.markets->size());
            for (const auto& info : *polymarketMarkets.markets) {
                markets.push_back(convertToMarket(info));
            }

            handleEmbedding(markets);
            marketService_.saveAll(markets);
            marketsRead += static_cast<int>(markets.size());
        } catch (const std::exception& e) {
            Logger::error("Error during reading page: {}", e.what());
        }
    }

    return marketsRead;
}

void PolymarketHandler::read() {
    setRecentMarkets(marketService_.getRecentMarkets());

    const long long totalMinSec = TimeUtil::nowEpochSeconds();
    const long long totalMaxSec = totalMinSec + 7 * TimeUtil::SECONDS_PER_DAY;
    const long long chunkDurationSec = (totalMaxSec - totalMinSec) / NTHREADS;

    std::atomic<int> marketsRead = 0;
    std::vector<std::jthread> threads;
    threads.reserve(NTHREADS);

    for (int i = 0; i < NTHREADS; ++i) {
        const long long threadMinSec = totalMinSec + i * chunkDurationSec;
        const long long threadMaxSec = (i == NTHREADS - 1) ? totalMaxSec : threadMinSec + chunkDurationSec;

        threads.emplace_back([this, endDateMin = TimeUtil::toIsoString(threadMinSec),
                              endDateMax = TimeUtil::toIsoString(threadMaxSec), &marketsRead] {
            marketsRead += readPeriod(endDateMin, endDateMax);
        });
    }

    threads.clear();
    Logger::info("[POLY] Total read {} markets", marketsRead.load());
}

}
