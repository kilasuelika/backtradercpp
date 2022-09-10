#pragma once
/*ZhouYao at 2022-09-10*/

#include "DataFeeds.hpp"
#include "Broker.hpp"
#include "Strategy.hpp"
namespace backtradercpp {

class Cerebro {
  public:
    void add_data(std::shared_ptr<feeds::GenericData> data, std::shared_ptr<broker::Broker> broker);
    // void init_feeds_aggrator_();
    void set_strategy(std::shared_ptr<strategy::GenericStrategy> strategy);
    void init_strategy();

    void run();
    const std::vector<FullAssetData> &datas() const { return feeds_agg_.datas(); }

  private:
    // std::vector<int> asset_broker_map_
    feeds::FeedsAggragator feeds_agg_;
    broker::BrokerAggragator broker_agg_;
    std::shared_ptr<strategy::GenericStrategy> strategy_;
    // strategy::FullAssetData data_;
};

void Cerebro::add_data(std::shared_ptr<feeds::GenericData> data,
                       std::shared_ptr<broker::Broker> broker) {
    feeds_agg_.add_feed(data);
    broker_agg_.add_broker(broker);

    broker->resize(data->assets());
    broker->current_ = data->data_ptr();
    // broker->
}
void Cerebro::set_strategy(std::shared_ptr<strategy::GenericStrategy> strategy) {
    strategy_ = strategy;
}
void Cerebro::init_strategy() { strategy_->init_strategy(&feeds_agg_, &broker_agg_); }

void Cerebro::run() {
    fmt::print(fmt::fg(fmt::color::yellow), "Runnng strategy..\n");
    init_strategy();
    while (!feeds_agg_.finished()) {
        if (!feeds_agg_.read())
            break;
        broker_agg_.process_old_orders();
        auto order_pool = strategy_->execute();
        broker_agg_.process(order_pool);
        broker_agg_.update_info();

        fmt::print("Time: {}, wealth: {}\n", to_simple_string(feeds_agg_.time()),
                   broker_agg_.wealth());
    }
    broker_agg_.summary();
}
}; // namespace backtradercpp