#include "util/Logger.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace arb {

void Logger::write(std::string_view level, const std::string& message) {
    static std::mutex mutex;

    const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    std::ostringstream threadId;
    threadId << std::this_thread::get_id();

    const std::string line = std::format("{:%Y-%m-%dT%H:%M:%S}Z {:>5} [{}] {}\n", now, level, threadId.str(), message);

    std::lock_guard lock(mutex);
    auto& out = level == "ERROR" ? std::cerr : std::cout;
    out << line;
    out.flush();
}

}
