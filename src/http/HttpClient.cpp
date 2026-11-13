#include "http/HttpClient.h"

#include <curl/curl.h>

#include <memory>
#include <stdexcept>

namespace arb {

namespace {

size_t writeCallback(char* data, size_t size, size_t count, void* userData) {
    auto* body = static_cast<std::string*>(userData);
    body->append(data, size * count);
    return size * count;
}

struct CurlDeleter {
    void operator()(CURL* curl) const { curl_easy_cleanup(curl); }
};

struct HeaderListDeleter {
    void operator()(curl_slist* list) const { curl_slist_free_all(list); }
};

}

void HttpClient::globalInit() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void HttpClient::globalCleanup() {
    curl_global_cleanup();
}

HttpResponse HttpClient::get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
    if (!curl) {
        throw std::runtime_error("Cannot initialize HTTP client");
    }

    curl_slist* rawList = nullptr;
    for (const auto& [name, value] : headers) {
        rawList = curl_slist_append(rawList, (name + ": " + value).c_str());
    }
    std::unique_ptr<curl_slist, HeaderListDeleter> headerList(rawList);

    HttpResponse response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headerList.get());
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, 120L);
    curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response.body);

    const CURLcode code = curl_easy_perform(curl.get());
    if (code != CURLE_OK) {
        throw std::runtime_error(std::string("HTTP request failed: ") + curl_easy_strerror(code));
    }

    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response.status);
    return response;
}

}
