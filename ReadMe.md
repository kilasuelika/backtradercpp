# backtradercpp -- A header-only C++ 20 back testing library

As the name suggesting, this library is partially inspired by [backtrader](https://www.backtrader.com/) on Python. In my own use, backtrader constantly made me confusing so I decide to write my own library.

## ToDo

- [ ] Alignment between price and common data
- [ ] Multiple strategies support
- [ ] Strategy Optimizer
- [x] Strategy data dump (not price data)
- [x] History data to vector and matrix
- [ ] data().ret() and data().adj_ret()
- [ ] data().invalid_count(): count invalid data count in window

## Install

It's a header only library. But you need to install some dependencies. On windows:

```
./vcpkg install boost:x64-windows eigen3:x64-windows fmt:x64-windows libfort:x64-windows
```

## Example

See `vs_examples`.

### Most basic example

```cpp
#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;
using namespace std;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 6th day. Index starts from 0, so index 5 means 6th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::BaseBroker(0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)));
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

### Equal weight strategy

Use a weight vector to specify target asset value weights. Today's **open** will be used to calculate target volumes. `adjust_to_weight_target` is especially usefull for maching learning type strategies.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        if (time_index() % 30 == 0) {
            adjust_to_weight_target<100>(
                0, VecArrXd::Constant(assets(0),
                                      1. / assets(0))); // 100 means target volumes will be 100*k.
        }
    }
};

int main() {
    Cerebro cerebro;
    cerebro.add_broker(
        broker::BaseBroker(1000000, 0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)),
        2); // 2 for window
    cerebro.add_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
```

### A directory of CSVs and stop on loss or profit

Here each csv represents an asset. It contains time, open, high, low, close. An array is used to specify the column index for each variable in sequence.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Do nothing at initial 30 days.
        if (time_index() < 30) {
            return;
        }
        // If daily return larger than 0.05, then buy.
        for (int j = 0; j < data(0).assets(); ++j) {
            if (data(0).valid(-1, j)) {
                double p = data(0).close(-1, j), old_p = data(0).close(-2, j);
                if ((old_p > 0) && ((p / old_p) > 1.05))
                    // Issue an order of buy at the price of open on next day.
                    delayed_buy(0, j, EvalOpen::exact(), 10);
            }
        }
        // Sell on broker 0 if profits or loss reaching target.
        // Price is open of next day.
        for (const auto &[asset, item] : portfolio_items(0)) {
            if (item.profit > 1500 || item.profit < -1000) {
                close(0, asset, EvalOpen::exact());
            }
        }
    }
};

int main() {
    Cerebro cerebro;

    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)),
        2); // 2 for window
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

### Hedging and use extra data in CSVDirectoryData

Here we have two data sources: one for stocks and one for options. On options data source, we have some extra data.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : public strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        // extra_data(0).num(-1, "n");
        if (data_valid(0)) {
            const auto &pct_change = data(0).num("pct_change");
            fmt::print("{} pct_change: {}\n", codes(0)[0], pct_change.coeffRef(0));
        }
        if (data_valid(1)) {
            // fmt::print("code : {} , open: {}\n", codes(1)[0], data(1).close(-1, 0));
            util::cout("code: {}\n", codes(1)[0]);
        }
        // std::abort();
        if (time_index() % 30 == 0) {
            // fmt::print(fmt::fg(fmt::color::yellow), "Adjusting.\n");

            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    cerebro.add_broker(
        broker::BaseBroker(10000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData("../../example_data/CSVDirectory/raw",
                                             "../../example_data/CSVDirectory/adjust",
                                             std::array{2, 3, 5, 6, 4},
                                             feeds::TimeStrConv::delimited_date)
                          .extra_num_col({{7, "pct_change"}})
                          .set_code_extractor([](const std::string &code) {
                              return code.substr(code.size() - 13, 9);
                          })),
        2); // 2 for window
    cerebro.add_broker(broker::BaseBroker(100000, 0.0005, 0.001)
                           .set_feed(feeds::CSVDirPriceData(
                               "../../example_data/CSVDirectory/share_index_future",
                               std::array{2, 3, 5, 6, 4}, feeds::TimeStrConv::delimited_date)),
                       2);
    cerebro.add_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.set_range(date(2015, 6, 1), date(2022, 6, 1));
    cerebro.run();
}
```

### Option delta hedging and common data

In this example, stock data are simulated from geometric brownian motion and stored in a csv file. Option prices for each period are calculated from BSM and stored in another CSV file. There is an extra csv file for storing information of option pricing. Then in strategy, we buy options at begining and short stocks by delta periodicly.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;

    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets(0));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target(0, target_C);
        }

        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets(1));

            // Accessing read common data.
            double sigma = common_data(0).num(-1, "sigma");
            double K = common_data(0).num(-1, "K");
            double rf = common_data(0).num(-1, "rf");

            for (int i = 0; i < assets(1); ++i) {
                double S = data(1).close(-1, i);
                double T = common_data(0).num(-1, "time");

                double d1 =
                    (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                double delta = boost::math::cdf(boost::math::normal(), d1);

                target_S.coeffRef(i) = -int(delta * 100);
            }

            adjust_to_volume_target(1, target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(
        broker::BaseBroker(0).allow_default().set_feed(feeds::CSVTabPriceData(
            "../../example_data/Option/Option.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Stock price
    cerebro.add_broker(
        broker::BaseBroker(0).allow_short().set_feed(feeds::CSVTabPriceData(
            "../../example_data/Option/Stock.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonDataFeed("../../example_data/Option/OptionInfo.csv",
                                                     feeds::TimeStrConv::delimited_date),
                            window);

    cerebro.add_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
}
```

### Use dividen data

In this example, dividen data for stocks are add through `StockBroker.set_xrd_dir(dir, columns)`. In the dir, dividen for each stock are stored in separated files. `columns` is a vector of length 5 to specify column indices (0 start) for `record date` (登记日), `execution date` (除权除息日), `bonus` (送股), `additional` (转增股) and `dividen` (分红). The unit is 10 stocks. For example, `bonus=5` means if you have 1000 stocks, then you will get extra `1000/10*5=500` stocks.

```cpp
#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;
using namespace std;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 5th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::StockBroker(100000, 0.0005, 0.001)
            .set_feed(feeds::CSVDirPriceData("../../example_data/CSVDirectory/raw",
                                              "../../example_data/CSVDirectory/adjust",
                                              std::array{2, 3, 5, 6, 4},
                                              feeds::TimeStrConv::delimited_date)
                          .set_code_extractor([](const std::string &code) {
                              return code.substr(code.size() - 13, 9);
                          }))
            .set_xrd_dir("../../example_data/CSVDirectory/xrd", {7, 6, 2, 3, 4}),
        2);
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.set_log_dir("log");
    cerebro.run();

    auto performance = cerebro.performance();
    fmt::print("Sharepe Ratio: {}\n",
               performance[0].sharepe); // index 0 for whole. index 1 for broker 0, e.t.c.
}
```

### Repeat runs with modified settings

In this example, we compare performances between different commission rates.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;

    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets("option"));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target("option", target_C);
        }
        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets("stock"));

            // Accessing read common data.
            double sigma = common_data("option").num(-1, "sigma");
            double K = common_data("option").num(-1, "K");
            double rf = common_data("option").num(-1, "rf");

            for (int i = 0; i < assets("stock"); ++i) {
                double S = data("stock").close(-1, i);
                double T = common_data("option").num(-1, "time");

                double d1 =
                    (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                double delta = boost::math::cdf(boost::math::normal(), d1);

                target_S.coeffRef(i) = -int(delta * 100);
            }

            adjust_to_volume_target("stock", target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(broker::BaseBroker(0).allow_default().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Option.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("option")),
                       window);
    // Stock price
    cerebro.add_broker(broker::BaseBroker(0).allow_short().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Stock.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("stock")),
                       window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonDataFeed("../../example_data/Option/OptionInfo.csv",
                                                     feeds::TimeStrConv::delimited_date)
                                .set_name("option"),
                            window);

    cerebro.add_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.set_verbose(VerboseLevel::OnlySummary);
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
    double profit0 =
        cerebro.performance()[0]
            .profit; // index 0 for performance of overall portfolio. Index 1 is for the 0th broker.

    // Run once more.
    cerebro.reset();
    cerebro.broker("stock").set_commission_rate(0.001, 0.001);
    cerebro.set_log_dir("log1");
    cerebro.run();
    double profit1 = cerebro.performance()[0].profit;

    fmt::print("Profits under 0 and 0.001 commission rate: {}, {}\n", profit0, profit1);
}
```

### Dump data

This library provides some facilities for automatic data dump management. So when the first run, you can dump calculated data, then for following runs you can access dumped data to avoid repeat calculation. Note that price and common data are not dumped. Use `set_timed_vec()` to set values, and `get_timed_vec()` to get data. There are other function such as `set_var(), set_timed_var()` for double value, `set_mat(), set_timed_mat()` for 2D `RowArrayXd`. Here `_timed_` means data are indexed by time so in each period you get different data. If you want to use dump, then you need to `set_dump_file(filename, read_dump=true)`. `read_dump` means whether to read dump from file. If you only want to overwrite existing dump, then set it to `false`. Note that all numeric data use double values, so you need to cast if original data is `int`.

Run the following program two times. You will find that in the first time, it prints "Calculating data." while in the second, it prints "Using dumped data.".

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;
    void init() override { set_dump_file("dump.bin"); }
    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets("option"));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target("option", target_C);
        }
        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S;
            if (auto v = get_timed_vec("delta")) {
                fmt::print("Using dumped data.\n");

                target_S = v->cast<int>();
            } else {
                fmt::print("Calculating data.\n");

                target_S.resize(assets("stock"));

                // Accessing read common data.
                double sigma = common_data("option").num(-1, "sigma");
                double K = common_data("option").num(-1, "K");
                double rf = common_data("option").num(-1, "rf");

                for (int i = 0; i < assets("stock"); ++i) {
                    double S = data("stock").close(-1, i);
                    double T = common_data("option").num(-1, "time");

                    double d1 =
                        (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                    double delta = boost::math::cdf(boost::math::normal(), d1);

                    target_S.coeffRef(i) = -int(delta * 100);
                }
                set_timed_vec(
                    "delta",
                    target_S
                        .cast<double>()); // store dumped data. vec is double type, so need to cast.
            }

            adjust_to_volume_target("stock", target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(broker::BaseBroker(0).allow_default().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Option.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("option")),
                       window);
    // Stock price
    cerebro.add_broker(broker::BaseBroker(0).allow_short().set_feed(
                           feeds::CSVTabPriceData("../../example_data/Option/Stock.csv",
                                                  feeds::TimeStrConv::delimited_date)
                               .set_name("stock")),
                       window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonDataFeed("../../example_data/Option/OptionInfo.csv",
                                                     feeds::TimeStrConv::delimited_date)
                                .set_name("option"),
                            window);

    cerebro.add_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.set_verbose(VerboseLevel::OnlySummary);
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
    double profit0 =
        cerebro.performance()[0]
            .profit; // index 0 for performance of overall portfolio. Index 1 is for the 0th broker.
}
```

### Optimize strategy

An `optimize_strategy` function is provided to optimize a strategy with tabular search. You need to use `get_var()` to get variable value. Then `optimize_strategy` will set those values for repeat runs.

```cpp
#include "../../include/backtradercpp/Cerebro.hpp"
#include <boost/math/distributions.hpp>
using namespace backtradercpp;

struct DeltaOptionHedgingStrategy : public strategy::GenericStrategy {
    explicit DeltaOptionHedgingStrategy(int period = 1) : period(period) {}

    int period = 1;

    void run() override {

        // Buy options at the first time.
        if (time_index() == 0) {
            VecArrXi target_C(assets(0));
            target_C.setConstant(100); // Buy 100 options for each path.
            adjust_to_volume_target(0, target_C);
        }

        // data(0).close(-1);
        std::cout << data(0).close(Sel::All, 0).transpose() << std::endl;
        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets(1));

            // Accessing read common data.
            double sigma = common_data(0).num(-1, "sigma");
            double K = common_data(0).num(-1, "K");
            double rf = common_data(0).num(-1, "rf");

            for (int i = 0; i < assets(1); ++i) {
                double S = data(1).close(-1, i);
                double T = common_data(0).num(-1, "time");

                double d1 =
                    (std::log(S / K) + (rf + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
                double delta = boost::math::cdf(boost::math::normal(), d1);

                target_S.coeffRef(i) = -int(delta * 100);
            }

            adjust_to_volume_target(1, target_S);
        }
    }
};

int main() {
    Cerebro cerebro;
    // code_extractor: extract code form filename.
    int window = 2;
    // Option price.
    cerebro.add_broker(
        broker::BaseBroker(0).allow_default().set_feed(feeds::CSVTabPriceData(
            "../../example_data/Option/Option.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Stock price
    cerebro.add_broker(
        broker::BaseBroker(0).allow_short().set_feed(feeds::CSVTabPriceData(
            "../../example_data/Option/Stock.csv", feeds::TimeStrConv::delimited_date)),
        window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonDataFeed("../../example_data/Option/OptionInfo.csv",
                                                     feeds::TimeStrConv::delimited_date),
                            window);

    cerebro.add_strategy(std::make_shared<DeltaOptionHedgingStrategy>());
    cerebro.set_log_dir("log");
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
    // cerebro.set_verbose(None);

    // Cerebro cerebro1 = cerebro.clone();
    // cerebro1.reset();
    // cerebro1.run();
}
```

## Important Notes

1. The library will read data row by row. So you must sort data before running. Also note that currently data sources doesn't deal with thousands separator. Please preprocess data before.

2. Link to `OpenMP` for accelerating.

3. If some asset prices are missing in middle, then portfolio value will be the last availiable one. In `Position.csv` of log file (after `set_log_dir()`), there will be a `State` column to indicate whether data is availiable.

4. Different data may have different time span, the library will align data automatically for you.

5. Internally, all assets data are stored in a vector even if some assets are not valid at that time. It may bring performance issues.

## Reference

### Data API used in strategy

```cpp
boost::posix_time::ptime time(); // Current time.
int time_index();  //Count of days (0 start).  

FullAssetData &data(int broker);   
 VecArrXd data(broker).open(int i=-1) const;  //-1 means latest (today) in window, -2 means previous day.
 double   data(broker).open(int i, int asset) const; //close of an asset.
 VecArrXd data(broker).open(Sel::All, int asset) const ; //Return last window of a specific asset as a vector.
 template <typename Ret = RowMatrixXd>
    Ret   data(broker).open(Sel::All, Sel::All) const;  //Return the last window of all assets as a mtrix, row is for time, col for assets.
                       high(), low(), close()    //similar.
                       adj_open(), adj_high(), adj_low(), adj_close();
 VecXrrXb data(broker).valid(int i=-1);  //If asset is valid.

//Number of assets.
int assets(int broker);  
double cash(int broker);

const VecArrXi &positions(int broker) ; //A full length vector (may contain 0 if didn't buy some assets) of position on each asset.
int position(int broker, int asset);
    values(), value();   //similar. Asset value under raw close price.
 adj_profits(), adj_profit(); //Accumulated profits under adjusted price. Note if you sell all of an asset. Then next time when you buy the asset, this value will be accumulated from 0.
```

Here `VecArrX*` is `eigen` type:

```cpp
using VecArrXd = Eigen::Array<double, Eigen::Dynamic, 1>;
using VecArrXi = Eigen::Array<int, Eigen::Dynamic, 1>;
using VecArrXb = Eigen::Array<bool, Eigen::Dynamic, 1>;
using RowArrayXd = Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
```



### Core logic

1. `Broker.hpp` - `BaseBrokerImpl.process()`: process orders.