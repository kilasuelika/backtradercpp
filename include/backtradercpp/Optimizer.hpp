#pragma once
#include "Cerebro.hpp"

namespace backtradercpp {
    namespace optim {
        /* struct Range {
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
         };*/


         /**
          * run a strategy multiple times with different parameters
          **/
        struct TableRunResult
        {
            std::vector<std::pair<std::string, double>> parameters;
            std::vector<analysis::PerformanceMetric> performance;
        };
        struct TableRunResults
        {
            std::vector<TableRunResult> data;
        };
        class TableRunner
        {
        public:
            TableRunResults run(Cerebro& cerebro, const std::vector<std::pair<std::string, std::vector<double>>>& vars, bool log = false)
            {
                TableRunResults results;
                auto parameter_sets = GenerateCartesianProduct(vars);
                cerebro.push_state();
                for (const auto& p : parameter_sets)
                {
                    cerebro.reset();
                    cerebro.pop_state(false);
                    auto result = run(cerebro, p, log);
                    results.data.emplace_back(result);
                }
                return results;

            }
            std::vector<double> generate_range(double start, double d, double end)
            {
                std::vector<double> results{ start };
                while (results.back() < end)
                {
                    results.push_back(results.back() + d);
                }
                return results;
            }
            TableRunResult run(Cerebro& cerebro, const std::vector<std::pair<std::string, double>>& param, bool log = false)
            {
                TableRunResult result;
                result.parameters = param;
                cerebro.set_strategy_param(param);
                cerebro.run();
                result.performance = cerebro.performance();
                return result;
            }

            static void GenerateCartesianProductRecursive(
                const std::vector<std::pair<std::string, std::vector<double>>>& vars,
                size_t depth,
                std::vector<std::pair<std::string, double>>& current,
                std::vector<std::vector<std::pair<std::string, double>>>& result)
            {
                if (depth == vars.size()) {
                    result.push_back(current);
                    return;
                }

                const auto& [varName, values] = vars[depth];
                for (double value : values) {
                    current[depth] = { varName, value };
                    GenerateCartesianProductRecursive(vars, depth + 1, current, result);
                }
            }

            static std::vector<std::vector<std::pair<std::string, double>>> GenerateCartesianProduct(const std::vector<std::pair<std::string, std::vector<double>>>& vars)
            {
                std::vector<std::vector<std::pair<std::string, double>>> result;
                if (vars.empty()) return result;

                std::vector<std::pair<std::string, double>> current(vars.size());
                GenerateCartesianProductRecursive(vars, 0, current, result);

                return result;
            }


        };
    } // namespace optim
} // namespace backtradercpp
