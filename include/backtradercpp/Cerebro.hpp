#pragma once
/*ZhouYao at 2022-09-10*/

#include "DataFeeds.hpp"
#include "BaseBrokerImpl.hpp"
#include "Strategy.hpp"
#define SPDLOG_FMT_EXTERNAL
#include <spdlog/stopwatch.h>

// #include<chrono>
namespace backtradercpp {

class Cerebro {
  public:
    // window is for strategy. DataFeed and Broker doesn't store history data.
    void add_broker(broker::BaseBroker broker, int window = 1);
    void add_common_data(feeds::GenericCommonDataFeed data, int window);

    // void init_feeds_aggrator_();
    void set_strategy(std::shared_ptr<strategy::GenericStrategy> strategy);
    void init_strategy();
    void set_range(const date &start, const date &end = date(boost::date_time::max_date_time));
    // Set a directory for logging.
    void set_log_dir(const std::string &dir);

    void run();
    const std::vector<PriceFeedDataBuffer> &datas() const { return price_feeds_agg_.datas(); }

    const auto &performance() const { return broker_agg_.performance(); }

  private:
    // std::vector<int> asset_broker_map_
    feeds::PriceFeedAggragator price_feeds_agg_;
    feeds::CommonFeedAggragator common_feeds_agg_;

    broker::BrokerAggragator broker_agg_;
    std::shared_ptr<strategy::GenericStrategy> strategy_;
    // strategy::FullAssetData data_;
    ptime start_{boost::posix_time::min_date_time}, end_{boost::posix_time::max_date_time};
};

void Cerebro::add_broker(broker::BaseBroker broker, int window) {
    price_feeds_agg_.add_feed(broker.feed());
    price_feeds_agg_.set_window(price_feeds_agg_.datas().size() - 1, window);
    broker_agg_.add_broker(broker);
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
            broker_agg_.process_terms();
            broker_agg_.update_info();

            fmt::print("cash: {:12.4f},  total_wealth: {:12.2f}\n", broker_agg_.total_cash(),
                       broker_agg_.total_wealth());
            fmt::print("Using {} seconds.\n", util::sw_to_seconds(sw));
        }
    }
    broker_agg_.summary();
}

void Cerebro::set_log_dir(const std::string &dir) {
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    broker_agg_.set_log_dir(dir);
}
}; // namespace backtradercpp
