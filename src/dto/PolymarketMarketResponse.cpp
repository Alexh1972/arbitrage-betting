#include "dto/PolymarketMarketResponse.h"

#include "dto/JsonUtil.h"

namespace arb {

void from_json(const nlohmann::json& json, PolymarketMarketInfoResponse& response) {
    response.id = JsonUtil::getString(json, "id");
    response.question = JsonUtil::getString(json, "question");
    response.slug = JsonUtil::getString(json, "slug");
    response.outcomes = JsonUtil::getString(json, "outcomes");
    response.outcomePrices = JsonUtil::getString(json, "outcomePrices");
    response.volumeNum = JsonUtil::getDouble(json, "volumeNum");
    response.liquidityNum = JsonUtil::getDouble(json, "liquidityNum");
    response.active = JsonUtil::getBool(json, "active");
    response.endDate = JsonUtil::getString(json, "endDate");
    response.startDate = JsonUtil::getString(json, "startDate");
}

void from_json(const nlohmann::json& json, PolymarketMarketResponse& response) {
    const auto markets = json.find("markets");
    if (markets != json.end() && !markets->is_null()) {
        response.markets = markets->get<std::vector<PolymarketMarketInfoResponse>>();
    }
    response.nextCursor = JsonUtil::getOptionalString(json, "next_cursor");
}

}
