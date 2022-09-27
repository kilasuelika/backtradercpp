#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "DataFeeds.hpp"
#include "AyalysisUtil.hpp"
#include <list>

namespace backtradercpp {
class Cerebro;
namespace broker {
struct GenericCommission {
    GenericCommission() = default;
    GenericCommission(double long_rate, double short_rate)
        : long_commission_rate(long_rate), short_commission_rate(short_rate){};

    virtual double cal_commission(double price, int volume) const {
        return (volume >= 0) ? price * volume * long_commission_rate
                             : price * volume * short_commission_rate;
    }
    // virtual VecArrXd cal_commission(VecArrXd price, VecArrXi volume) const;
    double long_commission_rate = 0, short_commission_rate = 0;
};
struct GenericTax {
    GenericTax() = default;
    GenericTax(double long_rate, double short_rate)
        : long_tax_rate(long_rate), short_tax_rate(short_rate){};

    virtual double cal_tax(double price, int volume) const {
        return (volume >= 0) ? price * volume * long_tax_rate : price * volume * short_tax_rate;
    }
    // virtual VecArrXd cal_tax(VecArrXd price, VecArrXi volume) const;
    double long_tax_rate = 0, short_tax_rate = 0;
};
class BrokerAggragator;
struct Broker;

class BrokerImpl {

  public:
    BrokerImpl(double cash = 10000, double long_rate = 0, double short_rate = 0,
               double long_tax_rate = 0, double short_tax_rate = 0);


    void process(Order &order);
    void process_old_orders();
    void update_info();

    int assets() const { return current_->volume.size(); }
    VecArrXi positions() const { return portfolio_.positions(assets()); }
    VecArrXd values() const { return portfolio_.values(assets()); }
    VecArrXd profits() const { return portfolio_.profits(assets()); }
    VecArrXd adj_profits() const { return portfolio_.adj_profits(assets()); };

    void resize(int n);

    void set_data_ptr(FeedData *data) { current_ = data; }

    void add_order(const Order &order);

  private:
    friend class BrokerAggragator;
    friend class Cerebro;
    friend struct Broker;

    std::shared_ptr<GenericCommission> commission_;
    std::shared_ptr<GenericTax> tax_;
    FeedData *current_;
    // double cash_ = 10000;
    Portfolio portfolio_;

    // VecArrXi position_;
    // VecArrXd investments_;
    //  FullAssetData *data_;
    std::list<Order> unprocessed;
};

struct Broker {
    std::shared_ptr<BrokerImpl> sp;

    Broker(double cash = 10000, double long_rate = 0, double short_rate = 0,
           double long_tax_rate = 0, double short_tax_rate = 0)
        : sp(std::make_shared<BrokerImpl>(cash, long_rate, short_rate, long_tax_rate,
                                          short_tax_rate)) {
    }

    Broker &commission_eval(std::shared_ptr<GenericCommission> commission_) {
        sp->commission_ = commission_;
        return *this;
    }

    Broker &commission_rate(double long_rate, double short_rate = 0);

    Broker &tax(std::shared_ptr<GenericTax> tax_) {
        sp->tax_ = tax_;
        return *this;
    }

    const auto &portfolio() const { return sp->portfolio_; };

    void process(Order &order) { sp->process(order); };
    void process_old_orders() { sp->process_old_orders(); };
    void update_info() { sp->update_info(); };

    int assets() const { return sp->assets(); }
    VecArrXi positions() const { return sp->positions(); }
    VecArrXd values() const { return sp->values(); }
    VecArrXd profits() const { return sp->profits(); }
    VecArrXd adj_profits() const { return sp->adj_profits(); };

    void resize(int n) { sp->resize(n); };

    void set_data_ptr(FeedData *data) { sp->set_data_ptr(data); }

    void add_order(const Order &order) { sp->add_order(order); };
};
class BrokerAggragator {
  public:
    void process(OrderPool &pool);
    void process_old_orders();
    void update_info(); // Calculate welath daily.

    void add_broker(const Broker &broker);

    const auto &positions() const { return positions_; }
    const auto &positions(int broker) const { return positions_[broker]; }

    const auto &values() const { return values_; }
    const auto &values(int broker) const { return values_[broker]; }

    const auto &profits() const { return profits_; }
    const auto &profits(int broker) const { return profits_[broker]; }
    const auto &adj_profits() const { return adj_profits_; }
    const auto &adj_profits(int broker) const { return adj_profits_[broker]; }

    int position(int broker, int asset) const {
        return brokers_[broker].portfolio().position(asset);
    }

    const auto &portfolio(int broker) const { return brokers_[broker].portfolio(); }

    int assets(int broker) const { return brokers_[broker].assets(); }
    double total_wealth() const { return wealth_; }
    double wealth(int broker) const { return wealths_.coeff(broker); }
    const auto &wealth_history() const { return wealth_history_; }
    double cash(int broker) const { return brokers_[broker].portfolio().cash; }
    // void init();
    void summary() const;

  private:
    // std::shared_ptr<feeds::FeedsAggragator> feed_agg_;
    std::vector<Broker> brokers_;

    std::vector<VecArrXi> positions_;
    std::vector<VecArrXd> values_, profits_, adj_profits_;

    VecArrXd wealths_; // welath of each broker.
    VecArrXd wealth_history_;
    double wealth_ = 0;
};

inline BrokerImpl::BrokerImpl(double cash, double long_commission_rate,
                              double short_commission_rate,
                              double long_tax_rate, double short_tax_rate)
    : portfolio_{cash},
      commission_(std::make_shared<GenericCommission>(long_commission_rate, short_commission_rate)),
      tax_(std::make_shared<GenericTax>(long_tax_rate, long_commission_rate)) {}

Broker &Broker::commission_rate(double long_rate, double short_rate) {
    sp->commission_->long_commission_rate = long_rate;
    sp->commission_->short_commission_rate = short_rate;
    return *this;
}

inline void BrokerImpl::process(Order &order) {
    // fmt::print("processing order.\n");
    // First check time
    int asset = order.asset;
    auto time = current_->time;
    bool to_unprocessed = true;
    if (time < order.valid_from) {
        order.state = OrderState::Waiting;
        return;
    } else if ((time <= order.valid_until)) {
        if (current_->valid.coeff(asset)) {
            //    fmt::print(fmt::fg(fmt::color::yellow), "in time\n");
            PriceEvaluatorInput info{
                current_->data.open.coeff(asset), current_->data.high.coeff(asset),
                current_->data.low.coeff(asset), current_->data.close.coeff(asset)};
            // Calculate price.
            if (order.price_eval) {
                order.price = order.price_eval->price(info);
            }
            //  fmt::print("target_price: {}, max: {}\n", order.price,
            //            current_->data.high.coeff(asset));
            //  1.Price is between min and max.
            if ((order.price >= info.low) && (order.price <= info.high)) {
                // fmt::print("Final decide\n");
                order.value = order.volume * order.price;
                double commission = commission_->cal_commission(order.price, order.volume);
                double tax = tax_->cal_tax(order.price, order.volume);
                order.fee = commission + tax;
                double total_v = order.value + order.fee;
                // fmt::print("cash: {}, total_v: {}\n", portfolio_.cash, total_v);
                if (((order.volume > 0) && (portfolio_.cash > total_v)) || (order.volume < 0)) {
                    order.processed = true;
                    order.processed_at = time;
                    //   fmt::print(fmt::fg(fmt::color::yellow), "created_at: {}, success_at: {}\n",
                    //             boost::posix_time::to_iso_string//(order.created_at),
                    //              to_iso_string(order.processed_at));
                    portfolio_.update(order, current_->adj_data.close(asset));
                    //  fmt::print("order processed. cash: {}\n", portfolio_.cash);
                    order.state = OrderState::Success;
                }
            }
        }
    } else {
        order.state = OrderState::Expired;
    }
}

inline void BrokerImpl::process_old_orders() {
    // Remove expired
    for (auto it = unprocessed.begin(); it != unprocessed.end();) {
        process(*it);
        if (it->state == OrderState::Waiting) {
            it++;
        } else {
            // If expired or success, remove it.
            it = unprocessed.erase(it);
        }
    }
}

inline void BrokerImpl::resize(int n) {
}

inline void BrokerImpl::add_order(const Order &order) { unprocessed.emplace_back(order); }
inline void BrokerAggragator::process(OrderPool &pool) {
    //#pragma omp parallel for
    for (auto &order : pool.orders) {
        auto broker = brokers_[order.broker_id];
        broker.process(order);
        if (order.state = OrderState::Waiting) {
            broker.add_order(order);
        }
    }
}

inline void BrokerAggragator::process_old_orders() {
    for (auto &broker : brokers_) {
        broker.process_old_orders();
    }
}
inline void BrokerAggragator::update_info() {
    //  update portfolio value
    for (int i = 0; i < brokers_.size(); ++i) {
        auto broker = brokers_[i];
        const auto &data = broker.sp->current_;
        const auto &time = data->time;
        wealths_.coeffRef(i) = broker.portfolio().cash;
        for (auto &[asset, item] : broker.sp->portfolio_.portfolio_items) {
            if (data->valid.coeff(asset)) {
                item.update_value(time, data->data.close(asset), data->adj_data.close(asset));
            }
            wealths_.coeffRef(i) += item.value + item.dyn_adj_profit - item.profit;
        }
        positions_[i] = broker.positions();
        values_[i] = broker.values();
        profits_[i] = broker.profits();
        adj_profits_[i] = broker.adj_profits();
    }
    wealth_ = wealths_.sum();
    int n = wealth_history_.size();
    wealth_history_.conservativeResize(n + 1);
    wealth_history_.coeffRef(n) = wealth_;
}

inline void BrokerAggragator::add_broker(const Broker &broker) {
    brokers_.push_back(broker);
    positions_.emplace_back();
    values_.emplace_back();
    profits_.emplace_back();
    adj_profits_.emplace_back();

    wealths_.conservativeResize(wealths_.size() + 1);
    wealths_.bottomRows(1) = broker.portfolio().cash;
}

inline void BrokerAggragator::summary() const {
    fmt::print("Initial: {:14.4f}, Final: {:14.4f}\n", wealth_history_.coeff(0),
               *(wealth_history_.end() - 1));
    analysis::cal_performance(wealth_history_).print();
}
} // namespace broker
} // namespace backtradercpp