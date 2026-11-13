#include "dto/JsonUtil.h"

#include <stdexcept>

namespace arb::JsonUtil {

std::optional<std::string> getOptionalString(const nlohmann::json& json, const std::string& key) {
    const auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return std::nullopt;
    }
    if (it->is_string()) {
        return it->get<std::string>();
    }
    return it->dump();
}

std::string getString(const nlohmann::json& json, const std::string& key) {
    return getOptionalString(json, key).value_or("");
}

double getDouble(const nlohmann::json& json, const std::string& key) {
    const auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return 0.0;
    }
    if (it->is_number()) {
        return it->get<double>();
    }
    if (it->is_string()) {
        const auto value = it->get<std::string>();
        return value.empty() ? 0.0 : std::stod(value);
    }
    if (it->is_boolean()) {
        return it->get<bool>() ? 1.0 : 0.0;
    }
    throw std::invalid_argument("Field " + key + " is not a number");
}

bool getBool(const nlohmann::json& json, const std::string& key) {
    const auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return false;
    }
    if (it->is_boolean()) {
        return it->get<bool>();
    }
    if (it->is_number()) {
        return it->get<double>() != 0.0;
    }
    if (it->is_string()) {
        return it->get<std::string>() == "true";
    }
    throw std::invalid_argument("Field " + key + " is not a boolean");
}

}
