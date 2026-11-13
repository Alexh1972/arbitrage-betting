#include "scheduler/KalshiMarketHandler.h"

#include "util/Logger.h"
#include "util/TimeUtil.h"

#include <atomic>
#include <thread>

namespace arb {

KalshiMarketHandler::KalshiMarketHandler(PlatformService& platformService, MarketService& marketService,
                                         ApiService& apiService, const EmbeddingModel& embeddingModel)
    : MarketAbstract(embeddingModel),
      platformService_(platformService),
      marketService_(marketService),
      apiService_(apiService) {}

Market KalshiMarketHandler::convertToMarket(const KalshiMarketInfoResponse& apiResponse) {
    Market market;
    market.marketId = apiResponse.id;
    market.question = apiResponse.question;
    market.slug = apiResponse.slug;
    market.outcomes = "[\"Yes\", \"No\"]";
    market.outcomePrices = "[\"" + apiResponse.yesAsk.value_or("null") + "\", \"" + apiResponse.noAsk.value_or("null") + "\"]";
    market.volumeNum = apiResponse.volumeNum;
    market.liquidityNum = apiResponse.liquidityNum;
    market.active = true;
    market.endDate = apiResponse.endDate;
    market.startDate = apiResponse.startDate;
    market.platform = platformService_.getPlatform("Kalshi").value_or(Platform{});
    return market;
}

int KalshiMarketHandler::readPeriod(long long endDateMin, long long endDateMax) {
    int marketsRead = 0;
    std::optional<std::string> cursor;

    while (cursor != "") {
        try {
            std::string url = "https://external-api.kalshi.com/trade-api/v2/markets?status=open&limit=" + std::to_string(LIMIT)
                + "&min_close_ts=" + std::to_string(endDateMin)
                + "&max_close_ts=" + std::to_string(endDateMax);

            if (cursor) {
                url += "&cursor=" + *cursor;
            }

            const HttpResponse response = apiService_.publish("KALSHI", url, 500);
            const std::string& jsonResponse = response.body;

            KalshiMarketResponse kalshiMarketResponse;
            try {
                kalshiMarketResponse = nlohmann::json::parse(jsonResponse).get<KalshiMarketResponse>();
            } catch (const std::exception& e) {
                Logger::error("Error during deserializing page {}: {}", jsonResponse, e.what());
                continue;
            }

            cursor = kalshiMarketResponse.cursor;

            if (!kalshiMarketResponse.markets) {
                Logger::error("[KALSHI] Markets is null: {}", jsonResponse);
                break;
            }

            std::vector<Market> markets;
            markets.reserve(kalshiMarketResponse.markets->size());
            for (const auto& info : *kalshiMarketResponse.markets) {
                markets.push_back(convertToMarket(info));
            }

            handleEmbedding(markets);
            marketService_.saveAll(markets);
            marketsRead += static_cast<int>(markets.size());

            if (!cursor) {
                break;
            }
        } catch (const std::exception& e) {
            Logger::error("Error during reading page: {}", e.what());
        }
    }

    return marketsRead;
}

void KalshiMarketHandler::read() {
    setRecentMarkets(marketService_.getRecentMarkets());

    const long long totalMin = TimeUtil::nowEpochSeconds();
    const long long totalMax = totalMin + 7 * TimeUtil::SECONDS_PER_DAY;
    const long long chunkDuration = (totalMax - totalMin) / NTHREADS;

    std::atomic<int> marketsRead = 0;
    std::vector<std::jthread> threads;
    threads.reserve(NTHREADS);

    for (int i = 0; i < NTHREADS; ++i) {
        const long long threadMin = totalMin + i * chunkDuration;
        const long long threadMax = (i == NTHREADS - 1) ? totalMax : threadMin + chunkDuration;

        threads.emplace_back([this, threadMin, threadMax, &marketsRead] {
            marketsRead += readPeriod(threadMin, threadMax);
        });
    }

    threads.clear();
    Logger::info("[KALSHI] Total read {} markets", marketsRead.load());
}

}
