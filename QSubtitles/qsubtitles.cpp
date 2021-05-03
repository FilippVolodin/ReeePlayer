#include "pch.h"
#include "qsubtitles.h"
#include "webvtt.h"

namespace qsubs
{

    ISubtitlesPtr load(const QString& filename)
    {
        // TODO
        return webvtt::parse(filename);
    }

    SubtitlePicker::SubtitlePicker(ISubtitlesPtr subtitles) : m_subtitles(subtitles)
    {

    }

    const ICue* SubtitlePicker::pick() const
    {
        return nullptr;
    }

}