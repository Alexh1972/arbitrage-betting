#include "service/ApiService.h"

#include "util/ThreadUtil.h"
#include "util/TimeUtil.h"

namespace arb {

HttpResponse ApiService::publish(const std::string& name, const std::string& url, long long maxWait) {
    long long sleepTime;
    {
        std::lock_guard lock(mutex_);
        const auto it = timestampMap_.find(name);
        const long long time = it == timestampMap_.end() ? 0 : it->second;
        const long long now = TimeUtil::nowEpochMillis();

        sleepTime = time + maxWait - now;
        if (sleepTime < 0) {
            sleepTime = 0;
        }

        timestampMap_[name] = now + sleepTime;
    }

    ThreadUtil::sleep(sleepTime);

    return HttpClient::get(url, {{"Accept", "application/json"}});
}

}
