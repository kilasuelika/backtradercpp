#pragma once

/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "DataFeeds.hpp"
#include "Broker.hpp"

namespace backtradercpp {
namespace strategy {
class Cerebro;

class GenericStrategy {
    friend class Cerebro;

  public:
    const auto &time() const { return price_feed_agg_->time(); }
    int time_index() const { return time_index_; }

    const auto &datas() const { return price_feed_agg_->datas(); }
    const auto &data(int broker) const { return datas()[broker]; }
    const auto &data(const std::string &broker_name) const {
        return datas()[broker_agg_->broker_id(broker_name)];
    }

    const auto &common_datas() const { return common_feed_agg_->datas(); }
    const auto &common_data(int i) const { return common_datas()[i]; }
    const auto &common_data(const std::string &name) const {
        return common_datas()[broker_agg_->broker_id(name)];
    }

    bool data_valid(int feed) const { return price_feed_agg_->data_valid(feed); }

    virtual void init_strategy(feeds::PriceFeedAggragator *feed_agg,
                               feeds::CommonFeedAggragator *common_feed_agg,
                               broker::BrokerAggragator *broker_agg);

    const Order &buy(int broker_id, int asset, double price, int volume,
                     date_duration til_start_d = days(0), time_duration til_start_t = hours(0),
                     date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23));
    const Order &buy(const std::string &broker_name, int asset, double price, int volume,
                     date_duration til_start_d = days(0), time_duration til_start_t = hours(0),
                     date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23)) {
        buy(broker_id(broker_name), asset, price, volume, til_start_d, til_start_t, start_to_end_d,
            start_to_end_t);
    }
    const Order &buy(int broker_id, int asset, std::shared_ptr<GenericPriceEvaluator> price_eval,
                     int volume, date_duration til_start_d = days(0),
                     time_duration til_start_t = hours(0), date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23));
    const Order &buy(const std::string &broker_name, int asset,
                     std::shared_ptr<GenericPriceEvaluator> price_eval, int volume,
                     date_duration til_start_d = days(0), time_duration til_start_t = hours(0),
                     date_duration start_to_end_d = days(0),
                     time_duration start_to_end_t = hours(23)) {
        buy(broker_id(broker_name), asset, price_eval, volume, til_start_d, til_start_t,
            start_to_end_d, start_to_end_t);
    }
    const Order &delayed_buy(int broker_id, int asset, double price, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0));
    const Order &delayed_buy(const std::string &broker_name, int asset, double price, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0)) {
        delayed_buy(broker_id(broker_name), asset, price, volume, til_start_d, til_start_t,
                    start_to_end_d, start_to_end_t);
    }
    const Order &delayed_buy(int broker_id, int asset,
                             std::shared_ptr<GenericPriceEvaluator> price_eval, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0));
    const Order &delayed_buy(const std::string &broker_name, int asset,
                             std::shared_ptr<GenericPriceEvaluator> price_eval, int volume,
                             date_duration til_start_d = days(1),
                             time_duration til_start_t = hours(0),
                             date_duration start_to_end_d = days(1),
                             time_duration start_to_end_t = hours(0)) {
        delayed_buy(broker_id(broker_name), asset, price_eval, volume, til_start_d, til_start_t,
                    start_to_end_d, start_to_end_t);
    }
    Order delayed_buy(int broker_id, int asset, int price, int volume, ptime start, ptime end);

    const Order &close(int broker_id, int asset, double price);
    const Order &close(const std::string &broker_name, int asset, double price) {
        close(broker_id(broker_name), asset, price);
    }
    const Order &close(int broker_id, int asset, std::shared_ptr<GenericPriceEvaluator> price_eval);
    const Order &close(const std::string &broker_name, int asset,
                       std::shared_ptr<GenericPriceEvaluator> price_eval) {
        close(broker_id(broker_name), asset, price_eval);
    }

    // Use today's open as target price. Only TOTAL_FRACTION of total total_wealth will be allocated
    // to reserving for fee.
    template <int UNIT = 1>
    void adjust_to_weight_target(int broker_id, const VecArrXd &w, const VecArrXd &p = VecArrXd(),
                                 double TOTAL_FRACTION = 0.99);
    void adjust_to_weight_target(const std::string &broker_name, const VecArrXd &w,
                                 const VecArrXd &p = VecArrXd(), double TOTAL_FRACTION = 0.99) {
        adjust_to_weight_target(broker_id(broker_name), w, p, TOTAL_FRACTION);
    };

    void adjust_to_volume_target(int broker_id, const VecArrXi &target_volume,
                                 const VecArrXd &p = VecArrXd(), double TOTAL_FRACTION = 0.99);
    void adjust_to_volume_target(const std::string &broker_name, const VecArrXi &target_volume,
                                 const VecArrXd &p = VecArrXd(), double TOTAL_FRACTION = 0.99) {
        adjust_to_volume_target(broker_id(broker_name), target_volume, p, TOTAL_FRACTION);
    };

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
    virtual void reset() { time_index_ = -1; }

    int assets(int broker) const { return broker_agg_->assets(broker); }
    int assets(const std::string &broker_name) const { return broker_agg_->assets(broker_name); }

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

    const std::vector<std::string> &codes(int broker) const {
        return price_feed_agg_->feed(broker).codes();
    }

    int broker_id(const std::string &name) const { return broker_agg_->broker_id(name); }

  private:
    int time_index_ = -1;

    Cerebro *cerebro_;
    feeds::PriceFeedAggragator *price_feed_agg_;
    feeds::CommonFeedAggragator *common_feed_agg_;

    broker::BrokerAggragator *broker_agg_;

    OrderPool order_pool_;
};

inline void GenericStrategy::init_strategy(feeds::PriceFeedAggragator *feed_agg,
                                           feeds::CommonFeedAggragator *common_feed_agg,
                                           broker::BrokerAggragator *broker_agg) {
    price_feed_agg_ = feed_agg;
    common_feed_agg_ = common_feed_agg;

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

const auto &GenericStrategy::portfolio(int broker) const { return broker_agg_->portfolio(broker); }

const auto &GenericStrategy::portfolio_items(int broker) const {
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
void GenericStrategy::adjust_to_weight_target(int broker_id, const VecArrXd &w, const VecArrXd &p,
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
}

void GenericStrategy::adjust_to_volume_target(int broker_id, const VecArrXi &target_volume,
                                              const VecArrXd &p, double TOTAL_FRACTION) {
    VecArrXd target_prices = data(broker_id).open();
    if (p.size() != 0) {
        target_prices = p;
    }
    VecArrXi volume_diff = target_volume - positions(broker_id);
    for (int i = 0; i < volume_diff.size(); ++i) {
        if (data(broker_id).valid().coeff(i) && (volume_diff.coeff(i) != 0)) {
            buy(broker_id, i, target_prices.coeff(i), volume_diff.coeff(i));
        }
    }
}
} // namespace strategy
} // namespace backtradercpp
