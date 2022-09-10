#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "AyalysisUtil.hpp"
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

class Broker {

  public:
    Broker(double long_rate = 0, double short_rate = 0, double long_tax_rate = 0,
           double short_tax_rate = 0);
    Broker &commission_eval(std::shared_ptr<GenericCommission> commission_) {
        this->commission_ = commission_;
        return *this;
    }
    Broker &commission_rate(double long_rate, double short_rate = 0);

    Broker &tax(std::shared_ptr<GenericTax> tax_) {
        this->tax_ = tax_;
        return *this;
    }
    void process(Order &order);
    void process_old_orders();
    void update_info();

    void resize(int n);

    void set_data_ptr(FeedData *data) { current_ = data; }

  private:
    friend class BrokerAggragator;
    friend class Cerebro;

    std::shared_ptr<GenericCommission> commission_;
    std::shared_ptr<GenericTax> tax_;
    FeedData *current_;
    double cash_ = 10000;
    VecArrXi position_;
    // FullAssetData *data_;
    std::vector<Order> unprocessed;
};

class BrokerAggragator {
  public:
    void process(OrderPool &pool);
    void process_old_orders();
    void update_info(); // Calculate welath daily.

    void add_broker(std::shared_ptr<broker::Broker> broker);
    const auto &positions() const { return positions_; }
    const auto &values() const { return values_; }
    double wealth() const { return wealth_; }
    // void init();
    void summary() const;

  private:
    std::vector<std::shared_ptr<broker::Broker>> brokers_;
    std::vector<VecArrXi *> positions_;
    std::vector<VecArrXd> values_;

    VecArrXd wealth_history_;
    double wealth_ = 0;
};

inline Broker::Broker(double long_commission_rate, double short_commission_rate,
                      double long_tax_rate, double short_tax_rate)
    : commission_(std::make_shared<GenericCommission>(long_commission_rate, short_commission_rate)),
      tax_(std::make_shared<GenericTax>(long_tax_rate, long_commission_rate)) {}
backtradercpp::broker::Broker &backtradercpp::broker::Broker::commission_rate(double long_rate,
                                                                              double short_rate) {
    commission_->long_commission_rate = long_rate;
    commission_->short_commission_rate = short_rate;
}
inline void Broker::process(Order &order) {
    fmt::print("processing order.\n");
    // First check time
    int asset = order.asset;
    auto time = current_->time;
    bool to_unprocessed = true;
    if ((time >= order.valid_from) && (time <= order.valid_until)) {
        if (current_->valid.coeff(asset)) {
            PriceEvaluatorInput info{
                current_->data.open.coeff(asset), current_->data.high.coeff(asset),
                current_->data.low.coeff(asset), current_->data.close.coeff(asset)};
            // Calculate price.
            if (order.price_eval) {
                order.price = order.price_eval->price(info);
            }
            // 1.Price is between min and max.
            if ((order.price >= info.low) && (order.price <= info.high)) {
                double target_value = order.volume * order.price;
                double commission = commission_->cal_commission(order.price, order.volume);
                double tax = tax_->cal_tax(order.price, order.volume);
                double total_v = target_value + commission + tax;
                if (((order.volume > 0) && (cash_ > total_v)) || (order.volume < 0)) {
                    position_.coeffRef(asset) += order.volume;
                    cash_ -= total_v;
                    order.processed = true;
                    order.processed_at = time;
                    fmt::print("order processed. cash: {}\n", cash_);
                    to_unprocessed = false;
                }
            }
        }
    } else {
        fmt::print("time expired.\n");
        to_unprocessed = false;
    }
    if (to_unprocessed) {
        unprocessed.emplace_back(order);
    }
}
void backtradercpp::broker::Broker::process_old_orders() {
    for (auto &order : unprocessed) {
        process(order);
    }
}

inline void Broker::resize(int n) { position_ = VecArrXi::Constant(n, 0); }

inline void backtradercpp::broker::BrokerAggragator::process(OrderPool &pool) {
    //#pragma omp parallel for
    for (auto &order : pool.orders) {
        brokers_[order.broker_id]->process(order);
    }
}
void BrokerAggragator::process_old_orders() {
    for (auto &broker : brokers_) {
        broker->process_old_orders();
    }
}
void BrokerAggragator::update_info() {
    wealth_ = 0;
    for (int i = 0; i < brokers_.size(); ++i) {
        values_[i] = positions()[i]->cast<double>() * brokers_[i]->current_->data.close;
        wealth_ = brokers_[i]->cash_ + values_[i].sum();
    }
    int n = wealth_history_.size();
    wealth_history_.conservativeResize(n + 1);
    wealth_history_.coeffRef(n) = wealth_;
}

void BrokerAggragator::add_broker(std::shared_ptr<broker::Broker> broker) {
    brokers_.push_back(broker);
    positions_.push_back(&broker->position_);
    values_.emplace_back();
}
void BrokerAggragator::summary() const {
    fmt::print("Initial: {:14.4f}, Final: {:14.4f}\n", wealth_history_.coeff(0),
               *(wealth_history_.end() - 1));
    analysis::cal_performance(wealth_history_).print();
}
} // namespace broker
} // namespace backtradercpp