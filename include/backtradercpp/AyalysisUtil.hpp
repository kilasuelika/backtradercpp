#pragma once
#include <Eigen/Core>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fmt/format.h>

namespace backtradercpp {
namespace analysis {
struct PerformanceMetric {
    double r = 0, growth = 0, mdd = 0;
    void print() const;
};
// V is a column vector
template <typename T> inline auto MDD(const Eigen::EigenBase<T> &_v) {
    const T &v(_v.derived());
    //取得具体类型
    typename T::PlainObject temp;

    return std::accumulate(
        v.begin(), v.end(), 0.0,
        [max_v = double(v.coeff(0))](const double &old_mdd, const double &b) mutable {
            double mdd = (max_v - b) / max_v;
            max_v = std::max(max_v, b);
            return std::max(old_mdd, mdd);
        });
}
template <typename T> inline auto cal_performance(const Eigen::EigenBase<T> &_wealth) {
    const T &wealth(_wealth.derived());
    typename T::PlainObject rv;

    int N = wealth.size();
    rv = wealth.bottomRows(N - 1) / wealth.topRows(N - 1) - 1;

    double r = rv.mean() * 250;
    double growth = std::log((*(wealth.end() - 1)) / (*wealth.begin())) / (N / 250.0);
    double mdd = MDD(_wealth);

    return PerformanceMetric{r, growth, mdd};
}

inline void PerformanceMetric::print() const {
    fmt::print("{: ^13} : {:7.3f}\n", "Yearly return", r);
    fmt::print("{: ^13} : {:7.3f}\n", "Yearly growth", growth);
    fmt::print("{: ^13} : {:7.3f}\n\n", "MDD", mdd);
}
} // namespace analysis
} // namespace backtradercpp