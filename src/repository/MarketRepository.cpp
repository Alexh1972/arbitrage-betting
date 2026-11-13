#include "repository/MarketRepository.h"

#include "util/Logger.h"

namespace arb {

namespace {

constexpr const char* UPDATE_SQL =
    "UPDATE market SET "
    "question = :question, slug = :slug, outcomes = :outcomes, outcome_prices = :outcome_prices, "
    "volume_num = :volume_num, liquidity_num = :liquidity_num, active = :active, "
    "end_date = :end_date, start_date = :start_date, question_vector = :question_vector "
    "WHERE market_id = :market_id AND platform_id = :platform_id";

constexpr const char* INSERT_SQL =
    "INSERT INTO market "
    "(market_id, platform_id, question, slug, outcomes, outcome_prices, volume_num, liquidity_num, active, end_date, start_date, question_vector) "
    "VALUES (:market_id, :platform_id, :question, :slug, :outcomes, :outcome_prices, :volume_num, :liquidity_num, :active, :end_date, :start_date, :question_vector)";

void bindMarket(Statement& statement, const Market& market) {
    statement.bindString("market_id", market.marketId);
    statement.bindInt("platform_id", market.platform.platformId);
    statement.bindString("question", market.question);
    statement.bindString("slug", market.slug);
    statement.bindString("outcomes", market.outcomes);
    statement.bindString("outcome_prices", market.outcomePrices);
    statement.bindDouble("volume_num", market.volumeNum);
    statement.bindDouble("liquidity_num", market.liquidityNum);
    statement.bindInt("active", market.active ? 1 : 0);
    statement.bindString("end_date", market.endDate);
    statement.bindString("start_date", market.startDate);
    statement.bindBytes("question_vector", market.getQuestionVector());
}

constexpr const char* RECENT_SQL =
    "SELECT m.market_id, m.platform_id, p.name, m.question, m.slug, m.outcomes, m.outcome_prices, "
    "m.volume_num, m.liquidity_num, m.active, m.end_date, m.start_date, m.question_vector "
    "FROM market m JOIN platform p ON p.platform_id = m.platform_id "
    "WHERE m.end_date >= :min_end_date";

}

MarketRepository::MarketRepository(OracleDatabase& database) : database_(database) {}

void MarketRepository::createSchema() {
    Connection connection = database_.acquire();
    connection.execute(
        "BEGIN "
        "EXECUTE IMMEDIATE 'CREATE TABLE market ("
        "market_id VARCHAR2(255 CHAR) NOT NULL, "
        "platform_id NUMBER(19,0) NOT NULL, "
        "question CLOB, "
        "slug VARCHAR2(255 CHAR), "
        "outcomes VARCHAR2(255 CHAR), "
        "outcome_prices VARCHAR2(255 CHAR), "
        "volume_num FLOAT(53) NOT NULL, "
        "liquidity_num FLOAT(53) NOT NULL, "
        "active NUMBER(1,0) NOT NULL CHECK (active IN (0,1)), "
        "end_date VARCHAR2(255 CHAR), "
        "start_date VARCHAR2(255 CHAR), "
        "question_vector BLOB, "
        "PRIMARY KEY (market_id, platform_id), "
        "FOREIGN KEY (platform_id) REFERENCES platform)'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; "
        "END;");
    connection.execute(
        "BEGIN "
        "EXECUTE IMMEDIATE 'CREATE INDEX idx_market_platform ON market (platform_id, market_id)'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE NOT IN (-955, -1408) THEN RAISE; END IF; "
        "END;");
}

void MarketRepository::save(Connection& connection, const Market& market) {
    Statement update(connection, UPDATE_SQL);
    bindMarket(update, market);
    update.execute();

    if (update.rowCount() == 0) {
        Statement insert(connection, INSERT_SQL);
        bindMarket(insert, market);
        insert.execute();
    }
}

void MarketRepository::save(const Market& market) {
    Connection connection = database_.acquire();
    save(connection, market);
    connection.commit();
}

void MarketRepository::saveAll(const std::vector<Market>& markets) {
    Connection connection = database_.acquire();
    for (const Market& market : markets) {
        try {
            save(connection, market);
            connection.commit();
        } catch (const std::exception& e) {
            connection.rollback();
            Logger::error("Error while saving market {}: {}", market.marketId, e.what());
        }
    }
}

std::vector<Market> MarketRepository::getRecentMarkets(const std::string& then) {
    Connection connection = database_.acquire();
    Statement statement(connection, RECENT_SQL);
    statement.bindString("min_end_date", then);
    statement.execute();
    statement.defineInt(2);
    statement.defineLongString(4);
    statement.defineDouble(8);
    statement.defineDouble(9);
    statement.defineInt(10);
    statement.defineLongBytes(13);

    std::vector<Market> markets;
    while (statement.fetch()) {
        Market market;
        market.marketId = statement.getString(1);
        market.platform = Platform(statement.getInt(2), statement.getString(3));
        market.question = statement.getString(4);
        market.slug = statement.getString(5);
        market.outcomes = statement.getString(6);
        market.outcomePrices = statement.getString(7);
        market.volumeNum = statement.getDouble(8);
        market.liquidityNum = statement.getDouble(9);
        market.active = statement.getInt(10) != 0;
        market.endDate = statement.getString(11);
        market.startDate = statement.getString(12);
        market.setQuestionVector(statement.getBytes(13));
        markets.push_back(std::move(market));
    }

    return markets;
}

}
