#pragma once

#include <format>
#include <string>
#include <string_view>

namespace arb {

class Logger {
public:
    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        write("INFO", std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        write("ERROR", std::format(fmt, std::forward<Args>(args)...));
    }

private:
    static void write(std::string_view level, const std::string& message);
};

}
