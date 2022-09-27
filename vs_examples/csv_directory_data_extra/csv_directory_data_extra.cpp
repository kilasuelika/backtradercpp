#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : public strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        //extra_data(0).num(-1, "n");
        const auto &pct_chagne = data(0).num("pct_change");
        fmt::print("{} pct_change: {}\n", codes(0)[0], pct_chagne.coeffRef(0));

        // std::abort();
        if (time_index() % 30 == 0) {
            // fmt::print(fmt::fg(fmt::color::yellow), "Adjusting.\n");

            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    Cerebro cerebro;
    //code_extractor: extract code form filename.
    cerebro.add_data(feeds::CSVDirectoryData("../../example_data/CSVDirectory/raw",
                                             "../../example_data/CSVDirectory/adjust",
                                             std::array{2, 3, 5, 6, 4},
                                             feeds::TimeStrConv::delimited_date)
                     .extra_num_col({{7, "pct_change"}})
                     .code_extractor([](const std::string &code) {
                         return code.substr(code.size() - 13, 9);
                     }),
                     broker::Broker(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
