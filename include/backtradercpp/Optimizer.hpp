#pragma once
#include "Cerebro.hpp"

namespace backtradercpp {
namespace optim {

static void optimize_strategy(const Cerebro &cerebro,
                              const std::unordered_map<std::string, int> &int_vars,
                              const std::unordered_map<std::string, int> &double_vars,
                              bool log = true, int nproc = -1) {}

} // namespace optim
} // namespace backtradercpp
