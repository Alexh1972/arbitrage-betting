#pragma once

#include "http/HttpClient.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace arb {

class ApiService {
public:
    HttpResponse publish(const std::string& name, const std::string& url, long long maxWait);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, long long> timestampMap_;
};

}
