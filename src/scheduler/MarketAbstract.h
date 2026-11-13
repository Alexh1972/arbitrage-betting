#pragma once

#include "embedding/EmbeddingModel.h"
#include "model/Market.h"

#include <memory>
#include <shared_mutex>
#include <vector>

namespace arb {

class MarketAbstract {
public:
    virtual ~MarketAbstract() = default;

    virtual void read() = 0;

    std::shared_ptr<const std::vector<Market>> getRecentMarkets() const;
    void setRecentMarkets(std::vector<Market> markets);

    std::vector<const Market*> getSimilarMarkets(const std::vector<Market>& candidates, const Market& market) const;
    static double cosineSimilarity(const std::vector<float>& f1, const std::vector<float>& f2);

protected:
    explicit MarketAbstract(const EmbeddingModel& embeddingModel);

    void handleEmbedding(std::vector<Market>& markets) const;

    const EmbeddingModel& embeddingModel_;

private:
    static constexpr double SIMILARITY_THRESHOLD = 0.95;

    static std::shared_ptr<const std::vector<Market>> recentMarkets_;
    static std::shared_mutex rwLock_;
};

}
