#pragma once

/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "DataFeeds.hpp"
#include "BrokerImpl.hpp"

namespace backtradercpp {
namespace strategy {
class Cerebro;
class GenericStrategy {
    friend class Cerebro;

  public:
    const auto &time() const { return feed_agg_->time(); }
    int time_index() const { return time_index_; }

    const std::vector<FullAssetData> &datas() const { return feed_agg_->datas(); }
    const FullAssetData &data(int broker) const { return datas()[broker]; }


    virtual void init_strategy(feeds::FeedsAggragator *feed_agg,
                               broker::BrokerAggragator *broker_agg);

    const Order &buy(int broker_id, int asset, double price, int volume,
                     date_duration til_start_d = days(0), time_duration til_start_t = hours(0),
                     date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23));
    const Order &buy(int broker_id, int asset, std::shared_ptr<GenericPriceEvaluator> price_eval,
                     int volume, date_duration til_start_d = days(0),
                     time_duration til_start_t = hours(0), date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23));
    const Order &delayed_buy(int broker_id, int asset, double price, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0));
    const Order &delayed_buy(int broker_id, int asset,
                             std::shared_ptr<GenericPriceEvaluator> price_eval, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0));
    Order delayed_buy(int broker_id, int asset, int price, int volume, ptime start, ptime end);

    const Order &close(int broker_id, int asset, double price);
    const Order &close(int broker_id, int asset, std::shared_ptr<GenericPriceEvaluator> price_eval);

    // Use today's open as target price. Only TOTAL_FRACTION of total total_wealth will be allocated
    // to reserving for fee.
    template <int UNIT = 1>
    void adjust_to_weight_target(int broker_id, VecArrXd w, VecArrXd p = VecArrXd(),
                                 double TOTAL_FRACTION = 0.99);

    void pre_execute() {
        order_pool_.orders.clear();
        ++time_index_;
    }
    const auto &execute() {
        pre_execute();
        run();
        return order_pool_;
    }
    virtual void run() = 0;

    int assets(int broker) const { return broker_agg_->assets(broker); }
    double wealth(int broker) const { return broker_agg_->wealth(broker); }
    double cash(int broker) const { return broker_agg_->cash(broker); };

    const VecArrXi &positions(int broker) const;
    int position(int broker, int asset) const;

    const VecArrXd &values(int broker) const;
    double value(int broker, int asset) const;

    const VecArrXd &profits(int broker) const;
    double profit(int broker, int asset) const;
    const VecArrXd &adj_profits(int broker) const;
    double adj_profit(int broker, int asset) const;

    const auto &portfolio(int broker) const;
    const auto &portfolio_items(int broker) const;

    const std::vector<std::string> &codes(int broker) const { return feed_agg_->codes(broker); }

  private:
    int time_index_ = 0;

    Cerebro *cerebro_;
    feeds::FeedsAggragator *feed_agg_;
    broker::BrokerAggragator *broker_agg_;

    OrderPool order_pool_;
};

inline void GenericStrategy::init_strategy(feeds::FeedsAggragator *feed_agg,
                                           broker::BrokerAggragator *broker_agg) {
    feed_agg_ = feed_agg;
    broker_agg_ = broker_agg;
}

#define BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR(name, type, vectype)                                  \
    inline const vectype &GenericStrategy::name##s(int broker) const {                             \
        return broker_agg_->name##s(broker);                                                       \
    }                                                                                              \
    inline type GenericStrategy::name(int broker, int asset) const {                               \
        return broker_agg_->name##s(broker).coeff(asset);                                          \
    }
BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR(position, int, VecArrXi)
BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR(value, double, VecArrXd)
BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR(profit, double, VecArrXd)
BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR(adj_profit, double, VecArrXd)
#undef BK_STRATEGY_PORTFOLIO_MEMBER_ACESSOR

const auto &backtradercpp::strategy::GenericStrategy::portfolio(int broker) const {
    return broker_agg_->portfolio(broker);
}

const auto &backtradercpp::strategy::GenericStrategy::portfolio_items(int broker) const {
    return broker_agg_->portfolio(broker).portfolio_items;
}

const Order &GenericStrategy::buy(int broker_id, int asset, double price, int volume,
                                  date_duration til_start_d, time_duration til_start_t,
                                  date_duration start_to_end_d, time_duration start_to_end_t) {
    Order order{.broker_id = broker_id,
                .asset = asset,
                .price = price,
                .volume = volume,
                .created_at = time(),
                .valid_from = time() + til_start_d + til_start_t,
                .valid_until =
                    time() + til_start_d + til_start_t + start_to_end_d + start_to_end_t};
    order_pool_.orders.emplace_back(std::move(order));
    return order_pool_.orders.back();
}
const Order &GenericStrategy::buy(int broker_id, int asset,
                                  std::shared_ptr<GenericPriceEvaluator> price_eval, int volume,
                                  date_duration til_start_d, time_duration til_start_t,
                                  date_duration start_to_end_d, time_duration start_to_end_t) {
    Order order{.broker_id = broker_id,
                .asset = asset,
                .volume = volume,
                .price_eval = price_eval,
                .created_at = time(),
                .valid_from = time() + til_start_d + til_start_t,
                .valid_until =
                    time() + til_start_d + til_start_t + start_to_end_d + start_to_end_t};
    order_pool_.orders.emplace_back(std::move(order));
    return order_pool_.orders.back();
}
const Order &GenericStrategy::delayed_buy(int broker_id, int asset, double price, int volume,
                                          date_duration til_start_d, time_duration til_start_t,
                                          date_duration start_to_end_d,
                                          time_duration start_to_end_t) {
    return buy(broker_id, asset, price, volume, til_start_d, til_start_t, start_to_end_d,
               start_to_end_t);
}
const Order &GenericStrategy::delayed_buy(int broker_id, int asset,
                                          std::shared_ptr<GenericPriceEvaluator> price_eval,
                                          int volume, date_duration til_start_d,
                                          time_duration til_start_t, date_duration start_to_end_d,
                                          time_duration start_to_end_t) {
    return buy(broker_id, asset, price_eval, volume, til_start_d, til_start_t, start_to_end_d,
               start_to_end_t);
}
inline const Order &GenericStrategy::close(int broker_id, int asset, double price) {
    return delayed_buy(broker_id, asset, price, position(broker_id, asset));
}
inline const backtradercpp::Order &
backtradercpp::strategy::GenericStrategy::close(int broker_id, int asset,
                                                std::shared_ptr<GenericPriceEvaluator> price_eval) {
    return delayed_buy(broker_id, asset, price_eval, -position(broker_id, asset));
}

template <int UNIT>
void GenericStrategy::adjust_to_weight_target(int broker_id, VecArrXd w, VecArrXd p,
                                              double TOTAL_FRACTION) {
    VecArrXd target_prices = data(broker_id).open();
    if (p.size() != 0) {
        target_prices = p;
    }
    VecArrXd target_value = wealth(broker_id) * TOTAL_FRACTION * w;
    VecArrXi target_volume = (target_value / (target_prices * UNIT)).cast<int>();
    VecArrXi volume_diff = target_volume - positions(broker_id);
    for (int i = 0; i < volume_diff.size(); ++i) {
        if (data(broker_id).valid().coeff(i) && (volume_diff.coeff(i) != 0)) {
            buy(broker_id, i, target_prices.coeff(i), volume_diff.coeff(i));
        }
    }
};
} // namespace strategy
} // namespace backtradercpp
