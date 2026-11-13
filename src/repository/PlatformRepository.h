#pragma once

#include "db/OracleDatabase.h"
#include "model/Platform.h"

#include <optional>
#include <string>

namespace arb {

class PlatformRepository {
public:
    explicit PlatformRepository(OracleDatabase& database);

    void createSchema();
    std::optional<Platform> getPlatformByName(const std::string& name);
    Platform save(const Platform& platform);

private:
    OracleDatabase& database_;
};

}
