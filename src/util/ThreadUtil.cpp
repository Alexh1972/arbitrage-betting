#include "util/ThreadUtil.h"

#include <chrono>
#include <thread>

namespace arb::ThreadUtil {

void sleep(long long millis) {
    if (millis > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }
}

}
