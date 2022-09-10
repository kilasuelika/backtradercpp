#pragma once
#include "Broker.hpp"
#include "DataFeeds.hpp"
namespace backtradercpp {
namespace analyzer {
class GenericAnalyzer {
  public:
    virtual void update();

  private:
    broker::BrokerAggragator *broker_agg_;
    feeds::FeedsAggragator *feeds_aggragator_;

    std::vector<ptime> times;
    VecArrXd wealth;
};
void GenericAnalyzer::update() { for (auto &broker : broker_agg_.) }
} // namespace analyzer
} // namespace backtradercpp