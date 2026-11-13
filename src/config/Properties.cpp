#include "config/Properties.h"

#include <fstream>
#include <stdexcept>

namespace arb {

namespace {

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

}

Properties Properties::load(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open configuration file " + path.string());
    }

    Properties properties;
    properties.baseDir_ = std::filesystem::absolute(path).parent_path();

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == '!') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        properties.values_[trim(line.substr(0, separator))] = trim(line.substr(separator + 1));
    }

    return properties;
}

std::string Properties::get(const std::string& key, const std::string& defaultValue) const {
    const auto it = values_.find(key);
    return it == values_.end() ? defaultValue : it->second;
}

long long Properties::getLong(const std::string& key, long long defaultValue) const {
    const auto it = values_.find(key);
    return it == values_.end() ? defaultValue : std::stoll(it->second);
}

std::filesystem::path Properties::getPath(const std::string& key, const std::string& defaultValue) const {
    std::filesystem::path path = get(key, defaultValue);
    return path.is_absolute() ? path : baseDir_ / path;
}

}
