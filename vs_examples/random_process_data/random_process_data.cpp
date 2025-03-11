#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
#include "../../include/backtradercpp/RandomProcessGenerator.hpp"
#include "../../include/backtradercpp/RandomProcessDataFeeds.hpp"
using namespace backtradercpp;
using namespace std;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 6th day. Index starts from 0, so index 5 means 6th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }

        Eigen::VectorXd diff = data(0).close() - data(0).close().mean();  // Element-wise difference
        double variance = (diff.array().square().sum()) / diff.size();  // Variance formula
        fmt::print("Mean value: {}, variance: {}\n", data(0).close().mean(), variance);
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::BaseBroker(0.0005, 0.001)
        .set_feed(feeds::RandomProcessData(100, GeometricBrownianMotionProcess<double>(0, 1, 100)).set_dump_csv("GBM.csv")));
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
