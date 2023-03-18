#pragma once
#include "Cerebro.hpp"

namespace backtradercpp {
namespace optim {

struct Range {
    Range(double start, double end) : start(start), end(end) {}
    double start = 0, end = 0;
    auto delta(double d) {
        std::vector<double> res;
        double s = start;
        while (s <= end) {
            res.emplace_back(s);
            s += d;
        }
        return res;
    }
    auto n(double n_) {
        std::vector<double> res(n);
        double d = (end - start) / (n - 1);
        for (int i = 0; i < n - 1; ++i) {
            res[i] = start + i * d;
        }
        res.back() = end;
        return res;
    }
};

static void optimize_strategy(const Cerebro &cerebro,
                              const std::vector<std::pair<std::string, std::vector<double>>> &vars,
                              const std::string &target = "sharpe", bool log = false,
                              int nproc = -1) {}

} // namespace optim
} // namespace backtradercpp
