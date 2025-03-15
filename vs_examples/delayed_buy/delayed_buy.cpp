#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct BuyLowStrategy : strategy::GenericStrategy {
    void run() override {
        // Do nothing at initial 30 days.
        if (time_index() < 30) {
            return;
        }
        // If daily return larger than 0.05, then buy.
        for (int j = 0; j < data(0).assets(); ++j) {
            if (data(0).valid(-1, j)) {
                double p = data(0).close(-1, j), old_p = data(0).close(-2, j);
                if ((old_p > 0) && ((p / old_p) > 1.05))
                    // Issue an order of buy at the price of open on next day.
                    delayed_buy(0, j, EvalOpen::exact(), 10);
            }
        }
        // Sell on broker 0 if profits or loss reaching target.
        // Price is open of next day.
        for (const auto &[asset, item] : portfolio_items(0)) {
            if (item.profit > 1500 || item.profit < -1000) {
                close(0, asset, EvalOpen::exact());
            }
        }
    }
};

int main() {
    Cerebro cerebro;

    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)),
        2); // 2 for window
    cerebro.add_strategy(std::make_shared<BuyLowStrategy>());
    cerebro.run();
}
