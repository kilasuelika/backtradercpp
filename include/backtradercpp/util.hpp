#pragma once
#include <filesystem>
#include <string>
#include <iostream>
#include <boost/date_time.hpp>
#include <format>
#include <fmt/format.h>

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/stopwatch.h>

namespace backtradercpp {
namespace util {
inline std::string absolute_path(const std::string &path) {
    return std::filesystem::absolute(std::filesystem::path(path)).string();
}

inline void check_path_exists(const std::string &path) {
    if (!std::filesystem::exists(path)) {
        std::cout << "File " + path + " not exists." << std::endl;
        std::cout << "Absolute path " + absolute_path(path) << std::endl;
        throw std::range_error("File " + path + " not exists.");
    }
}
template <typename... Args> std::string path_join(Args &&...args) {
    return (std::filesystem::path(args) / ...).string();
}

template <typename T> void resize_value(T &m, int n) {
    for (auto &[k, v] : m) {
        v.resize(n);
    }
}

template <typename T, typename T2> void reset_value(T &m, const T2 &v_) {
    for (auto &[k, v] : m) {
        std::fill(v.begin(), v.end(), v_);
    }
}

inline std::string to_string(const boost::posix_time::ptime &t) {
    std::cout << t << std::endl;
    return fmt::format("{:04}-{:02}-{:02} {:02}-{:02}-{:02}", t.date().year(), t.date().month(),
                       t.date().day(), t.time_of_day().hours(), t.time_of_day().minutes(),
                       t.time_of_day().seconds());
}

inline double sw_to_seconds(const spdlog::stopwatch &sw) {

    return static_cast<double>(
               std::chrono::duration_cast<std::chrono::milliseconds>(sw.elapsed()).count()) /
           1000;
}

template <typename... T> void cout(fmt::format_string<T...> fmt, T &&...args) {
    std::cout << fmt::format(fmt, args...);
}

template <typename T> std::string format_map(const T &m) {
    std::stringstream s;
    s << "{ ";
    for (const auto &[k, v] : m) {
        s << k << ": " << v << ", ";
    }
    std::string res = s.str();
    res.pop_back();
    if (m.size() > 0)
        res.pop_back();
    res += " }";
    return res;
}
} // namespace util
} // namespace backtradercpp