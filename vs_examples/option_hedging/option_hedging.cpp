#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;

    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets(0));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target(0, target_C);
        }

        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets(1));

            // Accessing read common data.
            double sigma = common_data(0).num(-1, "sigma");
            double K = common_data(0).num(-1, "K");
            double rf = common_data(0).num(-1, "rf");

            for (int i = 0; i < assets(1); ++i) {
                double S = data(1).close(-1, i);
                double T = common_data(0).num(-1, "time");

                double d1 =
                    (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                double delta = boost::math::cdf(boost::math::normal(), d1);

                target_S.coeffRef(i) = -int(delta * 100);
            }

            adjust_to_volume_target(1, target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(
        broker::BaseBroker(0).allow_default().set_feed(feeds::CSVTabularData(
            "../../example_data/Option/Option.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Stock price
    cerebro.add_broker(
        broker::BaseBroker(0).allow_short().set_feed(feeds::CSVTabularData(
            "../../example_data/Option/Stock.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonData("../../example_data/Option/OptionInfo.csv",
                                                 feeds::TimeStrConv::delimited_date),
                            window);

    cerebro.set_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
}
