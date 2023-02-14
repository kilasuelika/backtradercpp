#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;

    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets("option"));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target("option", target_C);
        }
        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets("stock"));

            // Accessing read common data.
            double sigma = common_data("option").num(-1, "sigma");
            double K = common_data("option").num(-1, "K");
            double rf = common_data("option").num(-1, "rf");

            for (int i = 0; i < assets("stock"); ++i) {
                double S = data("stock").close(-1, i);
                double T = common_data("option").num(-1, "time");

                double d1 =
                    (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                double delta = boost::math::cdf(boost::math::normal(), d1);

                target_S.coeffRef(i) = -int(delta * 100);
            }

            adjust_to_volume_target("stock", target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(broker::BaseBroker(0).allow_default().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Option.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("option")),
                       window);
    // Stock price
    cerebro.add_broker(broker::BaseBroker(0).allow_short().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Stock.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("stock")),
                       window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonDataFeed("../../example_data/Option/OptionInfo.csv",
                                                     feeds::TimeStrConv::delimited_date)
                                .set_name("option"),
                            window);

    cerebro.add_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.set_verbose(OnlySummary);
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
    double profit0 =
        cerebro.performance()[0]
            .profit; // index 0 for performance of overall portfolio. Index 1 is for the 0th broker.

    // Run once more.
    cerebro.reset();
    cerebro.broker("stock").set_commission_rate(0.001, 0.001);
    cerebro.set_log_dir("log1");
    cerebro.run();
    double profit1 = cerebro.performance()[0].profit;

    fmt::print("Profits under 0 and 0.001 commission rate: {}, {}\n", profit0, profit1);
}
