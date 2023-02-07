#pragma once
/*ZhouYao at 2022-09-10*/

#include "DataFeeds.hpp"
#include "BrokerImpl.hpp"
#include "Strategy.hpp"
#define SPDLOG_FMT_EXTERNAL
#include <spdlog/stopwatch.h>

// #include<chrono>
namespace backtradercpp {

class Cerebro {
  public:
    void add_asset_data(feeds::GenericPriceDataFeed data, broker::Broker broker, int window = 1);
    void add_common_data(feeds::GenericCommonDataFeed data, int window);

    // void init_feeds_aggrator_();
    void set_strategy(std::shared_ptr<strategy::GenericStrategy> strategy);
    void init_strategy();
    void set_range(const date &start, const date &end = date(boost::date_time::max_date_time));

    void run();
    const std::vector<PriceFeedDataBuffer> &datas() const { return price_feeds_agg_.datas(); }

  private:
    // std::vector<int> asset_broker_map_
    feeds::PriceFeedAggragator price_feeds_agg_;
    feeds::CommonFeedAggragator common_feeds_agg_;

    broker::BrokerAggragator broker_agg_;
    std::shared_ptr<strategy::GenericStrategy> strategy_;
    // strategy::FullAssetData data_;
    ptime start_{boost::posix_time::min_date_time}, end_{boost::posix_time::max_date_time};
};

void Cerebro::add_asset_data(feeds::GenericPriceDataFeed data, broker::Broker broker, int window) {
    price_feeds_agg_.add_feed(data);
    price_feeds_agg_.set_window(price_feeds_agg_.datas().size() - 1, window);

    broker.resize(data.assets());
    broker.sp->current_ = data.data_ptr();

    broker_agg_.add_broker(broker);

    // broker->
}
void Cerebro::add_common_data(feeds::GenericCommonDataFeed data, int window) {
    common_feeds_agg_.add_feed(data);
    common_feeds_agg_.set_window(common_feeds_agg_.datas().size() - 1, window);
};

void Cerebro::set_strategy(std::shared_ptr<strategy::GenericStrategy> strategy) {
    strategy_ = strategy;
}

void Cerebro::init_strategy() {
    strategy_->init_strategy(&price_feeds_agg_, &common_feeds_agg_, &broker_agg_);
}

inline void Cerebro::set_range(const date &start, const date &end) {
    start_ = ptime(start);
    end_ = ptime(end);
}

void Cerebro::run() {
    fmt::print(fmt::fg(fmt::color::yellow), "Runnng strategy..\n");
    init_strategy();

    while (!price_feeds_agg_.finished()) {
        spdlog::stopwatch sw;

        if ((!price_feeds_agg_.read()) || (price_feeds_agg_.time() > end_))
            break;
        common_feeds_agg_.read();
        if (price_feeds_agg_.time() >= start_) {
            fmt::print(fmt::runtime("┌{0:─^{2}}┐\n"
                                    "│{1: ^{2}}│\n"
                                    "└{0:─^{2}}┘\n"),
                       "", util::to_string(price_feeds_agg_.time()), 21);

            // fmt::print("{}\n", util::to_string(feeds_agg_.time()));
            broker_agg_.process_old_orders();
            auto order_pool = strategy_->execute();
            broker_agg_.process(order_pool);
            broker_agg_.update_info();

            fmt::print("cash: {:12.4f},  total_wealth: {:12.2f}\n", broker_agg_.total_cash(),
                       broker_agg_.total_wealth());
            fmt::print("Using {} seconds.\n", util::sw_to_seconds(sw));
        }
        // std::cout << "close:" << feeds_agg_.data(0).close().transpose() << std::endl;
        // std::cout << "adj_close:" << feeds_agg_.data(0).adj_close().transpose() << std::endl;
        // std::cout << "position: " << broker_agg_.positions(0).transpose() << std::endl;
        //// std::cout << "profits: " << broker_agg_.profits(0).transpose() << std::endl;
        // std::cout << "adj_profits: " << broker_agg_.adj_profits(0).transpose() << std::endl;
        //// std::cout << "values: " << broker_agg_.values(0).transpose() << std::endl;
        //// std::cout << "dyn_adj_profits: "
        ////           <<
        /// broker_agg_.portfolio(0).dyn_adj_profits(broker_agg_.assets(0)).transpose() / <<
        /// std::endl;

        // auto wv = broker_agg_.wealth_history();
        // int N = wv.size();
        // if (N > 2) {
        //     if (wv.coeff(N - 1) / wv.coeff(N - 2) > 1.1) {
        //         fmt::print(fmt::fg(fmt::color::red), "Abnormal returns.\n");
        //     }
        // }

        // for (const auto &item : broker_agg_.portfolio(0).portfolio_items) {
        //     fmt::print("asset: {}, value: {}, profit: {}\n", item.first, item.second.value,
        //                item.second.profit);
        // }
    }
    broker_agg_.summary();
}
}; // namespace backtradercpp
