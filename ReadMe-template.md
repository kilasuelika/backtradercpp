# backtradercpp -- A header-only C++ 20 back testing library

As the name suggests, this library is partially inspired by `backtrader` of python. However `backtrader` constantly made me confusing so I decide to write my own library.

## Example
See `vs_examples`.
### Most basic example

```cpp
![[vs_examples/basic_1/basic_1.cpp]]
```

## Code Structure
1. Generic(FeedData) -> FeedAggragator(FullAssetData)