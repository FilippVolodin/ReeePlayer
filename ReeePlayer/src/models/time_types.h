#ifndef TIME_TYPES_H
#define TIME_TYPES_H

using TimePoint = std::chrono::time_point<
    std::chrono::system_clock,
    std::chrono::seconds>;

TimePoint now();

using Duration = std::chrono::seconds;

#endif // !TIME_TYPES_H