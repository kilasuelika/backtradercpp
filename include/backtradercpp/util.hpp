#pragma once
#include <filesystem>
#include <string>
#include <sstream>
#include <set>
#include <iostream>

namespace backtradercpp {
namespace util {
std::string absolute_path(const std::string &path) {
    return std::filesystem::absolute(std::filesystem::path(path)).string();
}

void check_path_exists(const std::string &path) {
    if (!std::filesystem::exists(path)) {
        std::cout << "File " + path + " not exists." << std::endl;
        std::cout << "Absolute path " + absolute_path(path) << std::endl;
        throw std::range_error("File " + path + " not exists.");
    };
};
} // namespace util
} // namespace backtradercpp