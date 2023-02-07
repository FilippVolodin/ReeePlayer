#include "pch.h"
#include "session.h"
#include "repetition_model.h"
#include "clip_storage.h"
#include "app.h"
#include "library.h"
#include <srs_interfaces.h>

ClipPriorityCmp::ClipPriorityCmp(TimePoint cur_time)
    : m_cur_time(cur_time)
{
}

bool ClipPriorityCmp::operator()(const Clip* lhs, const Clip* rhs)
{
    float p1 = lhs->get_card() != nullptr ?
        lhs->get_card()->get_priority(m_cur_time) : 0.0f;
    float p2 = rhs->get_card() != nullptr ?
        rhs->get_card()->get_priority(m_cur_time) : 0.0f;
    return p1 > p2;
}

Session::Session(Library* library, const std::vector<File*>& files)
    : m_library(library)
{
    connect(m_library, &Library::clip_removed_sig, this, &Session::remove_clip);

    m_showed_clips.clear();
    m_showing_clip_index = -1;

    for (const File* file : files)
    {
        const Clips& clips = file->get_clips();
        m_clips.insert(m_clips.end(), clips.begin(), clips.end());
    }
}

Session::Session(Library*, const std::vector<Clip*>& clips) : m_clips(clips)
{
}

Session::~Session()
{
}

bool Session::has_prev_clip() const
{
    return m_showing_clip_index > 0;
}

bool Session::has_next_clip() const
{
    return m_showing_clip_index + 1 < m_showed_clips.size();
}

Clip* Session::get_prev_clip()
{
    if (!has_prev_clip())
        return nullptr;
        
    --m_showing_clip_index;
    return m_showed_clips[m_showing_clip_index];
}

Clip* Session::get_next_clip()
{
    if (m_clips.empty())
        return nullptr;

    Clip* clip;
    if (m_showing_clip_index + 1 < m_showed_clips.size())
    {
        clip = m_showed_clips[m_showing_clip_index + 1];
    }
    else
    {
        auto it = std::min_element(m_clips.begin(), m_clips.end(), ClipPriorityCmp(now()));
        if (it == m_clips.end())
            return nullptr;

        clip = *it;
        m_showed_clips.push_back(clip);
    }

    ++m_showing_clip_index;
    return clip;
}

void Session::remove_clip(Clip* clip)
{
    int showed_num = std::count(
        m_showed_clips.begin(),
        m_showed_clips.begin() + m_showing_clip_index + 1,
        clip
    );
    m_showing_clip_index -= showed_num;

    m_showed_clips.erase(
        remove(m_showed_clips.begin(),
            m_showed_clips.end(), clip),
        m_showed_clips.end());

    auto it = std::find(m_clips.begin(), m_clips.end(), clip);
    if (it != m_clips.end())
        m_clips.erase(it);
}

int Session::remain_clips()
{
    TimePoint cur_time = now();

    auto need_to_repeat = [cur_time](const Clip* clip) -> bool
    {
        return clip->get_card() != nullptr ? clip->get_card()->is_due(cur_time) : false;
    };

    return std::count_if(m_clips.begin(), m_clips.end(), need_to_repeat);
}

int Session::get_num_clips() const
{
    return m_clips.size();
}
