#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <functional>
#include "util.hpp"
#include <algorithm>
#include <numeric>
#include <filesystem>
//#include "3rd_party/glob/glob.hpp"
//#include <ranges>

namespace backtradercpp {
namespace feeds {
// CSVTabularData: A single matrix that contains multiple assets and only one type price. At this
// case, OHLC are the same value and valume are implicitly very large.
// time_converter: convert a time string to standard format: 2020-01-01 01:00:00

struct TimeStrConv {
    using func_type = std::function<std::string(const std::string &)>;

    //"20100202"
    static std::string non_delimited_date(const std::string &date_str) {
        std::string res_date = "0000-00-00 00:00:00";
        res_date.replace(0, 4, std::string_view{date_str.data(), 4});
        res_date.replace(5, 2, std::string_view{date_str.data() + 4, 2});
        res_date.replace(8, 2, std::string_view{date_str.data() + 6, 2});
        return res_date;
    }
    static std::string delimited_date(const std::string &date_str) {
        std::string res_date = "0000-00-00 00:00:00";
        res_date.replace(0, 4, std::string_view{date_str.data(), 4});
        res_date.replace(5, 2, std::string_view{date_str.data() + 5, 2});
        res_date.replace(8, 2, std::string_view{date_str.data() + 8, 2});
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
    std::vector<std::string> codes_;
    bool finished_ = false;

    TimeStrConv::func_type time_converter_;
};

struct CSVRowParaser {
    inline static boost::escaped_list_separator<char> esc_list_sep{"", ",", "\"\'"};
    static std::vector<std::string> parse_row(const std::string &row);

    static double parse_double(const std::string &ele);
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

class CSVDirectoryData : public GenericData {
  public:
    // tohlc_map: column number of time, open, high, low, close
    CSVDirectoryData(const std::string &raw_data_dir,
                     std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                     TimeStrConv::func_type time_converter = nullptr);
    CSVDirectoryData(const std::string &raw_data_dir, const std::string &adj_data_dir,
                     std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                     TimeStrConv::func_type time_converter = nullptr);
    bool read() override;

  private:
    void init();

    std::string raw_data_dir, adj_data_dir;
    std::vector<std::string> raw_data_filenames, adj_data_filenames;

    // std::vector<std::filesystem::path> raw_file_names, adj_file_names;
    std::vector<std::ifstream> raw_files, adj_files;
    std::vector<std::string> raw_line_buffer, adj_line_buffer;
    std::vector<std::vector<std::string>> raw_parsed_buffer, adj_parsed_buffer;
    std::vector<std::array<double, 4>> raw_parsed_double_buffer, adj_parsed_double_buffer;
    std::array<int, 5> tohlc_map;

    std::vector<ptime> times;

    enum State { Valid, Invalid, Finished };
    std::vector<State> status; // Track if data in each file has been readed.
};
class FeedsAggragator {
  public:
    FeedsAggragator() = default;
    // FeedsAggragator(const std::vector<std::shared_ptr<feeds::GenericData>> &);
    const std::vector<FullAssetData> &datas() const { return data_; }
    const auto &datas_valid() const { return datas_valid_; }
    const FullAssetData &data(int i) const { return data_[i]; }
    void add_feed(std::shared_ptr<feeds::GenericData> feed);
    void init();
    auto finished() const { return finished_; }
    bool read();
    // const std::vector<FullAssetData> &get_and_read();
    auto data_ptr() { return &data_; }
    const auto &time() const { return time_; }

    int set_window(int src, int window) { data_[src].set_window(window); };

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
double backtradercpp::feeds::CSVRowParaser::parse_double(const std::string &ele) {
    double res = 0;
    try {
        res = boost::lexical_cast<double>(ele);
    } catch (...) {
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

backtradercpp::feeds::CSVDirectoryData::CSVDirectoryData(const std::string &raw_data_dir,
                                                         std::array<int, 5> tohlc_map,
                                                         TimeStrConv::func_type time_converter)
    : GenericData(time_converter), raw_data_dir(raw_data_dir), adj_data_dir(raw_data_dir),
      tohlc_map(tohlc_map) {
    init();
}
backtradercpp::feeds::CSVDirectoryData::CSVDirectoryData(const std::string &raw_data_dir,
                                                         const std::string &adj_data_dir,
                                                         std::array<int, 5> tohlc_map,
                                                         TimeStrConv::func_type time_converter)
    : GenericData(time_converter), raw_data_dir(raw_data_dir), adj_data_dir(adj_data_dir),
      tohlc_map(tohlc_map) {
    init();
}
void CSVDirectoryData::init() {
    for (const auto &entry : std::filesystem::directory_iterator(raw_data_dir)) {
        auto file_path = entry.path().filename();
        auto adj_file_path = std::filesystem::path(adj_data_dir) / file_path;
        if (!std::filesystem::exists(adj_file_path)) {
            fmt::print(fmt::fg(fmt::color::red),
                       "Adjust data file {} doesn't exist. Ignore asset {}.\n",
                       adj_file_path.string(), file_path.string());
        } else {
            raw_files.emplace_back(entry.path());
            adj_files.emplace_back(adj_file_path);

            raw_data_filenames.emplace_back(entry.path().string());
            adj_data_filenames.emplace_back(adj_file_path.string());

            ++assets_;
            codes_.emplace_back(file_path.string());
        }
    }
    next_.resize(assets_);
    times.resize(assets_);
    status.resize(assets_, State::Invalid);
    raw_line_buffer.resize(assets_);
    adj_line_buffer.resize(assets_);
    raw_parsed_buffer.resize(assets_);
    adj_parsed_buffer.resize(assets_);
    raw_parsed_double_buffer.resize(assets_);
    adj_parsed_double_buffer.resize(assets_);

    // Read header.
#pragma omp parallel for
    for (int i = 0; i < assets_; ++i) {
        std::getline(raw_files[i], raw_line_buffer[i]);
        std::getline(adj_files[i], adj_line_buffer[i]);
    }
}

bool backtradercpp::feeds::CSVDirectoryData::read() {

    next_.reset();
    // read data.
    for (int i = 0; i < assets_; ++i) {
        switch (status[i]) {
        case Finished:
            break;
        case Invalid:
            // Try read data from file.
            if (!raw_files[i].eof()) {
                std::getline(raw_files[i], raw_line_buffer[i]);
                raw_parsed_buffer[i] = CSVRowParaser::parse_row(raw_line_buffer[i]);
                if (raw_parsed_buffer[i].empty()) {
                    status[i] = State::Finished;
                    break;
                }
                std::getline(adj_files[i], adj_line_buffer[i]);
                adj_parsed_buffer[i] = CSVRowParaser::parse_row(adj_line_buffer[i]);
                const auto &[t1, t2] = std::make_tuple(raw_parsed_buffer[i][tohlc_map[0]],
                                                       adj_parsed_buffer[i][tohlc_map[0]]);
                if (t1 != t2) {
                    fmt::print(fmt::fg(fmt::color::red),
                               "data in raw data file {} and adjusted data file {} have different "
                               "dates: {} "
                               "and {}. Please check data. Now abort...\n",
                               raw_data_filenames[i], adj_data_filenames[i], t1, t2);
                    std::abort();
                }
                times[i] = boost::posix_time::time_from_string(time_converter_(t1));

                status[i] = State::Valid; // After reading data, set to valid.
            } else {
                status[i] = State::Finished;
            }
            break;
        case Valid:
            break;
        default:
            break;
        }
    }
    if (std::ranges::all_of(status, [](const auto &e) { return e == State::Finished; })) {
        return false;
    }
    // Find smallest time.
    auto it = std::ranges::min_element(times);
    if (it != times.end()) {
        next_.time = *it;
        // Read data.
        for (int i = 0; i < assets_; ++i) {
            if ((status[i] == State::Valid) && (times[i] == *it)) {
                for (int j = 0; j < 4; ++j) {
                    raw_parsed_double_buffer[i][j] =
                        CSVRowParaser::parse_double(raw_parsed_buffer[i][tohlc_map[j + 1]]);
                    adj_parsed_double_buffer[i][j] =
                        CSVRowParaser::parse_double(adj_parsed_buffer[i][tohlc_map[j + 1]]);
                }
                next_.data.open.coeffRef(i) = raw_parsed_double_buffer[i][0];
                next_.data.high.coeffRef(i) = raw_parsed_double_buffer[i][1];
                next_.data.low.coeffRef(i) = raw_parsed_double_buffer[i][2];
                next_.data.close.coeffRef(i) = raw_parsed_double_buffer[i][3];

                next_.adj_data.open.coeffRef(i) = adj_parsed_double_buffer[i][0];
                next_.adj_data.high.coeffRef(i) = adj_parsed_double_buffer[i][1];
                next_.adj_data.low.coeffRef(i) = adj_parsed_double_buffer[i][2];
                next_.adj_data.close.coeffRef(i) = adj_parsed_double_buffer[i][3];

                status[i] = State::Invalid; // After read data, set to invalid.
            }
        }
        next_.validate_assets();
        return true;
    } else {
        return false;
    }
}
} // namespace feeds
} // namespace backtradercpp