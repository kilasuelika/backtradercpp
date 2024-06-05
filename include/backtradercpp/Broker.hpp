#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "DataFeeds.hpp"
#include "AyalysisUtil.hpp"
#include <list>
#include <fmt/core.h>

// #include "BaseBrokerImpl.hpp"

namespace backtradercpp {
class Cerebro;
namespace broker {
struct GenericCommission {
    GenericCommission() = default;
    GenericCommission(double long_rate, double short_rate)
        : long_commission_rate(long_rate), short_commission_rate(short_rate) {}

    virtual double cal_commission(double price, int volume) const {
        return (volume >= 0) ? price * volume * long_commission_rate
                             : -price * volume * short_commission_rate;
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
class BaseBroker;

struct BaseBrokerImplLogUtil {
    BaseBrokerImplLogUtil() = default;
    BaseBrokerImplLogUtil(const BaseBrokerImplLogUtil &util_)
        : log(util_.log), log_dir(util_.log_dir), name(util_.name) {
        if (log) {
        }
    }
    void set_log_dir(const std::string &dir, const std::string &name);
    void write_transaction(const std::string &log_) {
        if (log)
            transaction_file_ << log << std::endl;
    }
    void write_position(const std::string &log_) {
        if (log)
            position_file_ << log << std::endl;
    }

    bool log = false;
    std::string log_dir;
    std::string name;
    std::ofstream transaction_file_, position_file_;
};

class BaseBrokerImpl {

  public:
    BaseBrokerImpl(double cash = 10000, double long_rate = 0, double short_rate = 0,
                   double long_tax_rate = 0, double short_tax_rate = 0);
    // BaseBrokerImpl(const BaseBrokerImpl &impl_);

    void process(Order &order);
    void process_old_orders();
    virtual void process_trems() {} // Deal with terms, e.g. XRD.
    void update_info();
    const analysis::TotalValueAnalyzer &analyzer() const { return analyzer_; }

    int assets() const { return current_->volume.size(); }
    auto cash() const { return portfolio_.cash; }
    auto total_value() const { return portfolio_.total_value; }
    VecArrXi positions() const { return portfolio_.positions(assets()); }
    VecArrXd values() const { return portfolio_.values(assets()); }
    VecArrXd profits() const { return portfolio_.profits(assets()); }
    VecArrXd adj_profits() const { return portfolio_.adj_profits(assets()); }

    void set_allow_short(bool flag) { allow_short_ = flag; }
    void set_commission_rate(double long_rate, double short_rate) {
        commission_ = std::make_shared<GenericCommission>(long_rate, short_rate);
    }

    void set_log_dir(const std::string &dir, int index);

    void resize(int n);
    void set_feed(feeds::BasePriceDataFeed data);
    void set_df_feed(feeds::BasePriceDataFrameFeed data);
    auto feed() { return feed_; }

    void set_data_ptr(PriceFeedData *data) { current_ = data; }

    void add_order(const Order &order);

    void reset() {
        unprocessed.clear();
        analyzer_.reset();
        portfolio_.reset();
    }

    const auto &name() const { return feed_.name(); }
    const auto name(int index) const {
        const auto &name_ = name();
        return name_.empty() ? std::to_string(index) : name_;
    }

    virtual std::shared_ptr<BaseBrokerImpl> clone() {
        return std::make_shared<BaseBrokerImpl>(*this);
    }

  protected:
    friend class BrokerAggragator;
    friend class Cerebro;
    friend struct BaseBroker;

    feeds::BasePriceDataFeed feed_;
    feeds::BasePriceDataFrameFeed df_feed_;
    std::shared_ptr<GenericCommission> commission_;
    std::shared_ptr<GenericTax> tax_;
    PriceFeedData *current_;
    // double cash_ = 10000;
    Portfolio portfolio_;
    // VecArrXi position_;
    // VecArrXd investments_;
    //  FullAssetData *data_;
    std::list<Order> unprocessed;

    bool allow_short_ = false, allow_default_ = false;

    std::vector<std::string> codes_;

    analysis::TotalValueAnalyzer analyzer_;

    BaseBrokerImplLogUtil log_util_;

    void _write_position();
};

struct XRDSetting {
    double bonus, addition, dividen;
};
struct XRDTable {
    std::vector<date> record_date, execute_date;
    std::vector<XRDSetting> setting;

    void read(const std::string &file, const std::vector<int> &columns);
};

struct XRDRecord {
    int code;
    int volume;
    double cash;
};
// All XRD info will be read into memory.
class StockBrokerImpl : public BaseBrokerImpl {
  public:
    using BaseBrokerImpl::BaseBrokerImpl;

    void process_trems() override;
    // Columns: set column for record_date, execute_date, bonus嚗?, addition嚗蓮憓?,
    // dividen嚗?蝥ｇ?
    void set_xrd_dir(const std::string &dir, const std::vector<int> &columns,
                     std::function<std::string(const std::string &)> code_extract_func_ = nullptr);

  private:
    std::unordered_map<int, XRDTable> xrd_info_;

    std::unordered_multimap<date, XRDRecord> xrd_record_;

    std::function<std::string(const std::string &)> code_extract_func_ = nullptr;
};

class BaseBroker {
  protected:
    std::shared_ptr<BaseBrokerImpl> sp = nullptr;

  public:
    BaseBroker(double cash = 10000, double long_rate = 0, double short_rate = 0,
               double long_tax_rate = 0, double short_tax_rate = 0)
        : sp(std::make_shared<BaseBrokerImpl>(cash, long_rate, short_rate, long_tax_rate,
                                              short_tax_rate)) {}

    virtual BaseBroker &commission_eval(std::shared_ptr<GenericCommission> commission_) {
        sp->commission_ = commission_;
        return *this;
    }

    virtual BaseBroker &set_commission_rate(double long_rate, double short_rate) {
        sp->set_commission_rate(long_rate, short_rate);
        return *this;
    }

    virtual BaseBroker &tax(std::shared_ptr<GenericTax> tax_) {
        sp->tax_ = tax_;
        return *this;
    }

    virtual BaseBroker &allow_short() {
        sp->allow_short_ = true;
        return *this;
    }
    virtual BaseBroker &allow_default() {
        sp->allow_default_ = true;
        return *this;
    }

    virtual BaseBroker &reset() {
        sp->reset();
        return *this;
    }
    auto &portfolio() const { return sp->portfolio_; };

    void process(Order &order) { sp->process(order); }
    void process_old_orders() { sp->process_old_orders(); }
    void process_terms() { sp->process_trems(); }
    void update_info() { sp->update_info(); }
    const analysis::TotalValueAnalyzer &analyzer() const { return sp->analyzer(); }
    void set_log_dir(const std::string &dir, int index);

    const auto data_ptr() const { return sp->current_; }

    int assets() const { return sp->assets(); }
    auto cash() const { return sp->cash(); }
    auto total_value() const { return sp->total_value(); }

    VecArrXi positions() const { return sp->positions(); }
    VecArrXd values() const { return sp->values(); }
    VecArrXd profits() const { return sp->profits(); }
    VecArrXd adj_profits() const { return sp->adj_profits(); };

    void resize(int n) { sp->resize(n); };

    virtual BaseBroker &set_feed(feeds::BasePriceDataFeed data) {
        sp->set_feed(data);
        return *this;
    }

    virtual BaseBroker &set_df_feed(feeds::BasePriceDataFrameFeed data) {
        sp->set_df_feed(data);
        return *this;
    }
    auto feed() const { return sp->feed(); }

    void set_data_ptr(PriceFeedData *data) { sp->set_data_ptr(data); }

    void add_order(const Order &order) { sp->add_order(order); }

    const ptime &time() const { return sp->current_->time; }

    const auto &name() const { return sp->name(); }
    const auto &name(int index) const { return sp->name(index); }

    virtual ~BaseBroker() = default;
};

class StockBroker : public BaseBroker {
  protected:
    std::shared_ptr<StockBrokerImpl> sp = nullptr;

  public:
    StockBroker(double cash = 10000, double long_rate = 0, double short_rate = 0,
                double long_tax_rate = 0, double short_tax_rate = 0)
        : sp(std::make_shared<StockBrokerImpl>(cash, long_rate, short_rate, long_tax_rate,
                                               short_tax_rate)) {
        BaseBroker::sp = sp;
    }

    StockBroker &commission_eval(std::shared_ptr<GenericCommission> commission_) override {
        BaseBroker::commission_eval(commission_);
        return *this;
    }

    StockBroker &set_commission_rate(double long_rate, double short_rate = 0) override {
        BaseBroker::set_commission_rate(long_rate, short_rate);
        return *this;
    }

    StockBroker &tax(std::shared_ptr<GenericTax> tax_) override {
        BaseBroker::tax(tax_);
        return *this;
    }

    StockBroker &allow_short() override {
        BaseBroker::allow_short();
        return *this;
    }
    StockBroker &allow_default() override {
        BaseBroker::allow_default();
        return *this;
    }

    StockBroker &set_feed(feeds::BasePriceDataFeed data) {
        BaseBroker::set_feed(data);
        return *this;
    }

        StockBroker &set_df_feed(feeds::BasePriceDataFrameFeed data) {
        BaseBroker::set_df_feed(data);
        return *this;
    }

    StockBroker &
    set_xrd_dir(const std::string &dir, const std::vector<int> &columns,
                std::function<std::string(const std::string &)> code_extract_func_ = nullptr) {
        sp->set_xrd_dir(dir, columns, code_extract_func_);
        return *this;
    }
};

struct BrokerAggragatorLogUtil {};
class BrokerAggragator {
  public:
    void process(OrderPool &pool);
    void process_old_orders();
    void process_terms();
    void update_info(); // Calculate welath daily.
    void set_log_dir(const std::string &dir);

    void add_broker(const BaseBroker &broker);

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
    int assets(const std::string &broker_name) const {
        return brokers_[broker_name_map_.at(broker_name)].assets();
    }

    double total_wealth() const { return wealth_; }
    double wealth(int broker) const { return total_values_.coeff(broker); }
    const auto &wealth_history() const { return total_value_analyzer_.total_value_history(); }
    double cash(int broker) const { return brokers_[broker].portfolio().cash; }
    double total_cash() const {
        double cash = 0;
        for (const auto &b : brokers_)
            cash += b.portfolio().cash;
        return cash;
    }
    // void init();
    void summary();
    void cal_metrics();
    const auto &performance() const { return metric_analyzer_.performance(); }

    auto broker(int b) { return brokers_[b]; }
    auto broker(const std::string &b_name) { return brokers_[broker_name_map_.at(b_name)]; }
    auto broker_id(const std::string &b_name) { return broker_name_map_.at(b_name); }

    void reset() {
        times_.clear();
        total_value_analyzer_.reset();
        metric_analyzer_.reset();
        total_values_.resize(brokers_.size());

        for (auto &b : brokers_) {
            b.reset();
        }

        _collect_portfolio();
    }

    void sync_feed_agg(const feeds::PriceFeedAggragator &feed_agg_);

  private:
    // std::shared_ptr<feeds::FeedsAggragator> feed_agg_;
    std::vector<ptime> times_;
    std::vector<BaseBroker> brokers_;
    std::unordered_map<std::string, int> broker_name_map_; // Map name to index.

    std::vector<VecArrXi> positions_;
    std::vector<VecArrXd> values_, profits_, adj_profits_;

    VecArrXd total_values_; // Current total values of each broker.
    // VecArrXd total_value_history_; // History of total values.
    double wealth_ = 0;

    bool log_ = false;
    std::string log_dir_;

    std::shared_ptr<std::ofstream> wealth_file_;
    analysis::TotalValueAnalyzer total_value_analyzer_;
    analysis::MetricAnalyzer metric_analyzer_;

    // std::vector<analysis::PerformanceMetric> performance_;

    void _write_log();
    void _collect_portfolio();
};

inline void backtradercpp::broker::BaseBrokerImplLogUtil::set_log_dir(const std::string &dir,
                                                                      const std::string &name) {
    log = true;
    log_dir = dir;
    this->name = name;
    transaction_file_ =
        std::ofstream(std::filesystem::path(dir) /
                      std::filesystem ::path(std::format("Transaction_{}.csv", name)));
    transaction_file_ << "Date, CashBefore,  ID, Code, PositionBefore,  Direction, Volume, Price, "
                         "Value, Fee, PositionAfter, CashAfter,  Info"
                      << std::endl;

    position_file_ = std::ofstream(std::filesystem::path(dir) /
                                   std::filesystem ::path(std::format("Position_{}.csv", name)));
    position_file_ << "Date, ID, Code, Position, Price, Value, State" << std::endl;
}

inline BaseBrokerImpl::BaseBrokerImpl(double cash, double long_commission_rate,
                                      double short_commission_rate, double long_tax_rate,
                                      double short_tax_rate)
    : portfolio_{cash},
      commission_(std::make_shared<GenericCommission>(long_commission_rate, short_commission_rate)),
      tax_(std::make_shared<GenericTax>(long_tax_rate, long_commission_rate)) {}

inline void BaseBrokerImpl::process(Order &order) {
    // First check time
    int asset = order.asset;
    auto time = current_->time;
    bool to_unprocessed = true;
    if (time < order.valid_from) {
        order.state = OrderState::Waiting;
        return;
    } else if ((time <= order.valid_until)) {
        if (current_->valid.coeff(asset)) {
            PriceEvaluatorInput info{
                current_->data.open.coeff(asset), current_->data.high.coeff(asset),
                current_->data.low.coeff(asset), current_->data.close.coeff(asset)};
            // Calculate price.
            if (order.price_eval) {
                order.price = order.price_eval->price(info);
            }
            //  1.Price is between min and max.
            if ((order.price >= info.low) && (order.price <= info.high)) {
                order.value = order.volume * order.price;
                double commission = commission_->cal_commission(order.price, order.volume);
                double tax = tax_->cal_tax(order.price, order.volume);
                order.fee = commission + tax;
                double total_v = order.value + order.fee;
                bool order_valid = true;
                // 2. Cash constraint.
                if (portfolio_.cash < total_v) {
                    if (!allow_default_) {
                        order_valid = false;
                        fmt::print(fmt::fg(fmt::color::red), "Insufficient funds.\n");
                    }
                }
                // 3. Volume constraint.
                if (portfolio_.position(asset) + order.volume < 0) {
                    if (!allow_short_) {
                        order_valid = false;
                        fmt::print(fmt::fg(fmt::color::red), "Short not allowed.\n");
                    }
                }
                if (order_valid) {
                    order.processed = true;
                    order.processed_at = time;

                    double position_before = portfolio_.position(asset),
                           cash_before = portfolio_.cash;
                    portfolio_.update(order, current_->adj_data.close(asset));
                    double position_after = portfolio_.position(asset),
                           cash_after = portfolio_.cash;

                    order.state = OrderState::Success;

                    log_util_.write_transaction(std::format(
                        "{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}", util::to_string(time),
                        cash_before, asset, codes_[asset], position_before,
                        order.volume > 0 ? "+" : "-", order.volume, order.price, order.value,
                        order.fee, position_after, cash_after, ""));
                }
            }
        }
    } else {
        order.state = OrderState::Expired;
    }
}

inline void BaseBrokerImpl::process_old_orders() {
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

void BaseBrokerImpl::update_info() {
    log_util_.write_position(std::format("{}, {}, {}, {}, {}, {}, {}",
                                         util::to_string(current_->time), "", "Cash", "", "",
                                         portfolio_.cash, ""));
    for (auto &[asset, item] : portfolio_.portfolio_items) {
        std::string state = "";
        if (current_->valid.coeff(asset)) {
            item.update_value(current_->time, current_->data.close(asset),
                              current_->adj_data.close(asset));
        } else {
            state = "Invalid";
        }

        log_util_.write_position(std::format("{}, {}, {}, {}, {}, {}, {}",
                                             util::to_string(current_->time), asset, codes_[asset],
                                             item.position, item.prev_price, item.value, state));
    }
    portfolio_.update_info();
    analyzer_.update_total_value(portfolio_.total_value);
}

inline void BaseBrokerImpl::resize(int n) {}

void BaseBrokerImpl::set_feed(feeds::BasePriceDataFeed data) {
    feed_ = data;
    analyzer_.set_name(data.name());
    resize(data.assets());
    current_ = data.data_ptr();
    codes_ = data.codes();
}

void BaseBrokerImpl::set_df_feed(feeds::BasePriceDataFrameFeed data) {
    df_feed_ = data;
    analyzer_.set_name(data.name());
    resize(data.assets());
    current_ = data.data_ptr();
    codes_ = data.codes();
}


inline void BaseBrokerImpl::add_order(const Order &order) { unprocessed.emplace_back(order); }

void StockBrokerImpl::process_trems() {
    date dt = current_->time.date();

    // Register.
    for (const auto &[cd, pos] : portfolio_.portfolio_items) {
        const auto info_it = xrd_info_.find(cd);
        if (info_it != xrd_info_.end()) {
            const auto &recordates = info_it->second.record_date;
            const auto date_it = std::ranges::find(recordates, dt);
            if (date_it != recordates.end()) {
                int k = date_it - recordates.begin();
                const auto &setting = info_it->second.setting[k];
                xrd_record_.insert(std::make_pair(
                    info_it->second.execute_date[k],
                    XRDRecord{cd, int(pos.position / 10 * (setting.bonus + setting.addition)),
                              pos.position / 10 * setting.dividen}));
            }
        }
    }

    // Execute.
    auto records = xrd_record_.equal_range(dt);
    for (auto it = records.first; it != records.second; ++it) {
        const auto &[cd, vol, cash] = it->second;

        int position_before = portfolio_.position(cd);
        double cash_before = portfolio_.cash;
        portfolio_.transfer_stock(current_->time, cd, vol);
        portfolio_.transfer_cash(cash);
        int position_after = portfolio_.position(cd);
        double cash_after = portfolio_.cash;

        log_util_.write_transaction(
            std::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
                        util::to_string(ptime(dt)), cash_before, cd, codes_[cd], position_before,
                        "", vol, 0, cash, "", position_after, cash_after, "XRD"));
    }
}

void XRDTable::read(const std::string &file, const std::vector<int> &columns) {
    std::string row;
    std::ifstream f(file);

    std::getline(f, row);
    while (std::getline(f, row)) {
        if (row.size() > 0) {
            auto row_string = feeds::CSVRowParaser::parse_row(row);
            if (!row_string[columns[1]].empty()) {
                record_date.emplace_back(util::delimited_to_date(row_string[columns[0]]));
                execute_date.emplace_back(util::delimited_to_date(row_string[columns[1]]));
                setting.emplace_back(std::stod(row_string[columns[2]]),
                                     std::stod(row_string[columns[3]]),
                                     std::stod(row_string[columns[4]]));
            }
        }
    }
}
void StockBrokerImpl::set_xrd_dir(
    const std::string &dir, const std::vector<int> &columns,
    std::function<std::string(const std::string &)> code_extract_func_) {

    fmt::print(fmt::fg(fmt::color::yellow), "Reading XRD information.\n");

    // First read all codes in xrd_dir.
    util::check_path_exists(dir);
    std::unordered_map<std::string, std::string> code_file;
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".csv") {
            std::string code = entry.path().filename().stem().string();
            if (code_extract_func_ != nullptr) {
                code = code_extract_func_(code);
            }
            code_file[code] = entry.path().string();
        }
    }

    int ct = 0;
    for (int i = 0; i < codes_.size(); ++i) {
        const auto &cd = codes_[i];
        if (code_file.contains(cd)) {
            XRDTable tb;
            tb.read(code_file[cd], columns);
            xrd_info_[i] = std::move(tb);
            ++ct;
        }
    }

    fmt::print(fmt::fg(fmt::color::yellow), "Total {} codes.\n", ct);
}

inline void BrokerAggragator::process(OrderPool &pool) {
    // #pragma omp parallel for
    for (auto &order : pool.orders) {
        auto broker = brokers_[order.broker_id];
        broker.process(order);
        if (order.state = OrderState::Waiting) {
            broker.add_order(order);
        }
    }
    // std::cout << "processed" << std::endl;
}

inline void BrokerAggragator::process_old_orders() {
    for (auto &broker : brokers_) {
        broker.process_old_orders();
    }
}
inline void BrokerAggragator::process_terms() {
    for (auto &broker : brokers_) {
        broker.process_terms();
    }
}

inline void BrokerAggragator::update_info() {
    //  update portfolio value
    for (int i = 0; i < brokers_.size(); ++i) {
        auto broker = brokers_[i];
        const auto &data = broker.data_ptr();
        const auto &time = data->time;
        broker.update_info();

        total_values_.coeffRef(i) = broker.total_value();
        positions_[i] = broker.positions();
        values_[i] = broker.values();
        profits_[i] = broker.profits();
        adj_profits_[i] = broker.adj_profits();
    }
    wealth_ = total_values_.sum();
    total_value_analyzer_.update_total_value(wealth_);
    // Update time.
    ptime t = brokers_[0].time();
    for (int i = 1; i < brokers_.size(); ++i) {
        if (t < brokers_[i].time()) {
            t = brokers_[i].time();
        }
    }
    times_.emplace_back(std::move(t));
    _write_log();
}

void BrokerAggragator::_write_log() {
    try{
    *wealth_file_ << std::format("{}, {}", util::to_string(times_.back()), wealth_);

    }catch (const std::exception& e) {
                std::cerr << "錯誤: - " << e.what() << '\n';

            }
    for (int i = 0; i < brokers_.size(); ++i) {
        double cash = brokers_[i].cash();
        double holding_value = values_[i].sum();
        *wealth_file_ << std::format(", {}, {}, {}", cash, holding_value, cash + holding_value);
    }
    *wealth_file_ << std::endl;
}
void backtradercpp::broker::BrokerAggragator::_collect_portfolio() {
    for (int i = 0; i < brokers_.size(); ++i) {
        auto broker = brokers_[i];
        positions_[i] = broker.positions();
        values_[i] = broker.values();
        profits_[i] = broker.profits();
        adj_profits_[i] = broker.adj_profits();
    }
}

inline void BaseBrokerImpl::set_log_dir(const std::string &dir, int index) {
    log_util_.set_log_dir(dir, name(index));
}

inline void BrokerAggragator::set_log_dir(const std::string &dir) {
    log_ = true;
    log_dir_ = dir;

    for (int i = 0; i < brokers_.size(); ++i) {
        brokers_[i].set_log_dir(dir, i);
    }

    log_ = true;

    wealth_file_ = std::make_shared<std::ofstream>(std::filesystem::path(dir) /
                                                   std::filesystem ::path("TotalValue.csv"));
    *wealth_file_ << "Date, TotalValue";
    for (int i = 0; i < brokers_.size(); ++i) {
        const auto &name_ = brokers_[i].name(i);
        *wealth_file_ << std::format(
            ", Broker_{}_Cash, Broker_{}_HoldingValue, Broker_{}_TotalValue", name_, name_, name_);
    }
    *wealth_file_ << std::endl;
}
inline void BrokerAggragator::add_broker(const BaseBroker &broker) {
    broker_name_map_[broker.feed().name()] = brokers_.size();
    brokers_.push_back(broker);
    positions_.emplace_back(broker.positions());
    values_.emplace_back(broker.values());
    profits_.emplace_back();
    adj_profits_.emplace_back();

    total_values_.conservativeResize(total_values_.size() + 1);
    total_values_.bottomRows(1) = broker.portfolio().cash;
}

inline void BrokerAggragator::sync_feed_agg(const feeds::PriceFeedAggragator &feed_agg_) {
    for (int i = 0; i < brokers_.size(); ++i) {
        brokers_[i].set_feed(feed_agg_.feed(i));
    }
}

inline void BrokerAggragator::sync_feed_agg(const feeds::PriceFeedAggragator &feed_agg_) {
    for (int i = 0; i < brokers_.size(); ++i) {
        brokers_[i].set_df_feed(feed_agg_.feed(i));
    }
}

inline void BaseBroker::set_log_dir(const std::string &dir, int index) {

    sp->set_log_dir(dir, index);
}

void BrokerAggragator::cal_metrics() {
    metric_analyzer_.register_TotalValueAnalyzer(&total_value_analyzer_);
    if (brokers_.size() > 1) {
        for (const auto &b : brokers_) {
            metric_analyzer_.register_TotalValueAnalyzer(&(b.analyzer()));
        }
    }
    metric_analyzer_.cal_metrics();
}
inline void BrokerAggragator::summary() {
    util::cout("{:=^100}\nSummary\n", "");

    if (times_.empty()) {
        util::cout("No data...\n");
    } else {

        fmt::print("{: ^6} : {: ^21} ??{: ^21}, {: ^12} periods\n", "Time",
                   util::to_string(times_[0]), util::to_string(times_.back()), times_.size());
        double start_ = *(total_value_analyzer_.total_value_history().begin()),
               end_ = *(total_value_analyzer_.total_value_history().end() - 1), d = end_ - start_;
        if (d >= 0) {
            fmt::print("{: ^6} : {: ^21.4f} ??{: ^21.4f}, {: ^12.2f} profit\n", "Wealth", start_,
                       end_, d);
        } else {
            fmt::print("{: ^6} : {: ^21.4f} ??{: ^21.4f}, {: ^12.2f} loss\n", "Wealth", start_,
                       end_, d);
        }

        cal_metrics();
        metric_analyzer_.print_table();
        if (log_) {
            metric_analyzer_.write_table(
                (std::filesystem::path(log_dir_) / std::filesystem::path("Performance.csv"))
                    .string());
        }
    }

    util::cout("{:=^100}\n", "");
}
}; // namespace broker
}; // namespace backtradercpp
