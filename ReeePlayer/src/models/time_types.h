#ifndef TIME_TYPES_H
#define TIME_TYPES_H

using TimePoint = std::chrono::time_point<
    std::chrono::system_clock,
    std::chrono::seconds>;

using Duration = std::chrono::seconds;

TimePoint now();
QString get_interval_str(Duration);

int round50(int val);

#endif // !TIME_TYPES_H