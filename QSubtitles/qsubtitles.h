#pragma once

#include "types.h"

namespace qsubs
{

    class ICue
    {
    public:
        virtual int get_index() const = 0;
        virtual Timestamp get_start_time() const = 0;
        virtual Timestamp get_end_time() const = 0;
        virtual QString get_text() const = 0;
    };

    class ISubtitles
    {
    public:
        virtual ~ISubtitles() = default;

        virtual int get_num_cues() const = 0;
        virtual const ICue* get_cue(int index) const = 0;
        virtual const ICue* pick_cue(Timestamp time, bool check_end_time) const = 0;
    };

    using ISubtitlesPtr = std::shared_ptr<const ISubtitles>;

    ISubtitlesPtr load(const QString& filename);

    class SubtitlePicker
    {
        ISubtitlesPtr m_subtitles;
    public:
        SubtitlePicker(ISubtitlesPtr);

        const ICue* pick() const;
    };
}