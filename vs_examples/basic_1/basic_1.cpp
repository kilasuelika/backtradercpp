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
    cerebro.add_data(
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::Broker>(0.0005, 0.001), );
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
