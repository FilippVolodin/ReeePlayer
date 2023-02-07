#include <pch.h>
#include <time_types.h>

TimePoint now()
{
    using namespace std::chrono;
    return time_point_cast<seconds>(system_clock::now());
}
