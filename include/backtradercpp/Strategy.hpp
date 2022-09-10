#pragma once

/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "DataFeeds.hpp"

namespace backtradercpp {
namespace strategy {
class Cerebro;
class GenericStrategy {
    friend class Cerebro;

  public:
    const auto &time() const { return feed_agg_->time(); }
    const std::vector<FullAssetData> &datas() const { return feed_agg_->datas(); }
    const FullAssetData &data(int i) const { return datas()[i]; }
    virtual void init_strategy(feeds::FeedsAggragator *feed_agg,
                               broker::BrokerAggragator *broker_agg);

    const Order &buy(int broker_id, int asset, double price, int volume);
    Order delayed_buy(int broker_id, int asset, int price, int volume,
                      date_duration til_start_d = days(1), time_duration til_start_t = hours(0),
                      date_duration start_to_end_d = days(1),
                      time_duration start_to_end_t = hours(0));
    Order delayed_buy(int broker_id, int asset, int price, int volume, ptime start, ptime end);

    void pre_execute() { order_pool_.orders.clear(); }
    const auto &execute() {
        pre_execute();
        run();
        return order_pool_;
    }
    virtual void run() = 0;

    const auto &positions() const { return broker_agg_->positions(); }
    const auto &position(int broker) const { return positions()[broker]; }
    auto position(int broker, int asset) const { return position(broker)->coeff(asset); }
    const auto &values() const { return broker_agg_->values(); }

  private:
    Cerebro *cerebro_;
    feeds::FeedsAggragator *feed_agg_;
    broker::BrokerAggragator *broker_agg_;

    // std::vector<FullAssetData> *data_;
    // std::vector<VecArrXi *> position_;
    // std::vector<VecArrXd *> value_;

    OrderPool order_pool_;
};

inline void GenericStrategy::init_strategy(feeds::FeedsAggragator *feed_agg,
                                           broker::BrokerAggragator *broker_agg) {
    feed_agg_ = feed_agg;
    broker_agg_ = broker_agg;
}

const Order &GenericStrategy::buy(int broker_id, int asset, double price, int volume) {
    fmt::print("new order.\n");
    Order order{.broker_id = broker_id,
                .asset = asset,
                .price = price,
                .volume = volume,
                .valid_from = time(),
                .valid_until = time() + hours(23)};
    order_pool_.orders.emplace_back(std::move(order));
    return order_pool_.orders.back();
}
} // namespace strategy
} // namespace backtradercpp
