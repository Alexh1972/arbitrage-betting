#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace arb {

struct KalshiMarketInfoResponse {
    std::string id;
    std::string question;
    std::string slug;
    double volumeNum = 0.0;
    double liquidityNum = 0.0;
    std::string startDate;
    std::string endDate;
    std::optional<std::string> yesAsk;
    std::optional<std::string> noAsk;
};

struct KalshiMarketResponse {
    std::optional<std::vector<KalshiMarketInfoResponse>> markets;
    std::optional<std::string> cursor;
};

void from_json(const nlohmann::json& json, KalshiMarketInfoResponse& response);
void from_json(const nlohmann::json& json, KalshiMarketResponse& response);

}
