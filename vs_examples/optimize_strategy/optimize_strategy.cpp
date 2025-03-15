#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
#include "../../include/backtradercpp/Optimizer.hpp"
using namespace backtradercpp;
using namespace std;

/**
 * Buy stock if it's at low price.
 */
struct BuyLowStrategy : strategy::GenericStrategy {
    void set_param(const std::vector<std::pair<std::string, double>>& param) override
    {
        for (const auto& it : param)
        {
            if (it.first == "Q1")
            {
                Q1 = it.second;
            }
        }
        // Remember to reset state.
        in_stock = false;
    }
    double Q1 = 100;

    bool in_stock = false;
    void run() override {

        std::cout << data(0).close(-1, 0) << std::endl;

        if (!in_stock)
        {
            for (int i = 0; i < assets(0); ++i)
            {
                if (data(0).close(-1, i) < Q1)
                {
                    // std::print(std::cout, "close: {}\n", data(0).close(-1, i));

                    buy(0, i, data(0).close(-1, i), 100);
                    in_stock = true;
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
        broker::BaseBroker(10000000).disable_price_check()
        .set_feed(feeds::RandomProcessData(100, GeometricBrownianMotionProcess<double>(0, 1, 100)).set_dump_csv("GBM.csv")));
    cerebro.add_strategy(std::make_shared<BuyLowStrategy>());

    optim::TableRunner runner;
    auto results = runner.run(cerebro, { {"Q1",{80.0,90.0}} });
}
