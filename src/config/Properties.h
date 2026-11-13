#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace arb {

class Properties {
public:
    static Properties load(const std::filesystem::path& path);

    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    long long getLong(const std::string& key, long long defaultValue) const;
    std::filesystem::path getPath(const std::string& key, const std::string& defaultValue = "") const;

private:
    std::unordered_map<std::string, std::string> values_;
    std::filesystem::path baseDir_;
};

}
