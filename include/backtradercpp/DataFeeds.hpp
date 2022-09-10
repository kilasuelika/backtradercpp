#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <functional>
#include "util.hpp"
#include <algorithm>

namespace backtradercpp {
namespace feeds {
// CSVTabularData: A single matrix that contains multiple assets and only one type price. At this
// case, OHLC are the same value and valume are implicitly very large.
// time_converter: convert a time string to standard format: 2020-01-01 01:00:00

struct TimeStrConv {
    using func_type = std::function<std::string(const std::string &)>;

    //"20100202"
    static std::string non_delimt_date(const std::string &date_str) {
        std::string res_date = "0000-00-00 00:00:00";
        res_date.replace(0, 4, std::string_view{date_str.data(), 4});
        res_date.replace(5, 2, std::string_view{date_str.data() + 4, 2});
        res_date.replace(8, 2, std::string_view{date_str.data() + 6, 2});
        return res_date;
    }
};

class FeedsAggragator;
class GenericData {

  public:
    GenericData() = default;
    GenericData(TimeStrConv::func_type time_converter) : time_converter_(time_converter){};
    virtual bool read() = 0; // Update next_row_;

    virtual FeedData get_and_read() {
        auto res = next_;
        read();
        return res;
    };
    const auto &data() const { return next_; }
    FeedData *data_ptr() { return &next_; }
    bool finished() const { return finished_; }
    int assets() const { return assets_; }

  protected:
    friend class FeedsAggragator;

    int assets_ = 0;
    FeedData next_;
    bool finished_ = false;

    TimeStrConv::func_type time_converter_;
};

struct CSVRowParaser {
    inline static boost::escaped_list_separator<char> esc_list_sep{"", ",", "\"\'"};
    static std::vector<std::string> parse_row(const std::string &row);
};
class CSVTabularData : public GenericData {
  public:
    CSVTabularData(const std::string &raw_data_file,
                   TimeStrConv::func_type time_converter = nullptr);
    // You have to ensure that two file have the same rows.
    CSVTabularData(const std::string &raw_data_file, const std::string &adjusted_data_file,
                   TimeStrConv::func_type time_converter = nullptr);
    bool read() override;
    void init();

  private:
    void cast_ohlc_data_(std::vector<std::string> &row_string, OHLCData &dest);

    std::ifstream raw_data_file_, adj_data_file_;
    std::string next_row_, adj_next_row_;
};
class FeedsAggragator {
  public:
    FeedsAggragator() = default;
    // FeedsAggragator(const std::vector<std::shared_ptr<feeds::GenericData>> &);
    const std::vector<FullAssetData> &datas() const { return data_; }
    const auto &datas_valid() const { return datas_valid_; }
    void add_feed(std::shared_ptr<feeds::GenericData> feed);
    void init();
    auto finished() const { return finished_; }
    bool read();
    // const std::vector<FullAssetData> &get_and_read();
    auto data_ptr() { return &data_; }
    const auto &time() const { return time_; }

  private:
    std::vector<std::shared_ptr<feeds::GenericData>> feeds_;
    std::vector<FeedData *> next_;
    std::vector<FullAssetData> data_;
    VecArrXi datas_valid_;
    bool finished_ = false;

    ptime time_;
};

struct TimeConverter {
    //"20100202"
    static std::string non_delimt_date(const std::string &date_str) {
        std::string res_date = "0000-00-00 00:00:00";
        res_date.replace(0, 4, std::string_view{date_str.data(), 4});
        res_date.replace(5, 2, std::string_view{date_str.data() + 4, 2});
        res_date.replace(8, 2, std::string_view{date_str.data() + 6, 2});
        return res_date;
    }
};

std::vector<std::string> CSVRowParaser::parse_row(const std::string &row) {
    boost::tokenizer<boost::escaped_list_separator<char>> tok(row, esc_list_sep);
    std::vector<std::string> res;
    for (auto beg = tok.begin(); beg != tok.end(); ++beg) {
        res.emplace_back(*beg);
    }
    return res;
}
CSVTabularData::CSVTabularData(const std::string &raw_data_file,
                               TimeStrConv::func_type time_converter)
    : GenericData(time_converter), raw_data_file_(raw_data_file), adj_data_file_(raw_data_file) {
    backtradercpp::util::check_path_exists(raw_data_file);
    init();
}
CSVTabularData::CSVTabularData(const std::string &raw_data_file,
                               const std::string &adjusted_data_file,
                               TimeStrConv::func_type time_converter)
    : GenericData(time_converter), raw_data_file_(raw_data_file),
      adj_data_file_(adjusted_data_file) {
    backtradercpp::util::check_path_exists(raw_data_file);
    backtradercpp::util::check_path_exists(adjusted_data_file);
    init();
}

void CSVTabularData::init() {

    // Read header
    std::string header;
    std::getline(raw_data_file_, header);
    std::getline(adj_data_file_, header);

    // Read one line and detect assets.
    auto row_string = CSVRowParaser::parse_row(header);
    assets_ = row_string.size() - 1;
    next_.resize(assets_);
    // read();
}
inline void CSVTabularData::cast_ohlc_data_(std::vector<std::string> &row_string, OHLCData &dest) {
    for (int i = 0; i < assets_; ++i) {
        // dest.open.coeffRef(i) = std::numeric_limits<double>::quiet_NaN();
        dest.open.coeffRef(i) = 0;
        try {
            dest.open.coeffRef(i) = boost::lexical_cast<double>(row_string[i + 1]);
        } catch (...) {
        }
    }
    dest.high = dest.low = dest.close = dest.open;
};
inline bool CSVTabularData::read() {
    std::getline(raw_data_file_, next_row_);
    if (raw_data_file_.eof()) {
        finished_ = true;
        return false;
    }
    auto row_string = CSVRowParaser::parse_row(next_row_);
    next_.time = boost::posix_time::time_from_string(time_converter_(row_string[0]));
    cast_ohlc_data_(row_string, next_.data);

    std::getline(adj_data_file_, adj_next_row_);
    row_string = CSVRowParaser::parse_row(adj_next_row_);
    cast_ohlc_data_(row_string, next_.adj_data);

    // Set volume to very large.
    next_.volume.setConstant(1e12);
    next_.validate_assets();

    return true;
}

inline bool FeedsAggragator::read() {
    for (int i = 0; i < feeds_.size(); ++i) {
        bool _get = feeds_[i]->read();
        if (_get) {
            data_[i].push_back(feeds_[i]->next_);
        } else {
        }
        datas_valid_[i] = _get;
    }
    time_ = data_[0].time();
    bool success = datas_valid_.any();
    finished_ = !success;
    return success;
}
// inline const std::vector<FullAssetData> &FeedsAggragator::get_and_read() {
//     read();
//     return data_;
// }
inline void FeedsAggragator::add_feed(std::shared_ptr<feeds::GenericData> feed) {
    feeds_.push_back(feed);
    next_.push_back(&(feed->next_));
    data_.emplace_back(feed->next_);
    datas_valid_.resize(feeds_.size());
}
} // namespace feeds
} // namespace backtradercpp