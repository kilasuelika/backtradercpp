# backtradercpp -- A header-only C++ 20 back testing library

As the name suggesting, this library is partially inspired by `backtrader` on Python. In my own use, `backtrader` constantly made me confusing so I decide to write my own library.

## ToDo

- [ ] Alignment between price and common data
- [ ] Multiple strategies support
- [ ] Strategy Optimizer
- [ ] Strategy data dump (not price data)
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

### Repeat run with modified settings

In this example, we compare performances between different commission rates.

```cpp
![[vs_examples/repeat_run/repeat_run.cpp]]
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
```

### Core logic

1. `Broker.hpp` - `BaseBrokerImpl.process()`: process orders.
