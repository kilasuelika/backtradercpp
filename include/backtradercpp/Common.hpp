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
};
struct FeedData {
    boost::posix_time::ptime time;
    OHLCData data, adj_data;
    VecArrXi volume;
    VecArrXb valid;
    void validate_assets();
    void resize(int assets);
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
struct PriceEvaluatorInput {
    double open, high, low, close;
};
struct GenericPriceEvaluator {
    virtual double price(const PriceEvaluatorInput &input) = 0;
    virtual ~GenericPriceEvaluator();
};
struct Order {
    OrderType type;
    int broker_id = 0;
    int asset;
    double price;
    int volume;
    bool processed = false;
    double commission = 0;
    double tax = 0;
    std::shared_ptr<GenericPriceEvaluator> price_eval = nullptr;
    boost::posix_time::ptime valid_from, valid_until, processed_at;
};

// Each broker has an OrderPool.
struct OrderPool {
    std::vector<Order> orders;
};
void OHLCData::resize(int assets) {
    for (auto &ele : {&open, &high, &low, &close}) {
        ele->resize(assets);
    }
}
void FeedData::resize(int assets) {
    data.resize(assets);
    adj_data.resize(assets);
    volume.resize(assets);
    valid.resize(assets);
}
void FeedData::validate_assets() {
    valid = (data.open > 0) || (data.high > 0) || (data.low > 0) || (data.close > 0);
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