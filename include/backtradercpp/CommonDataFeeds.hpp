#pragma once
/*ZhouYao at 2023-03-15*/

#include "DataFeeds.hpp"
namespace backtradercpp {
namespace feeds {

class GenericCommonDataImpl {

  public:
    GenericCommonDataImpl() = default;

    GenericCommonDataImpl(TimeStrConv::func_type time_converter)
        : time_converter_(time_converter){};
    virtual bool read() = 0; // Update next_row_;

    virtual CommonFeedData get_and_read() {
        auto res = next_;
        read();
        return res;
    }
    const auto &data() const { return next_; }
    CommonFeedData *data_ptr() { return &next_; }
    bool finished() const { return finished_; }
    int assets() const { return assets_; }

  protected:
    friend class FeedsAggragator;

    int assets_ = 0;
    CommonFeedData next_;
    bool finished_ = false;

    TimeStrConv::func_type time_converter_;
};

// For common data, e.g. market indices.
class CSVCommonTabularData : public GenericDataImpl {
    CSVCommonTabularData(const std::string &raw_data_file,
                         TimeStrConv::func_type time_converter = nullptr) {}
    bool read() override;
    void init();

  private:
    std::ifstream raw_data_file_;
    std::string next_row_;
};
CSVCommonTabularData::CSVCommonTabularData(const std::string &raw_data_file,
                                           TimeStrConv::func_type time_converter)
    : GenericDataImpl(time_converter), raw_data_file_(raw_data_file) {
    backtradercpp::util::check_path_exists(raw_data_file);
    init();
}
} // namespace feeds
} // namespace backtradercpp