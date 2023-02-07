#pragma once
/*ZhouYao at 2022-09-10*/

#include "Common.hpp"
#include "util.hpp"
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <functional>
#include "util.hpp"
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <unordered_set>
// #include "3rd_party/glob/glob.hpp"
// #include <ranges>

namespace backtradercpp {
namespace feeds {
// CSVTabularDataImpl: A single matrix that contains multiple assets and only one type price. At
// this case, OHLC are the same value and valume are implicitly very large. time_converter: convert
// a time string to standard format: 2020-01-01 01:00:00

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

template <typename DataT, typename FeedT, typename BufferT> class GenericFeedsAggragator;

enum State { Valid, Invalid, Finished };

template <typename T> class GenericDataImpl {
  public:
    GenericDataImpl() = default;
    GenericDataImpl(TimeStrConv::func_type time_converter) : time_converter_(time_converter){};

    virtual bool read() = 0; // Update next_row_;
    virtual T get_and_read() {
        auto res = next_;
        read();
        return res;
    }

    const auto &data() const { return next_; }
    T *data_ptr() { return &next_; }
    bool finished() const { return finished_; }
    const auto &next() const { return next_; }

  protected:
    TimeStrConv::func_type time_converter_;

    T next_;
    bool finished_ = false;
};

class GenericPriceDataImpl : public GenericDataImpl<PriceFeedData> {

  public:
    GenericPriceDataImpl() = default;
    GenericPriceDataImpl(TimeStrConv::func_type time_converter)
        : GenericDataImpl<PriceFeedData>(time_converter){};

    int assets() const { return assets_; }
    const auto &codes() const { return codes_; }

  protected:
    friend class FeedsAggragator;

    virtual void init() {
        util::cout("Price data initilizing finished. Total {} assets.\n", assets_);
    }

    int assets_ = 0;
    std::vector<std::string> codes_;
};

struct CSVRowParaser {
    inline static boost::escaped_list_separator<char> esc_list_sep{"", ",", "\"\'"};
    static std::vector<std::string> parse_row(const std::string &row);

    static double parse_double(const std::string &ele);
};

// For tabluar pricing data. OHLC are the same.
class CSVTabularDataImpl : public GenericPriceDataImpl {
  public:
    CSVTabularDataImpl(const std::string &raw_data_file,
                       TimeStrConv::func_type time_converter = nullptr);
    // You have to ensure that two file have the same rows.
    CSVTabularDataImpl(const std::string &raw_data_file, const std::string &adjusted_data_file,
                       TimeStrConv::func_type time_converter = nullptr);
    bool read() override;

  private:
    void init() override;
    void cast_ohlc_data_(std::vector<std::string> &row_string, OHLCData &dest);

    std::ifstream raw_data_file_, adj_data_file_;
    std::string next_row_, adj_next_row_;
};

class CSVDirectoryDataImpl : public GenericPriceDataImpl {
  public:
    // tohlc_map: column number of time, open, high, low, close
    CSVDirectoryDataImpl(const std::string &raw_data_dir,
                         std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                         TimeStrConv::func_type time_converter = nullptr);
    CSVDirectoryDataImpl(const std::string &raw_data_dir, const std::string &adj_data_dir,
                         std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                         TimeStrConv::func_type time_converter = nullptr);
    bool read() override;
    // void init_extra_data();

    // Column name will be read form files.
    CSVDirectoryDataImpl &extra_num_col(const std::vector<std::pair<int, std::string>> &cols);

    CSVDirectoryDataImpl &extra_str_col(const std::vector<std::pair<int, std::string>> &cols);

    CSVDirectoryDataImpl &code_extractor(std::function<std::string(std::string)> fun) {
        // this->code_extractor_ = fun;
        for (auto &ele : codes_) {
            ele = fun(ele);
        }
        return *this;
    }

  private:
    void init() override;
    std::string raw_data_dir, adj_data_dir;
    std::vector<std::string> raw_data_filenames, adj_data_filenames;

    // std::vector<std::filesystem::path> raw_file_names, adj_file_names;
    std::vector<std::ifstream> raw_files, adj_files;
    std::vector<std::string> raw_line_buffer, adj_line_buffer;
    std::vector<std::vector<std::string>> raw_parsed_buffer, adj_parsed_buffer;
    std::vector<std::array<double, 4>> raw_parsed_double_buffer, adj_parsed_double_buffer;
    std::array<int, 5> tohlc_map;

    std::vector<int> extra_num_col_, extra_str_col_;
    std::vector<std::string> extra_num_col_names_, extra_str_col_names_;
    std::vector<std::reference_wrapper<VecArrXd>> extra_num_data_ref_;
    std::vector<std::reference_wrapper<std::vector<std::string>>> extra_str_data_ref_;

    std::vector<ptime> times;

    /* std::function<std::string(std::string)> code_extractor_ = [](const std::string &e) {
         return e;
     };*/

    std::vector<State> status; // Track if data in each file has been readed.
};

class CSVCommonDataImpl : public GenericDataImpl<CommonFeedData> {
  public:
    CSVCommonDataImpl(const std::string &file, TimeStrConv::func_type time_converter,
                      const std::vector<int> str_cols);
    bool read() override;

  private:
    void init();

    std::string file, next_row_;
    std::unordered_set<int> str_cols_;
    std::vector<std::string> col_names_;
    std::ifstream data_file_;
};

// ---------------------------------------------------------------------------------
struct CSVTabularData {
    std::shared_ptr<CSVTabularDataImpl> sp;

    CSVTabularData(const std::string &raw_data_file,
                   TimeStrConv::func_type time_converter = nullptr)
        : sp(std::make_shared<CSVTabularDataImpl>(raw_data_file, time_converter)) {}
    // You have to ensure that two file have the same rows.
    CSVTabularData(const std::string &raw_data_file, const std::string &adjusted_data_file,
                   TimeStrConv::func_type time_converter = nullptr)
        : sp(std::make_shared<CSVTabularDataImpl>(raw_data_file, adjusted_data_file,
                                                  time_converter)) {}

    bool read() { return sp->read(); }
};

struct CSVDirectoryData {
    std::shared_ptr<CSVDirectoryDataImpl> sp;

    CSVDirectoryData(const std::string &raw_data_dir,
                     std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                     TimeStrConv::func_type time_converter = nullptr)
        : sp(std::make_shared<CSVDirectoryDataImpl>(raw_data_dir, tohlc_map, time_converter)) {}
    CSVDirectoryData(const std::string &raw_data_dir, const std::string &adj_data_dir,
                     std::array<int, 5> tohlc_map = {0, 1, 2, 3, 4},
                     TimeStrConv::func_type time_converter = nullptr)
        : sp(std::make_shared<CSVDirectoryDataImpl>(raw_data_dir, adj_data_dir, tohlc_map,
                                                    time_converter)) {}

    bool read() { return sp->read(); }

    CSVDirectoryData &extra_num_col(const std::vector<std::pair<int, std::string>> &cols) {
        sp->extra_num_col(cols);
        return *this;
    }

    CSVDirectoryData &extra_str_col(const std::vector<std::pair<int, std::string>> &cols) {
        sp->extra_str_col(cols);
        return *this;
    }

    CSVDirectoryData &code_extractor(std::function<std::string(std::string)> fun) {
        sp->code_extractor(fun);
        return *this;
    };
};

// If no str_cols, then all columns are assumed to be numeric.
struct CSVCommonData {
    std::shared_ptr<CSVCommonDataImpl> sp;

    CSVCommonData(const std::string &file, TimeStrConv::func_type time_converter = nullptr,
                  const std::vector<int> &str_cols = {})
        : sp(std::make_shared<CSVCommonDataImpl>(file, time_converter, str_cols)) {}

    bool read() { return sp->read(); }
    CommonFeedData *data_ptr() { return sp->data_ptr(); }
};
struct GenericCommonDataFeed {
    std::shared_ptr<GenericDataImpl<CommonFeedData>> sp;
    GenericCommonDataFeed(const CSVCommonData &data) : sp(data.sp) {}

    bool read() { return sp->read(); }
    const auto &time() const { return sp->data().time; }
    CommonFeedData *data_ptr() { return sp->data_ptr(); }
};

struct GenericPriceDataFeed {
    std::shared_ptr<GenericPriceDataImpl> sp;

    GenericPriceDataFeed(const CSVTabularData &data) : sp(data.sp) {}

    GenericPriceDataFeed(const CSVDirectoryData &data) : sp(data.sp) {}

    bool read() { return sp->read(); }
    PriceFeedData *data_ptr() { return sp->data_ptr(); }
    int assets() const { return sp->assets(); }
    const auto &time() const { return sp->data().time; }
    const auto &codes() const { return sp->codes(); }
};

//-------------------------------------------------------------------
template <typename DataT, typename FeedT, typename BufferT> class GenericFeedsAggragator {
  public:
    GenericFeedsAggragator() = default;

    const auto &datas() const { return data_; }
    const auto &datas_valid() const { return datas_valid_; }
    bool data_valid(int feed) const { return datas_valid_.coeff(feed); }

    const auto &data(int i) const { return data_[i]; }

    const auto feed(int i) const { return feeds_[i]; }

    void add_feed(const FeedT &feed);

    void init();
    auto finished() const { return finished_; }
    bool read();

    auto data_ptr() { return &data_; }
    const auto &time() const { return time_; }

    void set_window(int src, int window) { data_[src].set_window(window); };

    // If two aggragators have different dates, then align them.
    template <typename T1, typename T2, typename T3>
    void align_with(const GenericFeedsAggragator<T1, T2, T3> &target);

  private:
    std::vector<State> status_;
    std::vector<ptime> times_;

    std::vector<const DataT *> next_;
    std::vector<FeedT> feeds_;
    std::vector<BufferT> data_;

    VecArrXb datas_valid_;
    bool finished_ = false;

    ptime time_;
};

using PriceFeedAggragator =
    GenericFeedsAggragator<PriceFeedData, GenericPriceDataFeed, PriceFeedDataBuffer>;
using CommonFeedAggragator =
    GenericFeedsAggragator<CommonFeedData, GenericCommonDataFeed, CommonFeedDataBuffer>;

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

//---------------------------------------------------------------

std::vector<std::string> CSVRowParaser::parse_row(const std::string &row) {
    boost::tokenizer<boost::escaped_list_separator<char>> tok(row, esc_list_sep);
    std::vector<std::string> res;
    for (auto beg = tok.begin(); beg != tok.end(); ++beg) {
        res.emplace_back(*beg);
    }
    return res;
}
double backtradercpp::feeds::CSVRowParaser::parse_double(const std::string &ele) {
    double res = std::numeric_limits<double>::quiet_NaN();
    std::string s = boost::algorithm::trim_copy(ele);
    try {
        res = boost::lexical_cast<double>(s);
    } catch (...) {
        util::cout("Bad cast: {}\n", ele);
    }
    return res;
}

inline CSVTabularDataImpl::CSVTabularDataImpl(const std::string &raw_data_file,
                                              TimeStrConv::func_type time_converter)
    : GenericPriceDataImpl(time_converter), raw_data_file_(raw_data_file),
      adj_data_file_(raw_data_file) {
    backtradercpp::util::check_path_exists(raw_data_file);
    init();
}

CSVTabularDataImpl::CSVTabularDataImpl(const std::string &raw_data_file,
                                       const std::string &adjusted_data_file,
                                       TimeStrConv::func_type time_converter)
    : GenericPriceDataImpl(time_converter) {
    backtradercpp::util::check_path_exists(raw_data_file);
    raw_data_file_ = std::ifstream(raw_data_file);
    backtradercpp::util::check_path_exists(adjusted_data_file);
    adj_data_file_ = std::ifstream(adjusted_data_file);

    init();
}

void CSVTabularDataImpl::init() {

    // Read header
    std::string header;
    std::getline(raw_data_file_, header);
    std::getline(adj_data_file_, header);

    // Read one line and detect assets.
    auto row_string = CSVRowParaser::parse_row(header);
    assets_ = row_string.size() - 1;
    next_.resize(assets_);

    // Set codes.
    for (int i = 1; i < row_string.size(); ++i) {
        codes_.emplace_back(std::move(row_string[i]));
    }

    GenericPriceDataImpl::init();
}

inline void CSVTabularDataImpl::cast_ohlc_data_(std::vector<std::string> &row_string,
                                                OHLCData &dest) {
    for (int i = 0; i < assets_; ++i) {
        // dest.open.coeffRef(i) = std::numeric_limits<double>::quiet_NaN();
        dest.open.coeffRef(i) = 0;
        auto &s = row_string[i + 1];
        try {
            boost::algorithm::trim(s);
            dest.open.coeffRef(i) = boost::lexical_cast<double>(s);
        } catch (...) {
            util::cout("Bad cast: {}\n", s);
        }
    }
    dest.high = dest.low = dest.close = dest.open;
};

inline bool CSVTabularDataImpl::read() {
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

template <typename DataT, typename FeedT, typename BufferT>
inline bool GenericFeedsAggragator<DataT, FeedT, BufferT>::read() {
    bool all_finished = true;
    for (int i = 0; i < feeds_.size(); ++i) {
        if (status_[i] == Invalid) {
            bool _get = feeds_[i].read();
            if (_get) {
                times_[i] = feeds_[i].time();
                status_[i] = Valid;
                all_finished = false;
            } else {
                status_[i] = Finished;
                times_[i] = boost::posix_time::max_date_time;
            }
        }
    }

    if (all_finished) {
        return false;
    }

    // Use minimum date as the next date.
    auto p = std::min_element(times_.begin(), times_.end());
    time_ = *p;
    for (int i = 0; i < feeds_.size(); ++i) {
        if (times_[i] == time_) {
            data_[i].push_back(feeds_[i].sp->next());
            status_[i] = Invalid;
            datas_valid_[i] = true;
        } else {
            data_[i].push_back_();
            datas_valid_[i] = false;
        }
    }
    bool success = datas_valid_.any();
    finished_ = !success;
    return success;
}
// inline const std::vector<FullAssetData> &FeedsAggragator::get_and_read() {
//     read();
//     return data_;
// }
template <typename DataT, typename FeedT, typename BufferT>
inline void GenericFeedsAggragator<DataT, FeedT, BufferT>::add_feed(const FeedT &feed) {
    status_.emplace_back(State::Invalid);
    times_.emplace_back();

    feeds_.push_back(feed);
    next_.push_back(&(feed.sp->next()));
    data_.emplace_back(feed.sp->next());
    datas_valid_.resize(feeds_.size());
}

inline CSVDirectoryDataImpl::CSVDirectoryDataImpl(const std::string &raw_data_dir,
                                                  std::array<int, 5> tohlc_map,
                                                  TimeStrConv::func_type time_converter)
    : GenericPriceDataImpl(time_converter), raw_data_dir(raw_data_dir), adj_data_dir(raw_data_dir),
      tohlc_map(tohlc_map) {
    init();
}

inline CSVDirectoryDataImpl::CSVDirectoryDataImpl(const std::string &raw_data_dir,
                                                  const std::string &adj_data_dir,
                                                  std::array<int, 5> tohlc_map,
                                                  TimeStrConv::func_type time_converter)
    : GenericPriceDataImpl(time_converter), raw_data_dir(raw_data_dir), adj_data_dir(adj_data_dir),
      tohlc_map(tohlc_map) {
    init();
}

inline void CSVDirectoryDataImpl::init() {
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

    // init_extra_data();

    GenericPriceDataImpl::init();
}

#define UNWRAP(...) __VA_ARGS__
#define BK_CSVDirectoryDataImpl_extra_col(name, init)                                              \
    inline CSVDirectoryDataImpl &CSVDirectoryDataImpl::extra_##name##_col(                         \
        const std::vector<std::pair<int, std::string>> &cols) {                                    \
        for (const auto &col : cols) {                                                             \
            extra_##name##_col_.emplace_back(col.first);                                           \
                                                                                                   \
            const auto &name_ = col.second;                                                        \
            extra_##name##_col_names_.emplace_back(name_);                                         \
            next_.name##_data_[name_] = init;                                                      \
            std::cout << name_ << " size " << next_.name##_data_[name_].size() << std::endl;       \
            extra_##name##_data_ref_.emplace_back(next_.name##_data_[name_]);                      \
        }                                                                                          \
        std::cout << extra_num_data_ref_[0].get().size() << std::endl;                             \
        return *this;                                                                              \
    }
BK_CSVDirectoryDataImpl_extra_col(num, UNWRAP(VecArrXd::Zero(assets_)));
BK_CSVDirectoryDataImpl_extra_col(str, UNWRAP(std::vector<std::string>(assets_)));
#undef BK_CSVDirectoryDataImpl_extra_col
#undef UNWRAP

bool CSVDirectoryDataImpl::read() {

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
#pragma omp parallel for
        for (int i = 0; i < assets_; ++i) {
            if ((status[i] == State::Valid) && (times[i] == *it)) {
                for (int j = 0; j < 4; ++j) {
                    // Fill ohlc data.
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

                // Fill extra data
                for (int j = 0; j < extra_num_col_.size(); ++j) {
                    extra_num_data_ref_[j].get().coeffRef(i) =
                        CSVRowParaser::parse_double(raw_parsed_buffer[i][extra_num_col_[j]]);
                }
                for (int j = 0; j < extra_str_col_.size(); ++j) {
                    extra_str_data_ref_[j].get()[i] = raw_parsed_buffer[i][extra_str_col_[j]];
                }

                status[i] = State::Invalid; // After read data, set to invalid.
            }
        }
        next_.validate_assets();
        return true;
    } else {
        return false;
    }
}

CSVCommonDataImpl ::CSVCommonDataImpl(const std::string &file,
                                      TimeStrConv::func_type time_converter,
                                      const std::vector<int> str_cols)
    : GenericDataImpl(time_converter), file(file), str_cols_(str_cols.begin(), str_cols.end()) {
    util::check_path_exists(file);
    data_file_ = std::ifstream(file);
    init();
}
bool CSVCommonDataImpl::read() {
    std::getline(data_file_, next_row_);
    if (data_file_.eof()) {
        finished_ = true;
        return false;
    }
    auto row_string = CSVRowParaser::parse_row(next_row_);
    auto &ts = row_string[0];
    if (time_converter_) {
        ts = time_converter_(ts);
    }
    next_.time = boost::posix_time::time_from_string(ts);

    for (int i = 1; i < row_string.size(); ++i) {
        if (!str_cols_.contains(i)) {
            next_.num_data_[col_names_[i - 1]] = CSVRowParaser::parse_double(row_string[i]);
        } else {
            next_.str_data_[col_names_[i - 1]] = row_string[i];
        }
    }
    // std::cout << util::format_map(next_.num_data_) << std::endl;

    return true;
}

void CSVCommonDataImpl::init() { // Read header

    std::string header;
    std::getline(data_file_, header);

    // Read one line and set col_names.
    auto row_string = CSVRowParaser::parse_row(header);
    for (int i = 1; i < row_string.size(); ++i) {
        boost::algorithm::trim(row_string[i]);
        col_names_.emplace_back(std::move(row_string[i]));
    }
}

} // namespace feeds
} // namespace backtradercpp