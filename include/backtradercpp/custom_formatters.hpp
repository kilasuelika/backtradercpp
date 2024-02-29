#include <fmt/core.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace fmt {

template <>
struct formatter<boost::gregorian::greg_year> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const boost::gregorian::greg_year& y, FormatContext& ctx) -> decltype(ctx.out()) {
    // 雿輻甇?＆?撘挪??greg_year ?僑隞賢?
    return format_to(ctx.out(), "{}", static_cast<int>(y)); // 瘜冽?嚗?亥蓮?Ｖ蛹 int
  }
};

template <>
struct formatter<boost::posix_time::ptime> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const boost::posix_time::ptime& t, FormatContext& ctx) -> decltype(ctx.out()) {
    auto date = t.date();
    auto tod = t.time_of_day();
    return format_to(
      ctx.out(),
      "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
      date.year(), date.month().as_number(), date.day(),
      tod.hours(), tod.minutes(), tod.seconds()
    );
  }
};

}  // namespace fmt

