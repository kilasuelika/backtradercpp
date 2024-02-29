#pragma once
#include <Eigen/Core>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fmt/format.h>

namespace backtradercpp {
namespace analysis {
struct PerformanceMetric {
    double profit = 0;
    double total_r = 0; // return in whole period.
    double r = 0, growth = 0, mdd = 0, sharepe = 0;

    double profit_attr = 0; // Given a total profit X, profit/X.
    double return_attr = 0; // Given a initial portfolio value X, profit/X.

    void print() const;
};
// V is a column vector
template <typename T> inline auto MDD(const Eigen::EigenBase<T> &_v) {
    const T &v(_v.derived());
    // ???瑚?蝐餃?
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

    if ((wealth.abs() < 1e-5).sum()) {
        fmt::print(
            fmt::fg(fmt::color::red),
            "Some wealths are very close to 0, results of performance may be meaningless.\n");
    }

    int N = wealth.size();

    double profit = *(wealth.end() - 1) - *(wealth.begin());
    double profit_r = *(wealth.end() - 1) / *(wealth.begin()) - 1;

    rv = wealth.bottomRows(N - 1) / wealth.topRows(N - 1) - 1;

    double r = rv.mean() * 250;
    double growth = std::numeric_limits<double>::quiet_NaN();
    if (*wealth.begin() > 0) {
        growth = std::log((*(wealth.end() - 1)) / (*wealth.begin())) / (N / 250.0);
    }
    double mdd = MDD(_wealth);
    double sharepe = rv.mean() / std::sqrt(rv.array().pow(2).mean() - std::pow(rv.mean(), 2));

    return PerformanceMetric{profit, profit_r, growth, mdd, sharepe};
}

inline void PerformanceMetric::print() const {
    fmt::print("{: ^13} : {:7.3f}\n", "Yearly return", r);
    fmt::print("{: ^13} : {:7.3f}\n", "Yearly growth", growth);
    fmt::print("{: ^13} : {:7.3f}\n", "MDD", mdd);
    fmt::print("{: ^13} : {:7.3f}\n", "Sharepe", sharepe);
}

class TotalValueAnalyzer {
  public:
    TotalValueAnalyzer() {}
    void update_total_value(double total_value);
    const auto &total_value_history() const { return total_value_history_; }

    void reset() { total_value_history_.resize(0); }
    const std::string &name() const { return name_; }
    void set_name(const std::string &name_) { this->name_ = name_; }

  protected:
    VecArrXd total_value_history_;
    std::string name_;
};

void TotalValueAnalyzer::update_total_value(double total_value) {
    int n = total_value_history_.size();
    total_value_history_.conservativeResize(n + 1);
    total_value_history_.coeffRef(n) = total_value;
}

class MetricAnalyzer {
  public:
    void register_TotalValueAnalyzer(const TotalValueAnalyzer *p) {
        total_value_analyzers_.push_back(p);
    }

    void print_table() const;
    void write_table(const std::string &file) const;
    void cal_metrics();
    const auto &performance() const { return performance_; }

    void reset() { total_value_analyzers_.clear(); }

  private:
    std::vector<const TotalValueAnalyzer *> total_value_analyzers_;
    // std::vector<VecArrXd> total_values_;
    std::vector<PerformanceMetric> performance_;
};

inline void MetricAnalyzer::cal_metrics() {
    performance_.clear();
    for (int i = 0; i < total_value_analyzers_.size(); ++i) {
        performance_.emplace_back(
            analysis::cal_performance(total_value_analyzers_[i]->total_value_history()));

        auto &perf = performance_.back();
        perf.profit_attr = perf.profit / performance_[0].profit;
        perf.return_attr =
            perf.profit / *(total_value_analyzers_[0]->total_value_history().begin());
    }
}

inline void MetricAnalyzer::print_table() const {

    fort::char_table tb;
    tb << fort::header << "Metrics"
       << "All";
    if (performance_.size() > 1)
        for (int i = 0; i < performance_.size() - 1; ++i)
            tb << total_value_analyzers_[i + 1]->name();
    tb << fort::endr;

    tb << "Profit    ";
    for (const auto &m : performance_)
        tb << fmt::format("{:8.0f}", m.profit);
    tb << fort::endr;

    tb << "Profit Attr. (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:8.0f}", m.profit_attr * 100);
    tb << fort::endr;

    tb << "Return (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.total_r * 100);
    tb << fort::endr;

    tb << "Return Attr. (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.return_attr * 100);
    tb << fort::endr;

    tb << fort::separator;

    tb << "Yearly return (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.r * 100);
    tb << fort::endr;

    tb << "Yearly growth (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.growth * 100);
    tb << fort::endr;

    tb << "MDD (%)";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.mdd * 100);
    tb << fort::endr;

    tb << "Sharepe Ratio    ";
    for (const auto &m : performance_)
        tb << fmt::format("{:6.3f}", m.sharepe);
    tb << fort::endr;

    for (int i = 0; i <= performance_.size(); ++i) {
        tb.column(i).set_cell_text_align(fort::text_align::right);
    }
    std::cout << tb.to_string();
}

void backtradercpp::analysis::MetricAnalyzer::write_table(const std::string &file) const {
    std::ofstream f(file);

    f << "Metrics, All";
    if (performance_.size() > 1)
        for (int i = 0; i < performance_.size(); ++i)
            f << fmt::format(",{}", i);
    f << std::endl;

    f << "Yearly return(%)";
    for (const auto &m : performance_)
        f << fmt::format(",{}", m.r * 100);
    f << std::endl;

    f << "Yearly growth(%)";
    for (const auto &m : performance_)
        f << fmt::format(",{}", m.growth * 100);
    f << std::endl;

    f << "MDD(%)";
    for (const auto &m : performance_)
        f << fmt::format(",{}", m.mdd * 100);
    f << std::endl;

    f << "Sharepe Ratio";
    for (const auto &m : performance_)
        f << fmt::format(",{}", m.sharepe);
    f << std::endl;
}

} // namespace analysis
} // namespace backtradercpp
