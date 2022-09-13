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

## Important Notes

1. Please use **backward adjusted** data (keep oldest value fixed and adjust following data). When you buy, use **raw price**. I developed an algorithm to deal with backward adjusted data. The core idea is to track the profits under raw price (profti) and adjusted price (dyn_adj_profit). Then the differen `adj_profit - profit` is profits due to external factors. Total wealth will be
```
total wealth = cash + asset value under raw price + (dyn_adj_profit - profit)
```

When you sell, a propotion of difference `dyn_adj_profit - profit` will be added to your cash.

Due to forward adjuted prices (keep newest price fixed and adjust older data) may be negative, I'm not sure if my algorithm will work at this case.

## Reference

### Data API used in strategy

```cpp
int time_index();  //Count of days.

FullAssetData &data(int broker);  	
	VecArrXd data(broker).open(int i=-1);  //-1 means latest (today) in window, -2 means previous day.
	double   data(broker).open(int i, int asset); //close of an asset.
	                      high(), low(), close()    //similar.
	                      adj_open(), adj_high(), adj_low(), adj_close()

int assets(int broker);  //Number of assets.
double cash(int broker);

const VecArrXi &positions(int broker) ; //A full length vector record position on each asset.
int position(int broker, int asset);
    values(), value()   //similar. Asset value under raw close price.

```
Here `VecArrXd` is `eigen` type:
```cpp
Eigen::Array<double, Eigen::Dynamic, 1>
```

### 

## Code Structure
1. Generic(FeedData) -> FeedAggragator(FullAssetData)