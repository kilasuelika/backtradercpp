#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;
using namespace std;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 5th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::StockBroker(100000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData("../../example_data/CSVDirectory/raw",
                                              "../../example_data/CSVDirectory/adjust",
                                              std::array{2, 3, 5, 6, 4},
                                              feeds::TimeStrConv::delimited_date)
                          .set_code_extractor([](const std::string &code) {
                              return code.substr(code.size() - 13, 9);
                          }))
            .set_xrd_dir("../../example_data/CSVDirectory/xrd", {7, 6, 2, 3, 4}),
        2);
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.set_log_dir("log");
    cerebro.run();

    auto performance = cerebro.performance();
    fmt::print("Sharepe Ratio: {}\n",
               performance[0].sharepe); // index 0 for whole. index 1 for broker 0, e.t.c.
}
