#pragma once
/*ZhouYao at 2022-09-10*/

#include "DataFeeds.hpp"
#include "Broker.hpp"
#include "Strategy.hpp"
#define SPDLOG_FMT_EXTERNAL
#include <spdlog/stopwatch.h>

// #include<chrono>
namespace backtradercpp {

enum class VerboseLevel { None, OnlySummary, AllInfo };
class Cerebro {
  public:
    // window is for strategy. DataFeed and Broker doesn't store history data.
    void add_broker(broker::BaseBroker broker, int window = 1);
    void add_common_data(feeds::BaseCommonDataFeed data, int window);

    // void init_feeds_aggrator_();
    void add_strategy(std::shared_ptr<strategy::GenericStrategy> strategy);
    void init_strategy();
    void set_range(const date &start, const date &end = date(boost::date_time::max_date_time));
    // Set a directory for logging.
    void set_log_dir(const std::string &dir);
    void set_verbose(VerboseLevel v) { verbose_ = v; };

    void run();
    void reset();

    auto broker(int broker);
    auto broker(const std::string &broker_name);

    const std::vector<PriceFeedDataBuffer> &datas() const { return price_feeds_agg_.datas(); }

    const auto &performance() const { return broker_agg_.performance(); }

    Cerebro clone();

  private:
    // std::vector<int> asset_broker_map_
    feeds::PriceFeedAggragator price_feeds_agg_;
    feeds::CommonFeedAggragator common_feeds_agg_;

    broker::BrokerAggragator broker_agg_;
    std::shared_ptr<strategy::GenericStrategy> strategy_;
    // strategy::FullAssetData data_;
    ptime start_{boost::posix_time::min_date_time}, end_{boost::posix_time::max_date_time};

    VerboseLevel verbose_ = VerboseLevel::OnlySummary;
};

void Cerebro::add_broker(broker::BaseBroker broker, int window) {
    // std::cout << "add_broker test started.." << std::endl; 
    price_feeds_agg_.add_feed(broker.feed());
    // std::cout << "add_broker test test01.." << std::endl;
    price_feeds_agg_.set_window(price_feeds_agg_.datas().size() - 1, window);
    // std::cout << "add_broker test test02.." << std::endl;
    broker_agg_.add_broker(broker);
    // std::cout << "add_broker test finished.." << std::endl;
}
void Cerebro::add_common_data(feeds::BaseCommonDataFeed data, int window) {
    common_feeds_agg_.add_feed(data);
    common_feeds_agg_.set_window(common_feeds_agg_.datas().size() - 1, window);
};

void Cerebro::add_strategy(std::shared_ptr<strategy::GenericStrategy> strategy) {
    strategy_ = strategy;
}

void Cerebro::init_strategy() {
    strategy_->init_strategy(&price_feeds_agg_, &common_feeds_agg_, &broker_agg_);
}

inline void Cerebro::set_range(const date &start, const date &end) {
    start_ = ptime(start);
    end_ = ptime(end);
}

void Cerebro::run() {
    if (verbose_ == VerboseLevel::AllInfo)
        fmt::print(fmt::fg(fmt::color::yellow), "Runnng strategy..\n");
    // fmt::print("Start init strategy..\n");
    init_strategy();
    // fmt::print("Finished init strategy..\n");
    while (!price_feeds_agg_.finished()) {
        spdlog::stopwatch sw;
        if (price_feeds_agg_.time() > end_) {
            break;
            }
        if (!price_feeds_agg_.read()){
            break;
        }
            
        common_feeds_agg_.read();
        if (price_feeds_agg_.time() >= start_) {
            if (verbose_ == VerboseLevel::AllInfo)
                try {

                // fmt::print(fmt::runtime("┌{0:─^{2}}┐\n"
                //                         "│{1: ^{2}}│\n"
                //                         "└{0:─^{2}}┘\n"),
                //            "", util::to_string(price_feeds_agg_.time()), 21);
                
            } catch (const std::exception& e) {
                std::cerr << "錯誤: - " << e.what() << '\n';

            }
                
            // fmt::print("{}\n", util::to_string(feeds_agg_.time()));
            // fmt::print("process old orders\n");
            broker_agg_.process_old_orders();
            // fmt::print("Start execute strategy_\n");
            auto order_pool = strategy_->execute();
            // fmt::print("Start process order_pool\n");
            broker_agg_.process(order_pool);
            // fmt::print("Start process_terms\n");
            broker_agg_.process_terms();
            // fmt::print("Start update_info\n");
            broker_agg_.update_info();
            // fmt::print("Finished update_info\n");
            

            if (verbose_ == VerboseLevel::AllInfo) {
                // fmt::print("cash: {:12.4f},  total_wealth: {:12.2f}\n", broker_agg_.total_cash(),
                           broker_agg_.total_wealth();
                // fmt::print("Using {} seconds.\n", util::sw_to_seconds(sw));
            }
            // fmt::print("The test number is: {:d}\n", 409);
        }
    }
    if (verbose_ == VerboseLevel::OnlySummary || verbose_ == VerboseLevel::AllInfo)
        broker_agg_.summary();
    strategy_->finish_all();
    // strategy_-
}

void Cerebro::reset() {
    price_feeds_agg_.reset();
    common_feeds_agg_.reset();
    broker_agg_.reset();
    strategy_->reset();
}

auto Cerebro::broker(int broker) { return broker_agg_.broker(broker); }
auto Cerebro::broker(const std::string &broker_name) { return broker_agg_.broker(broker_name); }
void Cerebro::set_log_dir(const std::string &dir) {
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    broker_agg_.set_log_dir(dir);
}

Cerebro Cerebro::clone() {
    Cerebro cerebro = *this;
    price_feeds_agg_ = price_feeds_agg_.clone();
    common_feeds_agg_ = common_feeds_agg_.clone();
    broker_agg_.sync_feed_agg(price_feeds_agg_);
    return cerebro;
}
}; // namespace backtradercpp
