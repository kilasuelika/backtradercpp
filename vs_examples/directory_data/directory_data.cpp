#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        if (time_index() % 30 == 0) {
            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    Cerebro cerebro;
    cerebro.add_data(std::make_shared<feeds::CSVDirectoryData>(
                         "../../example_data/CSVDirectory/raw",
                         "../../example_data/CSVDirectory/adjust",
                         std::array<int, 5>{2, 3, 5, 6, 4}, feeds::TimeStrConv::delimited_date),
                     std::make_shared<broker::Broker>(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
