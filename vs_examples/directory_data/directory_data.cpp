#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : public strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.

        // std::abort();
        if (time_index() % 30 == 0) {
            // fmt::print(fmt::fg(fmt::color::yellow), "Adjusting.\n");

            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    Cerebro cerebro;
    cerebro.add_data(feeds::CSVDirectoryData(
                         "../../example_data/CSVDirectory/raw",
                         "../../example_data/CSVDirectory/adjust", std::array{2, 3, 5, 6, 4},
                         feeds::TimeStrConv::delimited_date),
                     broker::Broker(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
