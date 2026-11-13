#include "dto/KalshiMarketResponse.h"

#include "dto/JsonUtil.h"

namespace arb {

void from_json(const nlohmann::json& json, KalshiMarketInfoResponse& response) {
    response.id = JsonUtil::getString(json, "ticker");
    response.question = JsonUtil::getString(json, "title");
    response.slug = JsonUtil::getString(json, "event_ticker");
    response.volumeNum = JsonUtil::getDouble(json, "volume_fp");
    response.liquidityNum = JsonUtil::getDouble(json, "liquidity_dollars");
    response.startDate = JsonUtil::getString(json, "open_time");
    response.endDate = JsonUtil::getString(json, "close_time");
    response.yesAsk = JsonUtil::getOptionalString(json, "yes_ask_dollars");
    response.noAsk = JsonUtil::getOptionalString(json, "no_ask_dollars");
}

void from_json(const nlohmann::json& json, KalshiMarketResponse& response) {
    const auto markets = json.find("markets");
    if (markets != json.end() && !markets->is_null()) {
        response.markets = markets->get<std::vector<KalshiMarketInfoResponse>>();
    }
    response.cursor = JsonUtil::getOptionalString(json, "cursor");
}

}
