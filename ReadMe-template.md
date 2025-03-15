# backtradercpp -- A header-only C++ 20 back testing library

As the name suggesting, this library is partially inspired by [backtrader](https://www.backtrader.com/) on Python. In my own use, backtrader constantly made me confusing so I decide to write my own library.

## Features

1. `backtrader`-like API, easy for porting code.
1. Multiple data source support like csv downladed from Yahoo, or generating data from random processes such as Geometric Brownian Motion..
1. Repeat run with modified settings and parameters. There is also an `TableRunner` which can be use to optimize strategy parameters by table search.
1. Strategy data dump and read. You can dump calculated data and read them in following runs to avoid repeat calculation. 

## ToDo

-   [ ] Alignment between price and common data
-   [ ] Strategy Optimizer
-   [x] Strategy data dump (not price data)
-   [x] History data to vector and matrix
-   [ ] data().ret(), data().adj_ret() require new fields. cal_ret() and cal_adj_ret() are calculated on demand.
-   [ ] data().invalid_count(): count invalid days in window, useful when don't want to trade stocks which has too many invalid data on previous days.
-   [x] RandomProcessDataFeeds for random process simulation and theoretic research.
-   [ ] `register_custom_metric(),update_custom_metric()`: Register custom data when storing results. e.g. for optimizing strategy with custom metric.
-   [ ] `require_indicator()`: Integrate financial indicator library like TA-Lib to register and calculate indicators on demand. `data(0).indicator(ID,-1,asset)`
-   [ ] Reinforcement learning support
-   [ ] Multiple strategies support, may not necessary

## Install

It's a header only library. But you need to install some dependencies. On windows:

```
./vcpkg install boost:x64-windows fmt:x64-windows libfort:x64-windows spdlog:x64-windows
```

## Example

See `vs_examples`.

### Most basic example

```cpp
![[vs_examples/basic_1/basic_1.cpp]]
```

### Equal weight strategy

Use a weight vector to specify target asset value weights. Today's **open** will be used to calculate target volumes. `adjust_to_weight_target` is especially usefull for maching learning type strategies.

```cpp
![[vs_examples/equal_weight/equal_weight.cpp]]
```

### A directory of CSVs and stop on loss or profit

Here each csv represents an asset. It contains time, open, high, low, close. An array is used to specify the column index for each variable in sequence.

```cpp
![[vs_examples/delayed_buy/delayed_buy.cpp]]
```

### Hedging and use extra data in CSVDirectoryData

Here we have two data sources: one for stocks and one for options. On options data source, we have some extra data.

```cpp
![[vs_examples/csv_directory_data_extra/csv_directory_data_extra.cpp]]
```

### Option delta hedging and common data

In this example, stock data are simulated from geometric brownian motion and stored in a csv file. Option prices for each period are calculated from BSM and stored in another CSV file. There is an extra csv file for storing information of option pricing. Then in strategy, we buy options at begining and short stocks by delta periodicly.

```cpp
![[vs_examples/option_hedging/option_hedging.cpp]]
```

### Use dividen data

In this example, dividen data for stocks are add through `StockBroker.set_xrd_dir(dir, columns)`. In the dir, dividen for each stock are stored in separated files. `columns` is a vector of length 5 to specify column indices (0 start) for `record date` (登记日), `execution date` (除权除息日), `bonus` (送股), `additional` (转增股) and `dividen` (分红). The unit is 10 stocks. For example, `bonus=5` means if you have 1000 stocks, then you will get extra `1000/10*5=500` stocks.

```cpp
![[vs_examples/stock_xrd/stock_xrd.cpp]]
```

### Repeat runs with modified settings

In this example, we compare performances between different commission rates.

```cpp
![[vs_examples/repeat_run/repeat_run.cpp]]
```

### Dump data

This library provides some facilities for automatic data dump management. So when the first run, you can dump calculated data, then for following runs you can access dumped data to avoid repeat calculation. Note that price and common data are not dumped. Use `set_timed_vec()` to set values, and `get_timed_vec()` to get data. There are other function such as `set_var(), set_timed_var()` for double value, `set_mat(), set_timed_mat()` for 2D `RowArrayXd`. Here `_timed_` means data are indexed by time so in each period you get different data. If you want to use dump, then you need to `set_dump_file(filename, read_dump=true)`. `read_dump` means whether to read dump from file. If you only want to overwrite existing dump, then set it to `false`. Note that all numeric data use double values, so you need to cast if original data is `int`.

Run the following program two times. You will find that in the first time, it prints "Calculating data." while in the second, it prints "Using dumped data.".

```cpp
![[vs_examples/dump_data/dump_data.cpp]]
```

### Optimize strategy

An `optimize_strategy` function is provided to optimize a strategy with tabular search. You need to use `get_var()` to get variable value. Then `optimize_strategy` will set those values for repeat runs.

```cpp
![[vs_examples/optimize_strategy/optimize_strategy.cpp]]
```

### Random process data generation

RandomProcessDataFeeds provide some random process generators like geometric brownian motion to generate price data, this can be useful when doing research.

```cpp
![[vs_examples/random_process_data/random_process_data.cpp]]
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
