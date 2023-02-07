# backtradercpp -- A header-only C++ 20 back testing library

As the name suggests, this library is partially inspired by `backtrader` of python. However `backtrader` constantly made me confusing so I decide to write my own library.

## Install

It's a header only library. However you need to install some dependencies. On windows:
```
./vcpkg install boost:x64-windows eigen3:x64-windows fmt:x64-windows
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
    cerebro.add_asset_data(
        feeds::CSVTabularData("../../example_data/CSVTabular/djia.csv",
                              feeds::TimeStrConv::non_delimited_date),
        broker::Broker(0.0005, 0.001));
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
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
    cerebro.add_asset_data(
        feeds::CSVTabularData("../../example_data/CSVTabular/djia.csv",
                              feeds::TimeStrConv::delimited_date),
        broker::Broker(1000000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
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

    cerebro.add_data(
        std::make_shared<feeds::CSVTabularDataImpl>("../../example_data/CSVTabular/djia.csv",
                                                    feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::BrokerImpl>(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
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
        //extra_data(0).num(-1, "n");
        if (data_valid(0)) {
            const auto &pct_change = data(0).num("pct_change");
            fmt::print("{} pct_change: {}\n", codes(0)[0], pct_change.coeffRef(0));
        }
        if (data_valid(1)) {
            //fmt::print("code : {} , open: {}\n", codes(1)[0], data(1).close(-1, 0));
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
    //code_extractor: extract code form filename.
    cerebro.add_asset_data(feeds::CSVDirectoryData("../../example_data/CSVDirectory/raw",
                                                   "../../example_data/CSVDirectory/adjust",
                                                   std::array{2, 3, 5, 6, 4},
                                                   feeds::TimeStrConv::delimited_date)
                           .extra_num_col({{7, "pct_change"}})
                           .code_extractor([](const std::string &code) {
                               return code.substr(code.size() - 13, 9);
                           }),
                           broker::Broker(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.add_asset_data(feeds::CSVDirectoryData(
                               "../../example_data/CSVDirectory/share_index_future",
                               std::array{2, 3, 5, 6, 4},
                               feeds::TimeStrConv::delimited_date)
                           ,
                           broker::Broker(100000, 0.0005, 0.001), 2);
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.set_range(date(2015, 6, 1), date(2022, 6, 1));
    cerebro.run();
}
```

### Option delta hedging and use common data

In this example, stock data are simulated from geometric brownian motion and stored in a csv file. Option prices for each period are calculated from BSM and stored in another CSV file. There is an extra csv file for storing information of option pricing. Then in strategy, we first buy options and short stocks with delta periodicly.

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
            target_C.setConstant(100);
            adjust_to_volume_target(0, target_C);
        }

        // Short stocks.
        if (time_index() % period == 0) {
            VecArrXi target_S(assets(1));

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
    cerebro.add_asset_data(feeds::CSVTabularData("../../example_data/Option/Option.csv",
                                                 feeds::TimeStrConv::delimited_date),
                           broker::Broker(0).allow_default(), window);
    // Stock price
    cerebro.add_asset_data(feeds::CSVTabularData("../../example_data/Option/Stock.csv",
                                                 feeds::TimeStrConv::delimited_date),
                           broker::Broker(0).allow_short(), window);
    // Information for option
    cerebro.add_common_data(feeds::CSVCommonData("../../example_data/Option/OptionInfo.csv",
                                                 feeds::TimeStrConv::delimited_date),
                            window);
    auto s = std::make_shared<DeltaOptionHedgingStrategy>();
    cerebro.set_strategy(s);
    cerebro.run();

    fmt::print(fmt::fg(fmt::color::yellow), "Exact profits: {}\n", -941686);
}
```

## Important Notes

1. Please use **backward adjusted** data (keep oldest value fixed and adjust following data). When you buy, use **raw price**. I developed an algorithm to deal with backward adjusted data. The core idea is to track the profits under raw price (profti) and adjusted price (dyn_adj_profit). Then the differen `adj_profit - profit` is profits due to external factors. Total wealth will be
```
total wealth = cash + asset value under raw price + (dyn_adj_profit - profit)
```

When you sell, a propotion of difference `dyn_adj_profit - profit` will be added to your cash. This means if price adjust is due to dividends, then they will be added to your cash after you sell assets. Although normally they will be direct send to you cash account. But dividens usually are small so I think delayed converting to cash won't too make much difference and I didn't come up with a better idea.

Due to forward adjuted prices (keep newest price fixed and adjust older data) may be negative, I'm not sure if my algorithm will work at this case.

2. The library will read data row by row. So you must sort data before running. Also note that currently data sources doesn't deal with thousands separator. Please preprocess data before.

3. Link to `OpenMP` to accelerate.

4. You need to fill missing data in middle otherwise there may be extra-ordinary loss, although the final wealth may won't be affected. 

5. Different data may have different time span, the library will align data automatically for you.

## Reference

### Data API used in strategy

```cpp
boost::posix_time::ptime time(); // Current time.
int time_index();  //Count of days.

FullAssetData &data(int broker);  	
	VecArrXd data(broker).open(int i=-1);  //-1 means latest (today) in window, -2 means previous day.
	double   data(broker).open(int i, int asset); //close of an asset.
	                      high(), low(), close()    //similar.
	                      adj_open(), adj_high(), adj_low(), adj_close();
	VecXrrXb data(broker).valid(int i=-1);  //If asset is valid.

int assets(int broker);  //Number of assets.
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
```

### 

## Code Structure
1. FeedData(ohld_data, num_data_, str_data_) -> Generic(FeedData) -> FeedAggragator(FullAssetData)