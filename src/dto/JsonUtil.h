#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace arb::JsonUtil {

std::optional<std::string> getOptionalString(const nlohmann::json& json, const std::string& key);
std::string getString(const nlohmann::json& json, const std::string& key);
double getDouble(const nlohmann::json& json, const std::string& key);
bool getBool(const nlohmann::json& json, const std::string& key);

}
