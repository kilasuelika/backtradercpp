#pragma once
#include <filesystem>
#include <string>
#include <iostream>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/functional/hash.hpp>

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
        throw std::range_error("File " + path + " doesn't exists.");
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

// inline std::string to_string(const boost::posix_time::ptime &t) {
//     // std::cout << t << std::endl;
//     return fmt::format("{:04}-{:02}-{:02} {:02}-{:02}-{:02}", t.date().year(), t.date().month(),
//                        t.date().day(), t.time_of_day().hours(), t.time_of_day().minutes(),
//                        t.time_of_day().seconds());
// }
inline std::string to_string(const boost::posix_time::ptime &t) {
    return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
                       static_cast<int>(t.date().year()), 
                       static_cast<int>(t.date().month()), 
                       static_cast<int>(t.date().day()), 
                       static_cast<int>(t.time_of_day().hours()), 
                       static_cast<int>(t.time_of_day().minutes()), 
                       static_cast<int>(t.time_of_day().seconds()));
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

auto delimited_to_date(const std::string &s) {
    return boost::gregorian::date(std::stoi(s.substr(0, 4)), std::stoi(s.substr(5, 2)),
                                  std::stoi(s.substr(8, 2)));
}

template <typename T> void reset_ifstream(T &f) {
    f.clear();
    f.seekg(0, std::ios::beg);
}

template <typename T1, typename T2> void update_map(T1 &dest, const T2 &src) {
    for (const auto &[k, v] : src) {
        dest[k] = v;
    }
}
} // namespace util
} // namespace backtradercpp

namespace std {

// Note: This is pretty much the only time you are allowed to
// declare anything inside namespace std!
template <> struct hash<boost::gregorian::date> {
    size_t operator()(const boost::gregorian::date &date) const {
        return std::hash<decltype(date.julian_day())>()(date.julian_day());
    }
};

template <> struct hash<boost::posix_time::ptime> {
    size_t operator()(const boost::posix_time::ptime &time) const {
        std::hash<long> hasher;
        std::size_t seed = 0;
        boost::hash_combine(seed, hasher(time.date().julian_day()));
        boost::hash_combine(seed, hasher(time.time_of_day().total_seconds()));
        return seed;
    }
};

} // namespace std



