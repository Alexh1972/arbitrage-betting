#include "config/PlatformInit.h"
#include "config/Properties.h"
#include "db/OracleDatabase.h"
#include "embedding/EmbeddingModel.h"
#include "http/HttpClient.h"
#include "repository/MarketRepository.h"
#include "repository/PlatformRepository.h"
#include "scheduler/KalshiMarketHandler.h"
#include "scheduler/PolymarketHandler.h"
#include "scheduler/Scheduler.h"
#include "service/ApiService.h"
#include "service/MarketService.h"
#include "service/PlatformService.h"
#include "util/Logger.h"

#include <exception>

int main(int argc, char** argv) {
    using namespace arb;

    try {
        const Properties properties = Properties::load(argc > 1 ? argv[1] : "application.properties");
        Logger::info("Starting {}", properties.get("application.name", "arbitrage-betting"));

        HttpClient::globalInit();

        DataSourceConfig dataSource;
        dataSource.host = properties.get("datasource.host", "localhost");
        dataSource.port = static_cast<int>(properties.getLong("datasource.port", 1521));
        dataSource.sid = properties.get("datasource.sid", "orcl");
        dataSource.username = properties.get("datasource.username");
        dataSource.password = properties.get("datasource.password");
        dataSource.poolSize = static_cast<unsigned int>(properties.getLong("datasource.pool-size", 50));

        OracleDatabase database(dataSource);

        PlatformRepository platformRepository(database);
        MarketRepository marketRepository(database);
        platformRepository.createSchema();
        marketRepository.createSchema();

        PlatformService platformService(platformRepository);
        MarketService marketService(marketRepository);
        ApiService apiService;

        PlatformInit(platformService).run();

        const EmbeddingModel embeddingModel(
            properties.getPath("embedding.model", "models/bge-small-en-v1.5-q.onnx"),
            properties.getPath("embedding.tokenizer", "models/bge-small-en-v1.5-tokenizer.json"));

        PolymarketHandler polymarketHandler(platformService, marketService, apiService, embeddingModel);
        KalshiMarketHandler kalshiMarketHandler(platformService, marketService, apiService, embeddingModel);

        const std::chrono::milliseconds fixedDelay(properties.getLong("scheduler.fixed-delay-ms", 2000));

        Scheduler scheduler;
        scheduler.scheduleWithFixedDelay("polymarket", [&] { polymarketHandler.read(); }, fixedDelay);
        scheduler.scheduleWithFixedDelay("kalshi", [&] { kalshiMarketHandler.read(); }, fixedDelay);
        scheduler.join();

        HttpClient::globalCleanup();
    } catch (const std::exception& e) {
        Logger::error("Application failed: {}", e.what());
        return 1;
    }

    return 0;
}
