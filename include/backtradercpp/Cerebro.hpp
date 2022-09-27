#pragma once
/*ZhouYao at 2022-09-10*/

#include "DataFeeds.hpp"
#include "BrokerImpl.hpp"
#include "Strategy.hpp"
namespace backtradercpp {

class Cerebro {
  public:
    void add_data(feeds::GenericData data,
                  broker::Broker broker,
                  int window = 1);
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

void Cerebro::add_data(feeds::GenericData data,
                       broker::Broker broker, int window) {
    feeds_agg_.add_feed(data);
    feeds_agg_.set_window(feeds_agg_.datas().size() - 1, window);
    broker_agg_.add_broker(broker);

    broker.resize(data.assets());
    broker.sp->current_ = data.data_ptr();
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

        fmt::print("{}, cash: {:12.4f},  total_wealth: {:12.4f}\n",
                   boost::posix_time::to_simple_string(feeds_agg_.time()), broker_agg_.cash(0),
                   broker_agg_.total_wealth());

        // std::cout << "close:" << feeds_agg_.data(0).close().transpose() << std::endl;
        // std::cout << "adj_close:" << feeds_agg_.data(0).adj_close().transpose() << std::endl;
        // std::cout << "position: " << broker_agg_.positions(0).transpose() << std::endl;
        //// std::cout << "profits: " << broker_agg_.profits(0).transpose() << std::endl;
        // std::cout << "adj_profits: " << broker_agg_.adj_profits(0).transpose() << std::endl;
        //// std::cout << "values: " << broker_agg_.values(0).transpose() << std::endl;
        //// std::cout << "dyn_adj_profits: "
        ////           <<
        ///broker_agg_.portfolio(0).dyn_adj_profits(broker_agg_.assets(0)).transpose() / <<
        ///std::endl;

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