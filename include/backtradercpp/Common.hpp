#pragma once
/*ZhouYao at 2022-09-10*/

#include <Eigen/Core>
#include <boost/circular_buffer.hpp>
#include <concepts>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost\date_time\gregorian\gregorian.hpp>
#include <queue>
#include <fmt/format.h>
#include <fmt/color.h>
#include <memory>
#include <map>

namespace backtradercpp {
using VecArrXd = Eigen::Array<double, Eigen::Dynamic, 1>;
using VecArrXi = Eigen::Array<int, Eigen::Dynamic, 1>;
using VecArrXb = Eigen::Array<bool, Eigen::Dynamic, 1>;

using boost::gregorian::date_duration;
using boost::gregorian::days;

using boost::posix_time::hours;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;

struct OHLCData {
    VecArrXd open, high, low, close;
    void resize(int assets);
    void reset();
};
struct FeedData {
    boost::posix_time::ptime time;
    OHLCData data, adj_data;
    VecArrXi volume;
    VecArrXb valid;
    void validate_assets();
    void resize(int assets);

    void reset();
};

class FullAssetData {
  public:
    FullAssetData() = default;
    FullAssetData(int assets, int window = 1);
    FullAssetData(const FeedData &data, int window = 1);

    const FeedData &data(int time = -1) const;

    VecArrXd open(int time = -1) const; // negative indices, -1 for latest
    double open(int time, int stock) const;
    VecArrXd high(int time = -1) const;
    double high(int time, int stock) const;
    VecArrXd low(int time = -1) const;
    double low(int time, int stock) const;
    VecArrXd close(int time = -1) const;
    double close(int time, int stock) const;

    VecArrXd adj_open(int time = -1) const;
    double adj_open(int time, int stock) const;
    VecArrXd adj_high(int time = -1) const;
    double adj_high(int time, int stock) const;
    VecArrXd adj_low(int time = -1) const;
    double adj_low(int time, int stock) const;
    VecArrXd adj_close(int time = -1) const;
    double adj_close(int time, int stock) const;

    VecArrXi volume(int time = -1) const;
    int volume(int time, int stock) const;
    VecArrXb valid(int time = -1) const;
    bool valid(int time, int stock) const;

    template <typename T>
    requires requires { std::is_same_v<T, FeedData>; }
    void push_back(T &&new_data) { data_.push_back(std::forward<T>(new_data)); }

    int window() const { return window_; }
    void set_window(int window);
    auto assets() const { return assets_; }
    const auto &time() const { return data_.back().time; }

  private:
    int window_ = 1, assets_ = 0;
    boost::circular_buffer<FeedData> data_;
};

enum OrderType { Market, Limit };
enum OrderState { Success, Waiting, Expired };
struct PriceEvaluatorInput {
    double open, high, low, close;
};
struct GenericPriceEvaluator {
    virtual double price(const PriceEvaluatorInput &input) = 0;
    virtual ~GenericPriceEvaluator() = default;
};
struct EvalOpen : GenericPriceEvaluator {
    int tag = 0; // 0:exact, 1: open+v, 2:open*v
    double v = 0;
    EvalOpen(int tag_, int v_) : tag(tag_), v(v_) {}
    double price(const PriceEvaluatorInput &input) override {
        switch (tag) {
        case (0):
            return input.open;
        case (1):
            return input.open + v;
        case (2):
            return input.open * v;
        default:
            return input.open;
        }
    }
    static std::shared_ptr<GenericPriceEvaluator> exact() {
        static std::shared_ptr<GenericPriceEvaluator> p = std::make_shared<EvalOpen>(0, 0);
        return p;
    }
    static std::shared_ptr<GenericPriceEvaluator> plus(double v) {
        std::shared_ptr<GenericPriceEvaluator> p = std::make_shared<EvalOpen>(1, v);
        return p;
    }
    static std::shared_ptr<GenericPriceEvaluator> mul(double v) {
        std::shared_ptr<GenericPriceEvaluator> p = std::make_shared<EvalOpen>(2, v);
        return p;
    }
};

struct Order {
    OrderState state = OrderState::Waiting;
    int broker_id = 0;
    int asset;
    double price = 0;
    int volume = 0;
    double value = 0; // price*volume
    double fee = 0;   // Commission + tax
    bool processed = false;
    std::shared_ptr<GenericPriceEvaluator> price_eval = nullptr;
    ptime created_at, valid_from, valid_until, processed_at;
};

// Each broker has an OrderPool.
struct OrderPool {
    std::vector<Order> orders;
};

struct PortfolioItem {
    int position = 0;
    double prev_price = 0,
           prev_adj_price = 0; // Long 5 at price 10 -> investment=5*10+cost. Then sell 2 at price
                               // 11 -> investment doesn't change. So only record if same direction.
    ptime buying_time;         // Initial buying.

    double value = 0, profit = 0, dyn_adj_profit = 0, adj_profit;
    time_duration holding_time = hours(0);

    void update_value(const ptime &time, double new_price, double new_adj_price);
};
struct Portfolio {
    double cash = 0;
    std::map<int, PortfolioItem> portfolio_items;

    int position(int asset) const;
    double profit(int asset) const;

    VecArrXi positions(int total_assets) const;
    VecArrXd values(int total_assets) const;
    VecArrXd profits(int total_assets) const;

    void update(const Order &order, double adj_price);
};
#define BK_DEFINE_PORTFOLIO_MEMBER_ACCESSOR(var, type, default_val)                                \
    inline type Portfolio::var(int asset) const {                                                  \
        auto it = portfolio_items.find(asset);                                                     \
        if (it != portfolio_items.end()) {                                                         \
            return it->second.var;                                                                 \
        } else {                                                                                   \
            return default_val;                                                                    \
        }                                                                                          \
    }
BK_DEFINE_PORTFOLIO_MEMBER_ACCESSOR(position, int, 0);
BK_DEFINE_PORTFOLIO_MEMBER_ACCESSOR(profit, double, 0);
#undef BK_DEFINE_PORTFOLIO_MEMBER_ACCESSOR

#define BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(name, type, default_val)                           \
    inline type Portfolio::name##s(int total_assets) const {                                       \
        type res(total_assets);                                                                    \
        res.setConstant(default_val);                                                              \
        for (const auto &[k, v] : portfolio_items) {                                               \
            res.coeffRef(k) = v.name;                                                              \
        }                                                                                          \
        return res;                                                                                \
    }

BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(position, VecArrXi, 0)
BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(value, VecArrXd, 0)
BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(profit, VecArrXd, 0)
#undef BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR

void Portfolio::update(const Order &order, double adj_price) {
    int asset = order.asset;
    auto it = portfolio_items.find(asset);
    cash -= (order.value + order.fee);
    if (it != portfolio_items.end()) { // Found
        auto &item = it->second;
        // Buy
        if (order.volume > 0) {
            item.profit += (order.price - item.prev_price) * item.position;
            double adj_profit_diff = (adj_price - item.prev_adj_price) * item.position;
            item.adj_profit += adj_profit_diff;
            item.dyn_adj_profit += adj_profit_diff;
        } else {
            double cash_diff =
                (item.dyn_adj_profit - item.profit) * ((-order.volume) / item.position);
            cash += cash_diff;
            item.dyn_adj_profit -= cash_diff;
        }
        item.position += order.volume;
        item.value += order.value;
        if (item.position == 0) {
            portfolio_items.erase(it);
        }
    } else {
        portfolio_items[asset] = {.position = order.volume,
                                  .prev_price = order.price,
                                  .prev_adj_price = adj_price,
                                  .buying_time = order.processed_at,
                                  .value = order.value,
                                  .profit = -order.fee,
                                  .dyn_adj_profit = -order.fee,
                                  .adj_profit = -order.fee};
    }
}; // namespace backtradercpp

inline void PortfolioItem::update_value(const ptime &date, double new_price, double new_adj_price) {
    holding_time = date - buying_time;

    value = new_price * position;
    profit += (new_price - prev_price) * position;

    double adj_profit_diff = (new_adj_price - prev_adj_price) * position;
    adj_profit += adj_profit_diff;
    dyn_adj_profit += adj_profit_diff;

    prev_price = new_price;
    prev_adj_price = new_adj_price;
}

void OHLCData::resize(int assets) {
    for (auto &ele : {&open, &high, &low, &close}) {
        ele->resize(assets);
    }
}
void backtradercpp::OHLCData::reset() {
    for (auto &ele : {&open, &high, &low, &close}) {
        ele->setConstant(0);
    }
}
void FeedData::resize(int assets) {
    data.resize(assets);
    adj_data.resize(assets);
    volume.resize(assets);
    valid.resize(assets);
}
void backtradercpp::FeedData::reset() {
    data.reset();
    adj_data.reset();
    valid.setConstant(false);
}
void FeedData::validate_assets() {
    valid = (data.open > 0) && (data.high > 0) && (data.low > 0) && (data.close > 0);
}

#define BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, var, fun)                                \
    VecArrXd FullAssetData::fun(int time) const { return data_[window_ + time].data.var; };        \
    double FullAssetData::fun(int time, int stock) const {                                         \
        return data_[window_ + time].data.var.coeff(stock);                                        \
    }
#define BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(type1, type2, var)                                  \
    type1 FullAssetData::var(int time) const { return data_[window_ + time].var; };                \
    type2 FullAssetData::var(int time, int stock) const {                                          \
        return data_[window_ + time].var.coeff(stock);                                             \
    }

FullAssetData::FullAssetData(int assets, int window)
    : window_(window), assets_(assets), data_(window){};
backtradercpp::FullAssetData::FullAssetData(const FeedData &data, int window)
    : window_(window), data_(window), assets_(data.volume.size()) {
    data_.push_back(data);
}
const FeedData &FullAssetData::data(int time) const { return data_[time]; };

BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, open, open);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, high, high);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, low, low);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, close, close);

BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, open, adj_open);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, high, adj_high);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, low, adj_low);
BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, close, adj_close);

BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(VecArrXi, int, volume);
BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(VecArrXb, bool, valid);

void FullAssetData::set_window(int window) {
    window_ = window;
    data_.resize(window);
};
#undef BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR
#undef BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR
} // namespace backtradercpp