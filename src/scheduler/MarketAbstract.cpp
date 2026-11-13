#include "scheduler/MarketAbstract.h"

#include "util/Logger.h"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace arb {

std::shared_ptr<const std::vector<Market>> MarketAbstract::recentMarkets_ = std::make_shared<const std::vector<Market>>();
std::shared_mutex MarketAbstract::rwLock_;

MarketAbstract::MarketAbstract(const EmbeddingModel& embeddingModel) : embeddingModel_(embeddingModel) {}

std::shared_ptr<const std::vector<Market>> MarketAbstract::getRecentMarkets() const {
    std::shared_lock lock(rwLock_);
    return recentMarkets_;
}

void MarketAbstract::setRecentMarkets(std::vector<Market> markets) {
    auto updated = std::make_shared<const std::vector<Market>>(std::move(markets));
    std::unique_lock lock(rwLock_);
    recentMarkets_ = std::move(updated);
}

std::vector<const Market*> MarketAbstract::getSimilarMarkets(const std::vector<Market>& candidates, const Market& market) const {
    std::vector<const Market*> similar;
    const long long platformId = market.platform.platformId;
    const auto& questionVector = market.getQuestionVectorFloat();

    for (const Market& candidate : candidates) {
        if (candidate.platform.platformId == platformId) {
            continue;
        }

        if (cosineSimilarity(candidate.getQuestionVectorFloat(), questionVector) >= SIMILARITY_THRESHOLD) {
            similar.push_back(&candidate);
        }
    }

    return similar;
}

double MarketAbstract::cosineSimilarity(const std::vector<float>& f1, const std::vector<float>& f2) {
    double product = 0.0;
    double d1 = 0.0;
    double d2 = 0.0;
    const size_t length = std::min(f1.size(), f2.size());

    for (size_t i = 0; i < length; ++i) {
        product += static_cast<double>(f1[i]) * f2[i];
        d1 += static_cast<double>(f1[i]) * f1[i];
        d2 += static_cast<double>(f2[i]) * f2[i];
    }

    if (d1 == 0.0 || d2 == 0.0) {
        return 0.0;
    }

    return product / std::sqrt(d1 * d2);
}

void MarketAbstract::handleEmbedding(std::vector<Market>& markets) const {
    if (markets.empty()) {
        return;
    }

    std::vector<std::string> questions;
    questions.reserve(markets.size());
    for (const Market& market : markets) {
        questions.push_back(market.question);
    }

    auto vectors = embeddingModel_.embedAll(questions);
    for (size_t i = 0; i < vectors.size(); ++i) {
        markets[i].setQuestionVectorFloat(std::move(vectors[i]));
    }

    const auto recentMarkets = getRecentMarkets();
    for (const Market& market : markets) {
        for (const Market* s : getSimilarMarkets(*recentMarkets, market)) {
            Logger::info("[{} | {}] (Outcomes: {}, Prices: {}) \"{}\" -> [{} | {}] (Outcomes: {}, Prices: {}) \"{}\"",
                market.platform.name,
                market.marketId,
                market.outcomes,
                market.outcomePrices,
                market.question,
                s->platform.name,
                s->marketId,
                s->outcomes,
                s->outcomePrices,
                s->question);
        }
    }
}

}
