#pragma once
/*ZhouYao at 2022-09-10*/

#include "3rd_party/eigen3/Eigen/Core"
#include <boost/circular_buffer.hpp>
#include <concepts>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost\date_time\gregorian\gregorian.hpp>
#include <queue>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <fort.hpp>

#include "util.hpp"

namespace backtradercpp {
    using VecArrXd = Eigen::Array<double, Eigen::Dynamic, 1>;
    using VecArrXi = Eigen::Array<int, Eigen::Dynamic, 1>;
    using VecArrXb = Eigen::Array<bool, Eigen::Dynamic, 1>;
    using RowArrayXd = Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    using boost::gregorian::date;
    using boost::gregorian::date_duration;
    using boost::gregorian::days;

    using boost::posix_time::hours;
    using boost::posix_time::ptime;
    using boost::posix_time::time_duration;

    enum OrderType { Market, Limit };
    enum OrderState { Success, Waiting, Expired };
    enum class Sel { All };

    struct OHLCData {
        VecArrXd open, high, low, close, ret, adj_ret;
        void resize(int assets);
        void reset();
    };
    struct PriceFeedData {
        PriceFeedData() {}
        explicit PriceFeedData(int assets);

        boost::posix_time::ptime time = boost::date_time::min_date_time;
        OHLCData data, adj_data;
        VecArrXi volume;
        VecArrXb valid;

        std::unordered_map<std::string, VecArrXd> num_data_;
        std::unordered_map<std::string, std::vector<std::string>> str_data_;

        void validate_assets();
        void resize(int assets);

        void reset();

    };
    struct CommonFeedData {
        boost::posix_time::ptime time;
        std::unordered_map<std::string, double> num_data_;
        std::unordered_map<std::string, std::string> str_data_;
    };
    template <typename T> class FeedDataBuffer {
    public:
        FeedDataBuffer() = default;
        FeedDataBuffer(int window) : window_(window) { set_window(window_); }

        const T& data(int time = -1) const { return data_[time]; }
        const auto& datas() const { return data_; }

        int window() const { return window_; }
        void set_window(int window) {
            window_ = window;
            data_.set_capacity(window);
        }
        virtual void push_back(const T& new_data) { data_.push_back(new_data); }
        virtual void push_back_() = 0;

        const auto& num(int k, const std::string& name) const;
        const auto& num(const std::string& name) const;
        const auto& num() const;
        const auto& str(int k, const std::string& name) const;
        const auto& str(const std::string& name) const;
        const auto& str() const;

    protected:
        int window_ = 1;
        boost::circular_buffer<T> data_;
        bool valid_ = false;
    };

    class PriceFeedDataBuffer : public FeedDataBuffer<PriceFeedData> {
    public:
        PriceFeedDataBuffer() = default;
        PriceFeedDataBuffer(int assets, int window = 1);
        PriceFeedDataBuffer(const PriceFeedData& data, int window = 1);

        VecArrXd open(int time = -1) const; // negative indices, -1 for latest
        double open(int time, int stock) const;
        VecArrXd open(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret open(Sel r, Sel c) const;

        VecArrXd high(int time = -1) const;
        double high(int time, int stock) const;
        VecArrXd high(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret high(Sel r, Sel c) const;

        VecArrXd low(int time = -1) const;
        double low(int time, int stock) const;
        VecArrXd low(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret low(Sel r, Sel c) const;

        VecArrXd close(int time = -1) const;
        double close(int time, int stock) const;
        VecArrXd close(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret close(Sel r, Sel c) const;

        VecArrXd adj_open(int time = -1) const;
        double adj_open(int time, int stock) const;
        VecArrXd adj_open(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret adj_open(Sel r, Sel c) const;

        VecArrXd adj_high(int time = -1) const;
        double adj_high(int time, int stock) const;
        VecArrXd adj_high(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret adj_high(Sel r, Sel c) const;

        VecArrXd adj_low(int time = -1) const;
        double adj_low(int time, int stock) const;
        VecArrXd adj_low(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret adj_low(Sel r, Sel c) const;

        VecArrXd adj_close(int time = -1) const;
        double adj_close(int time, int stock) const;
        VecArrXd adj_close(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret adj_close(Sel r, Sel c) const;

        VecArrXi volume(int time = -1) const;
        int volume(int time, int stock) const;
        VecArrXb valid(int time = -1) const;
        bool valid(int time, int stock) const;

        VecArrXd ret(int time = -1) const;
        double ret(int time, int stock) const;
        VecArrXd ret(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret ret(Sel r, Sel c) const;

        VecArrXd adj_ret(int time = -1) const;
        double adj_ret(int time, int stock) const;
        VecArrXd adj_ret(Sel s, int stock) const;
        template <typename Ret = RowArrayXd> Ret adj_ret(Sel r, Sel c) const;

        // template <typename T>
        // requires requires { std::is_same_v<T, FeedData>; }
        // calculate ret
        void push_back(const PriceFeedData& new_data) override
        {
            data_.push_back(new_data);
            if (data_.size() > 1)
            {
                auto& last_feed = data_[data_.size() - 2];
                auto& feed = data_.back();
                for (int i = 0; i < feed.data.close.size(); ++i)
                {
                    auto last_close = last_feed.data.close.coeff(i);
                    if (last_close != 0)
                    {
                        feed.data.ret.coeffRef(i) = feed.data.close[i] / last_close - 1;
                    }
                    else
                    {
                        feed.data.ret.coeffRef(i) = 0;
                    }
                    auto last_adj_close = last_feed.adj_data.ret.coeff(i);
                    if (last_adj_close != 0)
                    {
                        feed.adj_data.ret.coeffRef(i) = feed.adj_data.close[i] / last_adj_close - 1;
                    }
                    else
                    {
                        feed.adj_data.ret.coeffRef(i) = 0;
                    }
                }
            }
        }
        void push_back_() override { data_.push_back(PriceFeedData(assets_)); }

        auto assets() const { return assets_; }
        const auto& time() const { return data_.back().time; }

        // const auto &num(int k, const std::string &name) const;
        // const auto &num(const std::string &name) const;
        // const auto &str(int k, const std::string &name) const;
        // const auto &str(const std::string &name) const;

    private:
        int assets_ = 0;
    };

    class CommonFeedDataBuffer : public FeedDataBuffer<CommonFeedData> {
    public:
        CommonFeedDataBuffer() = default;
        CommonFeedDataBuffer(int window) : FeedDataBuffer(window) {}
        CommonFeedDataBuffer(const CommonFeedData& data, int window = 1) : FeedDataBuffer(window) {
            data_.push_back(data);
        }

        void push_back_() override {
            std::cout << "Common data" << std::endl;
            data_.push_back(CommonFeedData());
        }
    };

    struct PriceEvaluatorInput {
        double open, high, low, close;
    };
    struct GenericPriceEvaluator {
        virtual double price(const PriceEvaluatorInput& input) = 0;
        virtual ~GenericPriceEvaluator() = default;
    };
    struct EvalOpen : GenericPriceEvaluator {
        int tag = 0; // 0:exact, 1: open+v, 2:open*v
        double v = 0;
        EvalOpen(int tag_, int v_) : tag(tag_), v(v_) {}
        double price(const PriceEvaluatorInput& input) override {
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

        double value = 0, profit = 0, dyn_adj_profit = 0, adj_profit = 0;
        time_duration holding_time = hours(0);

        void update_value(const ptime& time, double new_price, double new_adj_price);
    };
    struct Portfolio {
        double cash = 0;
        std::map<int, PortfolioItem> portfolio_items;
        double holding_value = 0;
        double total_value = 0;

        int position(int asset) const;
        double profit(int asset) const;

        VecArrXi positions(int total_assets) const;
        VecArrXd values(int total_assets) const;
        VecArrXd profits(int total_assets) const;
        VecArrXd adj_profits(int total_assets) const;
        VecArrXd dyn_adj_profits(int total_assets) const;

        void update(const Order& order, double adj_price);
        void update_info();

        void transfer_stock(ptime time, int asset, int volume);
        void transfer_cash(int cash);

        void reset() { *this = Portfolio(); }
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
        BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(adj_profit, VecArrXd, 0)
        BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR(dyn_adj_profit, VecArrXd, 0)
#undef BK_DEFINE_PORTFOLIO_MEMBER_VEC_ACCESSOR

#define BK_DEFINE_FeedDataBuffer_EXTRA_ACCESSOS(name)                                              \
    template <typename T>                                                                          \
    const auto &FeedDataBuffer<T>::name(int k, const std::string &name_) const {                   \
        return data_[window_ + k].name##_data_.at(name_);                                          \
    }                                                                                              \
    template <typename T> const auto &FeedDataBuffer<T>::name(const std::string &name_) const {    \
        return data_.back().name##_data_.at(name_);                                                \
    }                                                                                              \
    template <typename T> const auto &FeedDataBuffer<T>::name() const {                            \
        return data_.back().name##_data_;                                                          \
    }

        BK_DEFINE_FeedDataBuffer_EXTRA_ACCESSOS(num);
    BK_DEFINE_FeedDataBuffer_EXTRA_ACCESSOS(str);
#undef BK_DEFINE_FULLASSETDATA_EXTRA_ACCESSOS

    inline void Portfolio::update(const Order& order, double adj_price) {
        int asset = order.asset;
        auto it = portfolio_items.find(asset);
        cash -= (order.value + order.fee);
        if (it != portfolio_items.end()) { // Found
            auto& item = it->second;
            // Buy
            if (order.volume > 0) {
                item.profit += (order.price - item.prev_price) * item.position;
                double adj_profit_diff = (adj_price - item.prev_adj_price) * item.position;
                item.adj_profit += adj_profit_diff;
                item.dyn_adj_profit += adj_profit_diff;
            }
            else {
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
        }
        else {
            portfolio_items[asset] = { .position = order.volume,
                                      .prev_price = order.price,
                                      .prev_adj_price = adj_price,
                                      .buying_time = order.processed_at,
                                      .value = order.value,
                                      .profit = -order.fee,
                                      .dyn_adj_profit = -order.fee,
                                      .adj_profit = -order.fee };
        }
    }

    inline void backtradercpp::Portfolio::update_info() {
        total_value = cash;
        for (const auto& [asset, item] : portfolio_items) {
            total_value += item.value;
        }
    }

    inline void PortfolioItem::update_value(const ptime& date, double new_price, double new_adj_price) {
        holding_time = date - buying_time;

        value = new_price * position;
        profit += (new_price - prev_price) * position;

        double adj_profit_diff = (new_adj_price - prev_adj_price) * position;
        adj_profit += adj_profit_diff;
        dyn_adj_profit += adj_profit_diff;

        prev_price = new_price;
        prev_adj_price = new_adj_price;
    }

    inline void OHLCData::resize(int assets) {
        for (auto& ele : { &open, &high, &low, &close, &ret }) {
            ele->resize(assets);
        }
    }
    inline void OHLCData::reset() {
        for (auto& ele : { &open, &high, &low, &close, &ret }) {
            ele->setConstant(0);
        }
    }

    inline PriceFeedData::PriceFeedData(int assets) { valid = VecArrXb::Constant(assets, false); }

    inline void PriceFeedData::resize(int assets) {
        data.resize(assets);
        adj_data.resize(assets);
        volume.resize(assets);
        valid.resize(assets);
    }
    inline void backtradercpp::PriceFeedData::reset() {
        data.reset();
        adj_data.reset();
        valid.setConstant(false);

        util::reset_value(num_data_, 0.0);
        util::reset_value(str_data_, std::string());
    }
    inline void PriceFeedData::validate_assets() {
        valid = (data.open > 0) && (data.high > 0) && (data.low > 0) && (data.close > 0);
    }

#define BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, var, fun)                                \
    inline VecArrXd PriceFeedDataBuffer::fun(int time) const {                                     \
        return data_[window_ + time].data.var;                                                     \
    };                                                                                             \
    inline double PriceFeedDataBuffer::fun(int time, int stock) const {                            \
        return data_[window_ + time].data.var.coeff(stock);                                        \
    }                                                                                              \
    inline VecArrXd PriceFeedDataBuffer::fun(Sel s, int stock) const {                             \
        VecArrXd res(window_);                                                                     \
        for (int i = 0; i < data_.size(); ++i) {                                                   \
            res.coeffRef(i) = data_[i].data.var.coeff(stock);                                      \
        }                                                                                          \
        return res;                                                                                \
    }                                                                                              \
    template <typename Ret> inline Ret PriceFeedDataBuffer::fun(Sel r, Sel c) const {              \
        Ret res(window_, assets_);                                                                 \
        for (int i = 0; i < data_.size(); ++i) {                                                   \
            res.row(i) = data_[i].data.var.transpose();                                            \
        }                                                                                          \
        return res;                                                                                \
    }
#define BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(type1, type2, var)                                  \
    inline type1 PriceFeedDataBuffer::var(int time) const { return data_[window_ + time].var; };   \
    inline type2 PriceFeedDataBuffer::var(int time, int stock) const {                             \
        return data_[window_ + time].var.coeff(stock);                                             \
    }

    inline PriceFeedDataBuffer::PriceFeedDataBuffer(int assets, int window)
        : FeedDataBuffer(window), assets_(assets) {
    };
    inline backtradercpp::PriceFeedDataBuffer::PriceFeedDataBuffer(const PriceFeedData& data,
        int window)
        : FeedDataBuffer(window), assets_(data.volume.size()) {
        data_.push_back(data);
    }
    // const FeedData &PriceFeedDataBuffer::data(int time) const

    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, open, open);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, high, high);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, low, low);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, close, close);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(data, ret, ret);

    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, open, adj_open);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, high, adj_high);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, low, adj_low);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, close, adj_close);
    BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR(adj_data, ret, adj_ret);

    BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(VecArrXi, int, volume);
    BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR(VecArrXb, bool, valid);

    // template <typename T> void FeedDataBuffer<T>::set_window(int window)
#undef BK_DEFINE_STRATEGYDATA_OHLC_MEMBER_ACCESSOR
#undef BK_DEFINE_STRATEGYDATA_MEMBER_ACCESSOR

    inline void Portfolio::transfer_stock(ptime time, int asset, int volume) {
        auto it = portfolio_items.find(asset);
        if (it != portfolio_items.end()) { // Found
            auto& item = it->second;
            item.position += volume;
        }
        else {
            PortfolioItem item;
            item.position = volume;
            item.buying_time = time;
            portfolio_items[asset] = item;
        }
    }

    inline void Portfolio::transfer_cash(int cash) { this->cash += cash; }
} // namespace backtradercpp