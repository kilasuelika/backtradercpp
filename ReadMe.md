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
        if (i == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
        ++i;
    }

    int i = 1;
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_data(
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::Broker>(0.0005, 0.001),);
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

### Equal weight strategy

```
#include "../../include/backtradercpp/Cerebro.hpp"
using namespace backtradercpp;

struct EqualWeightStrategy : strategy::GenericStrategy {
    void run() override {
        // Re-adjust to equal weigh each 30 trading days.
        if (time_index() % 30 == 0) {
            adjust_to_weight_target(0, VecArrXd::Constant(assets(0), 1. / assets(0)));
        }
    }
};

int main() {
    Cerebro cerebro;
    cerebro.add_data(
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::delimited_date),
        std::make_shared<broker::Broker>(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
```

### Stop on loss and profit

```
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
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::Broker>(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

## Important Notes

1. Please use **backward adjusted** data (keep oldest value fixed and adjust following data). When you buy, use **raw price**. I developed an algorithm to deal with backward adjusted data. The core idea is to track the profits under raw price (profti) and adjusted price (dyn_adj_profit). Then the differen `adj_profit - profit` is profits due to external factors. Total wealth will be
```
total wealth = cash + asset value under raw price + (dyn_adj_profit - profit)
```
When you sell, a propotion of difference `dyn_adj_profit - profit` will be added to your cash.

Due to forward adjuted prices (keep newest price fixed and adjust older data) may be negative, I'm not sure if my algorithm will work at this case.
## Code Structure
1. Generic(FeedData) -> FeedAggragator(FullAssetData)# backtradercpp -- A header-only C++ 20 back testing library

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
        if (i == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
        ++i;
    }

    int i = 1;
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_data(
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::Broker>(0.0005, 0.001),);
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

### Equal weight strategy

Use a weight vector to specify target asset value weights. Today's **open** will be used to calculate target volumes. `adjust_to_weight_target` is especially usefull for maching learning type strategies.

```
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
    cerebro.add_data(
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::delimited_date),
        std::make_shared<broker::Broker>(1000000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<EqualWeightStrategy>());
    cerebro.run();
}
```

### A directory of CSVs and stop on loss or profit

Here each csv represents an asset. It contains time, open, high, low, close.

```
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
        std::make_shared<feeds::CSVTabularData>("../../example_data/CSVTabular/djia.csv",
                                                feeds::TimeStrConv::non_delimited_date),
        std::make_shared<broker::Broker>(10000, 0.0005, 0.001), 2); // 2 for window
    cerebro.set_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}
```

## Important Notes

1. Please use **backward adjusted** data (keep oldest value fixed and adjust following data). When you buy, use **raw price**. I developed an algorithm to deal with backward adjusted data. The core idea is to track the profits under raw price (profti) and adjusted price (dyn_adj_profit). Then the differen `adj_profit - profit` is profits due to external factors. Total wealth will be
```
total wealth = cash + asset value under raw price + (dyn_adj_profit - profit)
```
When you sell, a propotion of difference `dyn_adj_profit - profit` will be added to your cash.

Due to forward adjuted prices (keep newest price fixed and adjust older data) may be negative, I'm not sure if my algorithm will work at this case.
## Code Structure
1. Generic(FeedData) -> FeedAggragator(FullAssetData)