#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace arb {

struct PolymarketMarketInfoResponse {
    std::string id;
    std::string question;
    std::string slug;
    std::string outcomes;
    std::string outcomePrices;
    double volumeNum = 0.0;
    double liquidityNum = 0.0;
    bool active = false;
    std::string endDate;
    std::string startDate;
};

struct PolymarketMarketResponse {
    std::optional<std::vector<PolymarketMarketInfoResponse>> markets;
    std::optional<std::string> nextCursor;
};

void from_json(const nlohmann::json& json, PolymarketMarketInfoResponse& response);
void from_json(const nlohmann::json& json, PolymarketMarketResponse& response);

}
