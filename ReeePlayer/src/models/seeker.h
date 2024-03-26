#pragma once

class Seeker
{
public:
    virtual ~Seeker() = default;
    virtual std::optional<PlaybackTime> seek(PlaybackTime) const = 0;
};