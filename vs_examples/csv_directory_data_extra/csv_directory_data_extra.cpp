#include "../../include/backtradercpp/Cerebro.hpp"
#include <fmt/core.h>
#include <windows.h>
using namespace backtradercpp;

struct EqualWeightStrategy : public strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        // extra_data(0).num(-1, "n");
        if (data_valid(0)) {
            const auto &pct_change = data(0).num("pct_change");
            fmt::print("{} pct_change: {}\n", codes(0)[0], pct_change.coeffRef(0));
        }
        if (data_valid(1)) {
            // fmt::print("code : {} , open: {}\n", codes(1)[0], data(1).close(-1, 0));
            util::cout("code: {}\n", codes(1)[0]);
        }
        // std::abort();
        if (time_index() % 30 == 0) {
            // fmt::print(fmt::fg(fmt::color::yellow), "Adjusting.\n");

            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    // SetConsoleOutputCP(CP_UTF8);
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    fmt::print("Hello, {}!\n", "world");
    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData("../../example_data/CSVDirectory/raw",
                                             "../../example_data/CSVDirectory/adjust",
                                             std::array{2, 3, 5, 6, 4},
                                             feeds::TimeStrConv::delimited_date)
                          .extra_num_col({{7, "pct_change"}})
                          .set_code_extractor([](const std::string &code) {
                              return code.substr(code.size() - 13, 9);
                          })),
        2); // 2 for window
    cerebro.add_broker(broker::BaseBroker(100000, 0.0005, 0.001)
                           .set_feed(feeds::CSVDirPriceData(
                               "../../example_data/CSVDirectory/share_index_future",
                               std::array{2, 3, 5, 6, 4}, feeds::TimeStrConv::delimited_date)),
                       2);
    cerebro.add_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.set_range(date(2015, 6, 1), date(2022, 6, 1));
    cerebro.set_log_dir("log");
    cerebro.run();
    fmt::print("The test number is: {:d}\n", 5);
}

extern "C" {
    __declspec(dllexport) void runBacktrader() {
   Cerebro cerebro;
    // code_extractor: extract code form filename.
    fmt::print("Hello, {}!\n", "world");
    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData("../../example_data/CSVDirectory/raw",
                                             "../../example_data/CSVDirectory/adjust",
                                             std::array{2, 3, 5, 6, 4},
                                             feeds::TimeStrConv::delimited_date)
                          .extra_num_col({{7, "pct_change"}})
                          .set_code_extractor([](const std::string &code) {
                              return code.substr(code.size() - 13, 9);
                          })),
        2); // 2 for window
    cerebro.add_broker(broker::BaseBroker(100000, 0.0005, 0.001)
                           .set_feed(feeds::CSVDirPriceData(
                               "../../example_data/CSVDirectory/share_index_future",
                               std::array{2, 3, 5, 6, 4}, feeds::TimeStrConv::delimited_date)),
                       2);
    cerebro.add_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.set_range(date(2015, 6, 1), date(2022, 6, 1));
    cerebro.set_log_dir("log");
    cerebro.run();
    }
}