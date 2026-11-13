#pragma once

#include <string>
#include <utility>
#include <vector>

namespace arb {

struct HttpResponse {
    long status = 0;
    std::string body;
};

class HttpClient {
public:
    static void globalInit();
    static void globalCleanup();

    static HttpResponse get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers);
};

}
