#pragma once
#include <filesystem>
#include <string>
#include <iostream>

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
    for (auto &[k,v] : m) {
        v.resize(n);
    }
}

template <typename T, typename T2> void reset_value(T &m, const T2 &v_) {
    for (auto &[k, v] : m) {
        std::fill(v.begin(), v.end(), v_);
    }
}
} // namespace util
} // namespace backtradercpp