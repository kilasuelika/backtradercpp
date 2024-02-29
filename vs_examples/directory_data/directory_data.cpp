#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct BasicStrategy : public strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.

        // std::abort();
        if (time_index() % 30 == 0) {
            // fmt::print(fmt::fg(fmt::color::yellow), "Adjusting.\n");
        }
    }
};

int main() {
    Cerebro cerebro;
    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData(
                "../../example_data/CSVDirectory/raw", "../../example_data/CSVDirectory/adjust",
                std::array{2, 3, 5, 6, 4}, feeds::TimeStrConv::delimited_date)),
        2); // 2 for window
    cerebro.add_strategy(std::make_shared<BasicStrategy>());
    cerebro.run();
}
