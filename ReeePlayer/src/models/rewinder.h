#pragma once

#include <time_types.h>

class Rewinder
{
public:
    virtual ~Rewinder() = default;
    virtual std::optional<PlaybackTime> rewind(PlaybackTime, PlaybackTimeDiff) const = 0;
};